/* system includes */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <assert.h>
#include <math.h>
#include <stdbool.h>

#include <unistd.h>
#include <getopt.h>

#include <hdf5.h>
#include <hdf5_hl.h>

#define _BSD_SOURCE
#include <endian.h>

/* local includes */
#include "mathutils.h"
#include "timeutils.h"
#include "h5utils.h"
#include "timeutils.h"
#include "mseviri.h"
#include "cgms_xrit.h"
#include "cds_time.h"
#include "msevi_l15.h"
#include "msevi_l15hrit.h"
#include "msevi_l15hdf.h"
#include "eum_wavelet.h"
#include "geos.h"
#include "sunpos.h"

struct prog_opts{
	int    nchan;
	char   *chan[12];
	time_t time;
	char   *dir;
	struct msevi_l15_coverage coverage;
	int    sunpos;
	int    satpos;
	bool   write_geolocation;
	bool   write_sun_angles;
	bool   write_sat_angles;
} popts= {
	.nchan    = 4,
	.chan     = { "vis006", "vis008", "ir_016", "ir_108" },
	.time     = 0,
	.dir      = ".",
	.coverage = { "vis_ir", 1296, 1332, 1857, 2210 }, /* RSS */
	// .coverage = { "vis_ir", 2957, 3556, 1557, 2356}, /* RSS */
	//.coverage = { "vis_ir", 2957, 3556, 1357, 2156}, /* HRS */
	.sunpos   = 0,
	.satpos   = 0,
	.write_geolocation = false,
	.write_sun_angles  = true,
	.write_sat_angles  = true,
};

static void print_usage (char *prog_name)
{
	printf ( "Usage: %s [OPTS]\n"
		 "Convert METEOSAT SEVIRI HRIT files to HDF5 format\n\n"
		 "Options:\n"
		 "\t-h, --help\t\tshow this help message\n"
		 "\t-d DIR, --dir=DIR\tdirectory containing the HRIT files (default:\n\t\t\t\tcurrent dir)\n"
		 "\t-S, --sun\t\tadd sun angles\n"
		 "\t-V, --view\t\tadd satellite viewing angles\n"
		 "\t-t TIME, --time=TIME\ttime of SEVIRI scan\n", prog_name );
	return;
}

static int parse_args (int argc, char **argv)
{
	int  optidx = 1, r=-1;
	char optstr[] = "hSVc:d:r:t:";
	char c;

	const struct option pargs [] = {
                 { .name = "help",   .has_arg = 0, .flag = NULL, .val = 'h'},
                 { .name = "chan",   .has_arg = 1, .flag = NULL, .val = 'c'},
                 { .name = "dir",    .has_arg = 1, .flag = NULL, .val = 'd'},
                 { .name = "time",   .has_arg = 1, .flag = NULL, .val = 't'},
                 { .name = "region", .has_arg = 1, .flag = NULL, .val = 'r'},
	};
	
	while (1) {
		c = getopt_long (argc, argv, optstr, pargs, &optidx);

		if (c == -1) break;
		switch (c) {
		case 'h':
			print_usage(argv[0]);
			exit(0);
		case 'S':
			popts.sunpos = 1;
			break;
		case 'V':
			popts.satpos = 1;
			break;
		case 'c':
			break;
		case 't':
			r = parse_utc_timestr( optarg, "%Y%m%d%H%M", &popts.time);
			break;
		case 'd':
			popts.dir = optarg;
			break;
		default:
			return -1;
		}
	}
	return r;
}

int write_pgm(char *fnam, int nlin, int ncol, uint16_t *data, char *comment )
{
	FILE *fp;
	int i;
	uint16_t val;
	
	fp = fopen(fnam, "w" );
	if( comment==NULL ) {
		fprintf( fp, "P5\n%d %d\n1023\n", ncol, nlin );
	} else {
		//fprintf( fp, "P5\n%d %d\n1023\n", ncol, nlin );
		fprintf( fp, "P5\n%d %d\n# %32s\n1023\n", ncol, nlin, comment );
	}
	for( i=0; i<nlin*ncol; i++ ) {
		val = htobe16( *data );
		fwrite( &val, 2, 1, fp );
		data++;
	}
	fclose(fp);
	return 0;
}

int main (int argc, char **argv)
{
	int i, sat_id;
	char *fnam_pgm = NULL;
	struct msevi_l15hrit_flist *flist;
	char *service;
	char *sat;

	struct msevi_l15_header  *header;
	struct msevi_l15_trailer *trailer;
	struct msevi_l15_image *img;
	char *timestr;
	double jd;
	bool   rss=true;

	// float *lat, *lon, *muS, *azS, *mu0, *az0;
	// RSS: char *reg_str = "800x600+1356+156";
	// HRS: char *reg_str = "800x600+1556+156";
	char *reg_str = "354x37+1502+2380";
	struct geos_ctx *gc;
	char cal_str[128];

	/* parse command line arguments */
	if (parse_args (argc, argv) <0) {
		print_usage( argv[0] );
		return -1;
	}

	/* get filenames  */
	flist = msevi_l15hrit_get_flist( popts.dir, &popts.time );
	if( (flist->prologue==NULL) | (flist->epilogue==NULL) ) {
		fprintf( stderr, "Unable to find pro/epilogue files\n" );
		goto err_out;
	}

	/* read pro/epilogue */
	header  = msevi_l15hrit_read_prologue( flist->prologue );
	trailer = msevi_l15hrit_read_epilogue( flist->epilogue );
	if( (header == NULL) | (trailer == NULL) ) {
		fprintf(stderr, "Unable to read HRIT pro/epilogue files\n");
		fprintf(stderr, "%s\n", flist->prologue);
		fprintf(stderr, "%s\n", flist->epilogue);
		goto err_out;
	}

	/* set service name */
	if( strstr(flist->prologue,"RSS") ) {
		service = "rss";
	} else {
		service = "hrs";
	}

	/* set sat name */
	sat_id = header->satellite_status.satellite_definition.satellite_id;
	switch( sat_id ) {
	case 321:
		sat = "msg1";
		break;
	case 322:
		sat = "msg2";
		break;
	default:
		printf("ERROR: unknown sat_id=%d\n", sat_id );
		return -1;
	}

	/* ... read channels and add images to HDF file */
	for( i=0; i<popts.nchan; i++ ) {
		int id;
		
		/* Create file ... */
		fnam_pgm = calloc( strlen(popts.dir)+256, 1 );
		timestr = get_utc_timestr( "%Y%m%d%H%M", popts.time );
		sprintf( fnam_pgm, "%s/%s-sevi-%s-%s-%s-%s.pgm", popts.dir, sat, timestr,
			 service, "sc", popts.chan[i] );
		printf( "Creating: %s\n", fnam_pgm );
		free(timestr);

		printf( "Reading channel=%s\n", popts.chan[i] );
		id  = msevi_chan2id( popts.chan[i] );
		img = msevi_l15hrit_read_image( flist->nseg[id-1], flist->channel[id-1],
						&popts.coverage );
		if(img==NULL) goto err_out;
		msevi_l15hrit_annotate_image( img, header, trailer );
		sprintf( cal_str, "cal_slope=%.8f cal_offset=%.8f", img->cal_slope, img->cal_offset );
		write_pgm( fnam_pgm, img->nlin, img->ncol, img->counts, cal_str );
		msevi_l15_image_free( img );
	}

	/* cleanup */
	msevi_l15hrit_free_flist( flist );
	free(header);
	free(trailer);

	return  0;

err_out:
	printf("Error\n");
	return -1;
	
}


