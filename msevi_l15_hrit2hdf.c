/* system includes */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <ctype.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include <hdf5.h>
#include <hdf5_hl.h>

/* local includes */
#include "mathutils.h"
#include "timeutils.h"
#include "h5utils.h"
#include "timeutils.h"
#include "cgms_xrit.h"
#include "cds_time.h"
#include "msevi_l15.h"
#include "msevi_l15hrit.h"
#include "msevi_l15hdf.h"
#include "eum_wavelet.h"
#include "geos.h"
#include "sunpos.h"

struct prog_opts {
	int    nchan;
	char   *chan[12];
	time_t time;
	char   *dir;
	char   *region;
	char   *service;
	struct msevi_l15_coverage coverage;
	int    sunpos;
	int    satpos;
	bool   write_geolocation;
	bool   write_sun_angles;
	bool   write_sat_angles;
} popts= {
	.nchan    = 12,
	.chan     = { "vis006", "vis008", "ir_016", "ir_039", "wv_062", 
		      "wv_073", "ir_087", "ir_097", "ir_108", "ir_120", 
		      "ir_134", "hrv" },
	.time     = 0,
	.dir      = ".",
	.region   = "eu",
	.service  = "pzs",
	//.coverage = { "vis_ir", 2957, 3556, 1557, 2356}, /* RSS */
	.coverage = { "vis_ir", 2957, 3556, 1357, 2156}, /* HRS */
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
		 "\t-r, --region\t\tspecify region\n"
		 "\t-s, --service\t\tspecify satellite service (pzs or rss)\n"
		 "\t-t TIME, --time=TIME\ttime of SEVIRI scan\n", prog_name );
	return;
}

static int parse_args (int argc, char **argv)
{
	int  optidx = 1, r=-1;
	char optstr[] = "hSVc:d:r:s:t:";
	char c;

	const struct option pargs [] = {
                 { .name = "help",    .has_arg = 0, .flag = NULL, .val = 'h'},
                 { .name = "chan",    .has_arg = 1, .flag = NULL, .val = 'c'},
                 { .name = "dir",     .has_arg = 1, .flag = NULL, .val = 'd'},
                 { .name = "time",    .has_arg = 1, .flag = NULL, .val = 't'},
                 { .name = "region",  .has_arg = 1, .flag = NULL, .val = 'r'},
                 { .name = "service", .has_arg = 1, .flag = NULL, .val = 's'},
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
		case 'r':
			popts.region = optarg;
			break;
		case 't':
                        optarg[8] = toupper(optarg[8]); /* allow lowercase t */
                        r = parse_utc_timestr( optarg, "%Y%m%dT%H%M", &popts.time);
			break;
		case 's':
 			popts.service = optarg;
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

int write_cds_time( hid_t hid, char *name, int n, struct cds_time *t )
{
	size_t cds_size     = sizeof(struct cds_time);
	size_t cds_off[2]   = { offsetof(struct cds_time, days),
				offsetof(struct cds_time, msec)  };
	// size_t cds_msize[2] = { sizeof(uint16_t), sizeof(uint32_t) };
	hid_t  cds_type[2]  = { H5T_NATIVE_UINT16, H5T_NATIVE_UINT32 };
	const char *cds_names[2]  = { "days", "milliseconds" };
	
	H5TBmake_table( "cds_time", hid, name, 2, n, cds_size, cds_names,
			cds_off, cds_type, 32, NULL, 6, t );
	return 0;
}

static int sdset_annotate( hid_t hid, char *name, char *long_name, char *units,
			   double scale, double offset )
{
	int r;

	r = H5LTset_attribute_double(hid, name, "scale_factor", &scale, 1);
	if(r<0) goto err_out;
	r = H5LTset_attribute_double(hid, name, "add_offset", &offset, 1);
	if(r<0) goto err_out;
	r = H5LTset_attribute_string(hid, name, "units", units);
	if(r<0) goto err_out;
	r = H5LTset_attribute_string(hid, name, "long_name",long_name);
	if(r<0) goto err_out;
	return 0;

 err_out:
	return r;
}

static void coverage_visir2hrv( struct msevi_l15_coverage *vi,struct msevi_l15_coverage *hrv )
{
	strcpy( hrv->channel, "hrv" );
	hrv->southern_line  = vi->southern_line*3-3;
	hrv->northern_line  = vi->northern_line*3-1;
	hrv->eastern_column = vi->eastern_column*3-3;
	hrv->western_column = vi->western_column*3-1;
	return;
}

int main (int argc, char **argv)
{
	hid_t fid;
	hid_t img_gid, meta_gid, lsi_gid, geom_gid;
	int i, r, npix, sat_id;
	char *fnam_hdf = NULL;
	struct msevi_l15hrit_flist *flist;
	double x0, y0, dx, dy;
	const int coff=1856, cfac=13642337, loff=1856, lfac=13642337;

	char tstamp[32], sbuf[4096], host[32];
	time_t now;

	struct msevi_l15_header  *header;
	struct msevi_l15_trailer *trailer;
	struct msevi_l15_image   *img;
	struct msevi_satinf      *satinf;
	struct msevi_region      *reg;

	char *timestr;
	struct cds_time *line_acq_time;
	double jd;

	hsize_t dim[2];
	float *lat, *lon, *muS, *azS, *mu0, *az0;
	// RSS: reg_str = "800x600+1356+156";
	// HRS: reg_str = "800x600+1556+156";
	// StratoCu: reg_str = "354x37+1502+2380";
	struct geos_param *gp;
	double proj_ss_lon = 0.0, true_ss_lon = 0.0;

	/* parse command line arguments */
	if (parse_args (argc, argv) <0) {
		print_usage( argv[0] );
		return -1;
	}

	/* get filenames  */
	flist = msevi_l15hrit_get_flist( popts.dir, &popts.time, popts.service );
	if( (flist->prologue==NULL) | (flist->epilogue==NULL) ) {
		fprintf( stderr, "Unable to find pro/epilogue files\n" );
		goto err_out;
	}

	/* read pro/epilogue */
	header  = msevi_l15hrit_read_prologue( flist->prologue );
	trailer = msevi_l15hrit_read_epilogue( flist->epilogue );
	if( (header == NULL) | (trailer == NULL) ) {
		fprintf(stderr, "Unable to read HRIT pro/epilogue files\n");
		printf("%s\n", flist->prologue);
		printf("%s\n", flist->epilogue);
		goto err_out;
	}

	/* init misc. parameters */
	sat_id = header->satellite_status.satellite_definition.satellite_id;

	satinf = msevi_read_satinf( "msevi_satinf.json", sat_id );
	if( satinf==NULL ) {
		printf("ERROR: sat_id=%d\n", sat_id );
		printf("Unable to read satellite info\n" );
		return -1;
	}
	reg = msevi_read_region( "msevi_region.json", popts.service, popts.region );
	if( reg==NULL ) {
		printf("ERROR: region=%s svc=%s\n", popts.region, popts.service);
		printf("Unable to find region\n");
		return -1;
	}

	popts.coverage.northern_line  = 3712-reg->lin0;
	popts.coverage.southern_line  = 3712-(reg->lin0+reg->nlin-1);
	popts.coverage.western_column = 3712-reg->col0;
	popts.coverage.eastern_column = 3712-(reg->col0+reg->ncol-1);

	line_acq_time = calloc( reg->nlin, sizeof(struct cds_time));

	/* Create file ... */
	fnam_hdf = calloc( strlen(popts.dir)+256, 1 );
	timestr = get_utc_timestr( "%Y%m%dt%H%Mz", popts.time );
	sprintf( fnam_hdf, "%s/%s-sevi-%s-l15hdf-%s-%s.c2.h5", popts.dir, satinf->name, timestr,
		 popts.service, popts.region );
	printf( "Creating: %s\n", fnam_hdf );
	free(timestr);

	fid = H5Fcreate( fnam_hdf, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
	if(fid<0) goto err_out;

	/* add various attributes */
	r = H5LTset_attribute_string(fid, "/", "version", "2.0.0" );
	r = H5LTset_attribute_ushort(fid, "/", "satellite_id", &sat_id, 1);
	now = time(NULL);
	gethostname(host, 32);
	strftime( tstamp, 32,"%Y-%m-%dT%H:%M:%SZ", gmtime(&now) );
	sprintf( sbuf, "%s: HDF5 file generated by user %s on %s using msevi_l15_hrit2hdf\n", tstamp, getlogin(), host);
	r = H5LTset_attribute_string(fid, "/", "history", sbuf );
	r = H5LTset_attribute_string(fid, "/", "title", "METEOSAT SEVIRI level1.5 image data");
	r = H5LTset_attribute_string(fid, "/", "institution", "Leibniz-Institute for Tropospheric Research (TROPOS), Leipzig, Germany");
	r = H5LTset_attribute_string(fid, "/", "contact", "sat@tropos.de");
	r = H5LTset_attribute_string(fid, "/", "reference", "http://sat.tropos.de/docs/msevi_l15hdf_filespec.pdf");
	sprintf( sbuf, "%s,  Spacecraft ID: %d, Spinning Enhanced Visible and Infrared Imager (SEVIRI)",  satinf->long_name, satinf->id);
	r = H5LTset_attribute_string(fid, "/", "source", sbuf );
	r = H5LTset_attribute_string(fid, "/", "copyright", "EUMETSAT/TROPOS");

	/* Create HDF groups */
	img_gid = H5Gcreate2( fid, msevi_l15hdf_img_grp, 0, H5P_DEFAULT, H5P_DEFAULT );
	if(img_gid<0) goto err_out;
	meta_gid = H5Gcreate2( fid, msevi_l15hdf_meta_grp, 0, H5P_DEFAULT, H5P_DEFAULT );
	if(meta_gid<0) goto err_out;
	lsi_gid = H5Gcreate2( meta_gid, msevi_l15hdf_lsi_grp, 0, H5P_DEFAULT, H5P_DEFAULT );
	if(lsi_gid<0) goto err_out;
	geom_gid = H5Gcreate2( fid, "geometry", 0, H5P_DEFAULT, H5P_DEFAULT );
	if(geom_gid<0) goto err_out;

	/* add coverage */
	msevi_l15hdf_write_coverage( meta_gid, "coverage", &popts.coverage );

	/* ... read channels and add images to HDF file */
	for( i=0; i<popts.nchan; i++ ) {
		int r, id;
		struct msevi_chaninf *chaninf;
		
		printf( "Reading channel=%s\n", popts.chan[i] );
		id = msevi_chan2id( popts.chan[i] );
		if( id!=12 ) { /* normal resolution channels */
			img = msevi_l15hrit_read_image( flist->nseg[id-1], flist->channel[id-1],
							&popts.coverage );
		} else {
			/* HRV channel */
			struct msevi_l15_coverage hrv_cov;
			coverage_visir2hrv( &popts.coverage, &hrv_cov);
			img = msevi_l15hrit_read_image( flist->nseg[id-1], flist->channel[id-1],
							&hrv_cov );
			
			msevi_l15hdf_append_coverage( meta_gid, "coverage", &hrv_cov );
		}
		if(img==NULL) goto err_out;
		chaninf = msevi_get_chaninf( satinf, id );
		msevi_l15hrit_annotate_image( img, header, trailer, chaninf );

		/* save image information to hdf */
		r = msevi_l15hdf_write_image( img_gid, img );
		if(r<0) goto err_out;

		r = msevi_l15hdf_write_line_side_info( lsi_gid, img );
		if(r<0) goto err_out;

		if( i==0 ) {
			int l;
			for( l=0; l<reg->nlin; l++ ) {
				line_acq_time[l].days = img->line_side_info[l].acquisition_time.days;
				line_acq_time[l].msec = img->line_side_info[l].acquisition_time.msec;
			}
			write_cds_time( meta_gid, "line_mean_acquisition_time", reg->nlin, line_acq_time );
		}

		satinf->chaninf[i].cal_slope = img->cal_slope;
		satinf->chaninf[i].cal_offset = img->cal_offset;
		satinf->chaninf[i].refl_slope = img->refl_slope;
		satinf->chaninf[i].refl_offset = img->refl_offset;
		if( chaninf->nu_c<=0.0 ) {
			chaninf->nu_c  = 0.01/chaninf->lambda_c;
		}
		printf("name=%s id=%d\n", satinf->chaninf[i].name, satinf->chaninf[i].id);
		printf("cal_slope=%f cal_offset=%f\n", satinf->chaninf[i].cal_slope, satinf->chaninf[i].cal_offset);
		printf("lambda_c=%f\n", satinf->chaninf[i].lambda_c);
		if(i==0) {
			msevi_l15hdf_create_chaninf( meta_gid, "channel_info", chaninf );
		} else {
			msevi_l15hdf_append_chaninf( meta_gid, "channel_info", chaninf );
		}
		msevi_l15_image_free( img );
	}



	/* add geometry */
	true_ss_lon = header->satellite_status.satellite_definition.nominal_longitude;
	proj_ss_lon = header->image_description.projection_description.longitude_of_ssp;
	printf("Sub-Satellite Longitude: true=%.3f proj=%.3f\n", true_ss_lon, proj_ss_lon );

	x0 = -DEG2RAD((double)(popts.coverage.western_column-coff)*65536/cfac);
	dx = DEG2RAD((double)65536/cfac);
	y0 = DEG2RAD((double)(popts.coverage.northern_line-loff)*65536/lfac);
	dy = -DEG2RAD((double)65536/lfac);
	gp = geos_init( x0, y0, dx, dy );

	npix = reg->nlin*reg->ncol;
	lat = calloc(npix, sizeof(float));
	lon = calloc(npix, sizeof(float));
	if( lat==NULL || lon==NULL ) goto err_out;

	muS = calloc(npix, sizeof(float));
	azS = calloc(npix, sizeof(float));
	if( muS==NULL || azS==NULL ) goto err_out;

	mu0 = calloc(npix, sizeof(float));
	az0 = calloc(npix, sizeof(float));
	if( mu0==NULL || az0==NULL ) goto err_out;
	
	/* calculate geolocation and satellite/sun angles */
	geos_latlon2d( gp, proj_ss_lon, reg->nlin, reg->ncol, lat, lon );
	dim[0] = reg->nlin; dim[1] = reg->ncol;

	if( popts.write_geolocation ) {
		r = H5UTmake_dataset( geom_gid, "latitude", 2, dim, H5T_NATIVE_FLOAT, lat, 6 );
		if(r<0) goto err_out;
		r = sdset_annotate( geom_gid, "latitude", "latitude north", "degrees", 1.0, 0.0 );
		if(r<0) goto err_out;
		r = H5UTmake_dataset( geom_gid, "longitude", 2, dim, H5T_NATIVE_FLOAT, lon, 6 );
		if(r<0) goto err_out;
		r = sdset_annotate( geom_gid, "longitude", "longitude east", "degrees", 1.0, 0.0 );
		if(r<0) goto err_out;
	}

	if( popts.write_sat_angles ) {
		uint16_t *cnt_zen = (uint16_t *)((void *)muS);
		uint16_t *cnt_azi = (uint16_t *)((void *)azS);

		geos_satpos2d( gp, true_ss_lon, reg->nlin, reg->ncol, lat, lon, muS, azS );
		for(i=0;i<npix;i++) cnt_zen[i] = (uint16_t) round(RAD2DEG(acosf(muS[i]))*100.0);
		for(i=0;i<npix;i++) cnt_azi[i] = (uint16_t) round(azS[i]*100.0);

		r = H5UTmake_dataset( geom_gid, "satellite_zenith", 2, dim, H5T_NATIVE_UINT16, cnt_zen, 6 );
		if(r<0) goto err_out;
		r = sdset_annotate( geom_gid, "satellite_zenith", "satellite zenith angle", "degrees",
				    0.01, 0.0 );
		if(r<0) goto err_out;
		r = H5UTmake_dataset( geom_gid, "satellite_azimuth", 2, dim, H5T_NATIVE_UINT16, cnt_azi, 6 );
		if(r<0) goto err_out;
		r = sdset_annotate( geom_gid, "satellite_azimuth", "satellite azimuth angle", "degrees",
				    0.01, 0.0 );
		if(r<0) goto err_out;
	}

	if( popts.write_sun_angles ) {
		double dt = -0.2/8.64e+04;
		uint16_t *cnt_zen = (uint16_t *)((void *)muS);
		uint16_t *cnt_azi = (uint16_t *)((void *)azS);

		jd = (double)(line_acq_time[0].days-15340)-0.5 + line_acq_time[0].msec/8.64e+07;
		sunpos2d( jd, dt, reg->nlin, reg->ncol, lat, lon, mu0, az0 );
		for(i=0;i<npix;i++) cnt_zen[i] = (uint16_t) roundf(RAD2DEG(acosf(mu0[i]))*100.0);
		for(i=0;i<npix;i++) cnt_azi[i] = (uint16_t) roundf(az0[i]*100.0);

		r = H5UTmake_dataset( geom_gid, "sun_zenith", 2, dim, H5T_NATIVE_UINT16, cnt_zen, 6 );
		if(r<0) goto err_out;
		r = sdset_annotate( geom_gid, "sun_zenith", "sun zenith angle", "degrees", 0.01, 0.0 );
		if(r<0) goto err_out;
		r = H5UTmake_dataset( geom_gid, "sun_azimuth", 2, dim, H5T_NATIVE_UINT16, cnt_azi, 6 );
		if(r<0) goto err_out;
		r = sdset_annotate( geom_gid, "sun_azimuth", "sun azimuth angle", "degrees", 0.01, 0.0 );
		if(r<0) goto err_out;
	}

	/* close group/file */
	free(lat);
	free(lon);
	free(muS);
	free(azS);

	/* close image group/file */
	printf( "Closing file and exit...\n" );
	H5Gclose( img_gid );
	H5Gclose( lsi_gid );
	H5Gclose( meta_gid );
	H5Gclose( geom_gid );
	H5Fclose( fid );

	/* cleanup */
	msevi_l15hrit_free_flist( flist );
	free(header);
	free(trailer);
	free(line_acq_time);
	free(satinf);

	return  0;

err_out:
	printf("Error\n");
	return -1;
	
}


