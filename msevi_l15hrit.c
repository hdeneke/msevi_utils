/* System includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <endian.h>
#include <math.h>

#include <glob.h>
#include <libgen.h>

/* Local includes */
#include "memcpy_endian.h"
#include "cds_time.h"
#include "mathutils.h"
#include "timeutils.h"
#include "cgms_xrit.h"
#include "msevi_l15.h"
#include "msevi_l15hrit.h"
#include "eum_wavelet.h"
#include "sunpos.h"

/* SEVIRI L15 header record lengths */
/* TODO: ???? re-check, 1 byte for version seems to be missing ??? */
const static size_t header_rec_len[8] = {
	     0,  /* start, version */
	 60134,  /* satellite_status */
	   700,  /* image_acquisition */
	326058,  /* celestial_events */
	   101,  /* image_description */
	 20815,  /* radiometric_processing */
	 17653,  /* geometric_processing */
	 19786,  /* impf_configuration */
};

/* SEVIRI L15 header record offsets, i.e cumulative sums of 
   record length 
   TODO: ???? check, version seems to be missing ???        */
const static size_t header_rec_off[9] = {
	     0, /* start, version */
	     0, /* satellite_status */
	 60134, /* image_acquisition */
	 60834, /* celestial_events */
	386892, /* image_description */
	386993, /* radiometric_processing */
	407808, /* geometric_processing */
        425461, /* impf_configuration */ 
        445247  /* end */
};

/* SEVIRI L15 trailer record lengths */
const static size_t trailer_rec_len[7] = { 
	     1, /* version */
	   340, /* image_production_stats */
	  5680, /* navigation_extraction */
	371256, /* radiometric_quality */
	  2916, /* geometric_quality */
	   132  /* timeliness_and_completeness */
};

/* SEVIRI L15 trailer record offsets i.e cumulative sums of 
   record length */
const static size_t trailer_rec_off[8] = { 
	     0, /* start, version */
	     1, /* image_production_stats */
	   341, /* navigation_extraction */
	  6021, /* radiometric_quality */
	377277, /* geometric_quality */
	380193, /* timeliness_and_completeness */
        390325  /* end */
};


/**
 * \brief  Return a list of SEVIRI L15 HRIT files for one repeat cycle
 *
 * \param[in]  dir    the directory to search in
 * \param[in]  time   the time of the repeat cycle
 *
 * \return     the list of SEVIRI L15 image files
 */
struct msevi_l15hrit_flist *msevi_l15hrit_get_flist( char *dir, time_t *time, char *svc )
{
	int i, r;
	int ichan, iseg;
	struct msevi_l15hrit_flist *flist;
	char *fnam, *bnam, *tnam;
	char timestr[16], pattern[512], chanstr[7], segstr[7];
	glob_t globbuf = {};

	/* allocate memory for return structure */
	flist = calloc(1,sizeof(*flist));
	if(flist==NULL) goto err_out;

	/* match SEVIRI HRIT files */
	snprint_utc_timestr( timestr, 16, "%Y%m%d%H%M", *time );
	if (0==strncasecmp(svc,"pzs",3)) {
		snprintf( pattern, 512, "%s/H-000-MSG*%s*", dir, timestr );
	}else if (0==strncasecmp(svc,"rss",3)) {
		snprintf( pattern, 512, "%s/H-000-MSG*RSS*%s*", dir, timestr );
	} else {
		printf("ERROR: unknown service %s\nExiting\n", svc);
		exit(-1);
	}
	globbuf.gl_offs = 0;
	r = glob( pattern, 0, NULL, &globbuf );

	for( i=0; i<globbuf.gl_pathc; i++ ) {

		/* get basename */
		fnam = strdup( globbuf.gl_pathv[i] );
		tnam = strdup(fnam);
		bnam = basename( tnam );

		/* get channel/segment substrings */
		strncpy( chanstr, bnam+26, 6 );
		chanstr[6]=0;
		strncpy( segstr, bnam+36, 6 );
		segstr[6]=0;
		free(tnam);

		if( strncasecmp(segstr,"PRO",3)==0 ) {
			flist->prologue = fnam;
		} else if( strncasecmp(segstr,"EPI",3)==0 ) {
			flist->epilogue = fnam;
		} else {
			ichan = msevi_chan2id(chanstr);
			iseg = flist->nseg[ichan-1];
			flist->channel[ichan-1][iseg] = fnam;
			flist->nseg[ichan-1]++;
		}
	}

	globfree( &globbuf );
	return flist;

err_out:
	return NULL;
}

/**
 * \brief  Free a list of SEVIRI L15 HRIT files
 *
 * \param[in]  fl     the file list
*
 * \return     nothing
 */
void msevi_l15hrit_free_flist( struct msevi_l15hrit_flist *fl )
{
	int ichan, iseg;
	
	if( fl ) {
		free(fl->prologue);
		free(fl->epilogue);
		for( ichan=0; ichan<MSEVI_NCHAN-1; ichan++ ) {
			for( iseg=0; iseg<fl->nseg[ichan]; iseg++ ) {
				free( fl->channel[ichan][iseg] );
			}
		}
		free( fl );
	}
	return;
}

struct msevi_l15_image *msevi_l15hrit_read_segment( char *fnam )
{
	struct xrit_file *xf;
	void *hdr, *hrec, *data;
	struct xrit_hrec_primary *prim;
	struct xrit_hrec_image_structure  *img_struct;
	struct xrit_hrec_image_navigation *img_nav;
	struct msevi_hrec_segment_identification *seg_id;
	struct msevi_hrec_segment_line_quality *line_qual;
	struct msevi_l15_image *img;

	/* open file and check that it is an image */
	xf  = xrit_fopen(fnam, "rb");
	if( xf==NULL || xf->ftype!=XRIT_FTPYE_IMAGE ) goto err_out;

	/* read relevant header records */
	hdr = xrit_read_header(xf);
	if(hdr==NULL) goto err_out;
	hrec = xrit_find_hrec(hdr, xf->header_len, XRIT_HREC_PRIMARY);
	prim = xrit_decode_hrec(hrec);
	hrec = xrit_find_hrec(hdr, xf->header_len, XRIT_HREC_IMAGE_STRUCTURE);
	img_struct = xrit_decode_hrec(hrec);
	hrec = xrit_find_hrec(hdr, xf->header_len, XRIT_HREC_IMAGE_NAVIGATION);
	img_nav = xrit_decode_hrec(hrec);
	hrec = xrit_find_hrec(hdr, xf->header_len, MSEVI_HREC_SEGMENT_IDENTIFICATION );
	seg_id = msevi_l15_hrit_decode_hrec(hrec);
	hrec = xrit_find_hrec(hdr, xf->header_len, MSEVI_HREC_SEGMENT_LINE_QUALITY);
	line_qual = msevi_l15_hrit_decode_hrec(hrec);

	/* allocate image */
	img = calloc(1,sizeof(*img));
	if(img==NULL) goto err_out;
	
	/* set image information */
	img->nlin = img_struct->nlin;
	img->ncol = img_struct->ncol;
	img->depth = img_struct->bpp;

	if( seg_id->channel_id==12 ) {
		img->coverage.southern_line = 5566-img_nav->loff+1;
		img->coverage.northern_line = img->coverage.southern_line+img->nlin-1;
		img->coverage.eastern_column = 5566-img_nav->coff+1;
		img->coverage.western_column = img->coverage.eastern_column+img->ncol-1;
	}else {
		img->coverage.southern_line = 1856-img_nav->loff+1;
		img->coverage.northern_line = img->coverage.southern_line+img->nlin-1;
		img->coverage.eastern_column = 1856-img_nav->coff+1;
		img->coverage.western_column = img->coverage.eastern_column+img->ncol-1;
	}
	img->channel_id = seg_id->channel_id;
	img->spacecraft_id = seg_id->sat_id;
	img->line_side_info = line_qual->line_side_info;
	// printf("lfac=%d cfac=%d\n", img_nav->lfac, img_nav->cfac );

	/* read and uncompress data */
	data = xrit_read_data(xf);
	if( img_struct->compression>0 ) {
		img->counts = xrit_data_decompress( img->nlin, img->ncol,
						    img->depth, 3, data,
						    xf->data_len );
		/* free(data); ?? Why no free ?? */
	} else {
		img->counts = data;
	}

	/* cleanup and return */
	xrit_fclose(xf);
	free(prim);
	free(img_struct);
	free(img_nav);
	free(seg_id);
	free(line_qual);
	free(hdr);

	return img;

 err_out:
	return NULL;
}

static int map_segment(struct msevi_l15_image *dest, struct msevi_l15_image *src)
{
	int il, ic, nlin, ncol;
	int south_lin, north_lin, east_col, west_col;
	uint16_t *csrc, *cdest;

	south_lin = MAX(dest->coverage.southern_line, src->coverage.southern_line);
	north_lin = MIN(dest->coverage.northern_line, src->coverage.northern_line);
	east_col  = MAX(dest->coverage.eastern_column, src->coverage.eastern_column);
	west_col  = MIN(dest->coverage.western_column, src->coverage.western_column);

	nlin = north_lin-south_lin+1;
	ncol = west_col-east_col+1;
	if(nlin<0||ncol<0) return 0; /* no overlap of regions */

	for(il=0;il<nlin;il++) {
		int loff_dest, loff_src;

		/* calculate line offsets */
		loff_dest = dest->coverage.northern_line-north_lin+il;
		loff_src  = south_lin-src->coverage.southern_line+nlin-il-1;
		
		/* copy line side information */
		memcpy( dest->line_side_info+loff_dest, src->line_side_info+loff_src, 
		        sizeof(struct msevi_l15_line_side_info) );

		/* get data pointers, add column offsets */
		cdest  = dest->counts + loff_dest*dest->ncol;
		csrc   = src->counts  + loff_src*src->ncol;
		cdest +=  dest->coverage.western_column-west_col;
		csrc  +=  east_col-src->coverage.eastern_column+(ncol-1);

		/* and write counts */
		for(ic=0;ic<ncol;ic++) 	*cdest++ = *csrc--;
	}
	if( dest->spacecraft_id == 0 ) {
		dest->spacecraft_id = src->spacecraft_id;
		dest->channel_id = src->channel_id;
	}
	return nlin;
}


int msevi_l15hrit_get_segment_coverage( char *fnam, struct msevi_l15_coverage *cov )
{
	struct xrit_file *xf;
	void *hdr, *hrec;
	struct xrit_hrec_image_structure  *img_struct;
	struct xrit_hrec_image_navigation *img_nav;
	struct msevi_hrec_segment_identification *seg_id;

	/* open file and check that it is an image */
	xf  = xrit_fopen(fnam, "rb");
	if( xf==NULL || xf->ftype!=XRIT_FTPYE_IMAGE ) goto err_out;

	/* read relevant header records */
	hdr = xrit_read_header( xf );
	if(hdr==NULL) goto err_out;

	hrec = xrit_find_hrec(hdr, xf->header_len, XRIT_HREC_IMAGE_STRUCTURE);
	img_struct = xrit_decode_hrec(hrec);
	hrec = xrit_find_hrec(hdr, xf->header_len, XRIT_HREC_IMAGE_NAVIGATION);
	img_nav = xrit_decode_hrec(hrec);
	hrec = xrit_find_hrec(hdr, xf->header_len, MSEVI_HREC_SEGMENT_IDENTIFICATION );
	seg_id = msevi_l15_hrit_decode_hrec(hrec);

	if(seg_id->channel_id==12) { /* special-case HRV */
		cov->southern_line  = 5566-img_nav->loff+1;
		cov->northern_line  = cov->southern_line+img_struct->nlin-1;
		cov->eastern_column = 5566-img_nav->coff+1;
		cov->western_column = cov->eastern_column+img_struct->ncol-1;
	} else {
		cov->southern_line  = 1856-img_nav->loff+1;
		cov->northern_line  = cov->southern_line+img_struct->nlin-1;
		cov->eastern_column = 1856-img_nav->coff+1;
		cov->western_column = cov->eastern_column+img_struct->ncol-1;
	}

	return 0;

err_out:
	return -1;
}

static int coverage_overlaps ( struct msevi_l15_coverage *c1,
			       struct msevi_l15_coverage *c2 )
{
	if(     c1->southern_line  > c2->northern_line
	     || c1->northern_line  < c2->southern_line 
	     || c1->eastern_column > c2->western_column
             || c1->western_column < c2->eastern_column )
		return 0;
	return 1;
}


struct msevi_l15_image *msevi_l15hrit_read_image( int nfile, char **files, 
						  struct msevi_l15_coverage *cov )

{
	int i, r;
	struct msevi_l15_image   *img = NULL, *seg;
	int nlin, ncol;

	/* allocate memory for image */
	if( cov != NULL ) {
		nlin = cov->northern_line-cov->southern_line+1;
		ncol = cov->western_column-cov->eastern_column+1;
	} else {
		nlin = 3712;
		ncol = 3712;
	}
	img = msevi_l15_image_alloc( nlin, ncol );
	if(img==NULL) goto err_out;
	memcpy( &img->coverage, cov, sizeof(struct msevi_l15_coverage) );

	/* read segments */
	for (i=0; i<nfile; i++) {
		struct msevi_l15_coverage seg_cov;
		r = msevi_l15hrit_get_segment_coverage( files[i], &seg_cov );
		if( coverage_overlaps(cov, &seg_cov) ) {
			printf("Reading: %s\n", files[i] );
			seg = msevi_l15hrit_read_segment( files[i] );
			if(seg==NULL) goto err_out;

			map_segment(img, seg);
			msevi_l15_image_free( seg );
		} else {
			// printf("Skipping: %s\n", files[i] );
		}
	}
	return img;

err_out:
	msevi_l15_image_free(img);
	return NULL;
}


void *msevi_l15_hrit_decode_hrec( void *hrec )
{
	uint8_t  hrec_type;
	uint16_t hrec_len;
	
	memcpy(&hrec_type, hrec, 1);
	memcpy_be16toh(&hrec_len, hrec+1, 1);

	switch (hrec_type) {
	case  MSEVI_HREC_SEGMENT_IDENTIFICATION: {
		struct msevi_hrec_segment_identification *si;

		si = calloc(1,sizeof(*si));
		if(si==NULL) goto err_out;
		
		si->hrec_type = hrec_type;
		si->hrec_len  = hrec_len;
		memcpy(&si->sat_id, hrec+3, 2);
		memcpy(&si->channel_id, hrec+5, 1);
		memcpy_be16toh(&si->segm_seq_nr, hrec+6, 1);
		memcpy_be16toh(&si->planned_start_segm_seq_nr, hrec+8, 1);
		memcpy_be16toh(&si->planned_end_segm_seq_nr, hrec+10, 1);
		memcpy(&si->data_field_representation, hrec+12, 1);

		return si;
	}
	case MSEVI_HREC_SEGMENT_LINE_QUALITY: {
		struct msevi_hrec_segment_line_quality *lq;
		int i, nlin;
		void *entry = hrec+3;

		lq = calloc(1,sizeof(*lq));
		if(lq==NULL) goto err_out;

		lq->hrec_type = hrec_type;
		lq->hrec_len  = hrec_len;
		nlin = (hrec_len-3)/13;
		lq->line_side_info = calloc(nlin, sizeof(*lq->line_side_info));

		for(i=0; i<nlin; i++) {
			memcpy_be32toh(&lq->line_side_info[i].nr_in_grid, entry, 1);
			memcpy_be16toh(&lq->line_side_info[i].acquisition_time.days, entry+4, 1);
			memcpy_be32toh(&lq->line_side_info[i].acquisition_time.msec, entry+6, 1);
			memcpy(&lq->line_side_info[i].validity,            entry+10, 1);
			memcpy(&lq->line_side_info[i].radiometric_quality, entry+11, 1);
			memcpy(&lq->line_side_info[i].geometric_quality,   entry+12, 1);
			entry += 13;
		}
		return lq;
	} }
err_out:
	return NULL;
}

struct msevi_l15_header *msevi_l15hrit_read_prologue( char *file )
{
	
	int i,j;
	struct msevi_l15_header *header;
	struct xrit_file *pro;
	void *data;
	void *rec_ptr;

	/* allocate structure */
	header = calloc( 1, sizeof(*header) );
	if( header==NULL ) goto err_out;

	pro = xrit_fopen( file, "rb" );
	if( pro->ftype!=MSEVI_L15HRIT_PROLOGUE ) {
		fprintf( stderr, "ERROR: %s not a SEVIRI prologue file\n", file );
		goto err_out;
	}

	data = xrit_read_data( pro );
	if(data==NULL) goto err_out;

	// printf("Satellite status rec\n");
	{  /* get satellite status record */
		{ /* get satellite definition */
			struct _satellite_definition *sd = 
				&header->satellite_status.satellite_definition;
			rec_ptr = data+header_rec_off[1];

			memcpy_be16toh( &sd->satellite_id, rec_ptr, 1 );
			memcpy_be32toh( &sd->nominal_longitude, rec_ptr+2, 1 );
			memcpy( &sd->satellite_status, rec_ptr+6, 1 );
		}
		{ /* get satellite orbit */
			struct _orbit *orb = &header->satellite_status.orbit;
			rec_ptr = data+header_rec_off[1]+7+28;
			memcpy_be16toh( &orb->period_start_time.days,  rec_ptr,   1 );
			memcpy_be32toh( &orb->period_start_time.msec,  rec_ptr+2, 1 );
			memcpy_be16toh( &orb->period_end_time.days,    rec_ptr+6, 1 );
			memcpy_be32toh( &orb->period_end_time.msec,    rec_ptr+8, 1 );
			
			rec_ptr = data+header_rec_off[1]+7+28+12;
			for( i=0; i<100; i++ ){
				memcpy_be16toh( &orb->orbitcoef[i].start_time.days,  rec_ptr, 1);
				memcpy_be32toh( &orb->orbitcoef[i].start_time.msec, rec_ptr+2, 1);
				memcpy_be16toh( &orb->orbitcoef[i].end_time.days,  rec_ptr+6, 1);
				memcpy_be32toh( &orb->orbitcoef[i].end_time.msec, rec_ptr+8, 1);
				for( j=0; j<8; j++ ){
					memcpy_be64toh( &orb->orbitcoef[i].x,  rec_ptr+ 12, 8 );
					memcpy_be64toh( &orb->orbitcoef[i].y,  rec_ptr+ 76, 8 );
					memcpy_be64toh( &orb->orbitcoef[i].z,  rec_ptr+140, 8 );
					memcpy_be64toh( &orb->orbitcoef[i].vx, rec_ptr+204, 8 );
					memcpy_be64toh( &orb->orbitcoef[i].vy, rec_ptr+268, 8 );
					memcpy_be64toh( &orb->orbitcoef[i].vz, rec_ptr+332, 8 );
				}
				rec_ptr += 396;
			}
		}
	}

	{ /* get image acquisition record */
		{ /* planned acquisition time */
			struct _planned_acquisition_time *pat = 
				&header->image_acquisition.planned_acquisition_time;

			rec_ptr = data + header_rec_off[2];
			memcpy_be16toh( &pat->true_repeat_cycle_start.days,  rec_ptr+0, 1);
			memcpy_be32toh( &pat->true_repeat_cycle_start.msec, rec_ptr+2, 1);
			memcpy_be16toh( &pat->planned_fwd_scan_end.days,  rec_ptr+10, 1);
			memcpy_be32toh( &pat->planned_fwd_scan_end.msec, rec_ptr+12, 1);
			memcpy_be16toh( &pat->planned_repeat_cylce_end.days,  rec_ptr+20, 1);
			memcpy_be32toh( &pat->planned_repeat_cylce_end.msec, rec_ptr+22, 1);
		}
	}      

	{ /* get image description record */
		{  /* type of projection */
			struct _projection_description *pd = 
				&header->image_description.projection_description;
		
			rec_ptr = data+header_rec_off[4];
			memcpy( &pd->type_of_projection, rec_ptr, 1 );
			memcpy_be32toh( &pd->longitude_of_ssp, rec_ptr+1, 1);
		} { /* reference grid */
			
			struct _reference_grid *rg = 
				&header->image_description.reference_grid_vis_ir;
			rec_ptr = data+header_rec_off[4]+5;
			
			memcpy_be32toh( &rg->number_of_lines, rec_ptr, 1 );
			memcpy_be32toh( &rg->number_of_columns, rec_ptr+4, 1 );
			memcpy_be32toh( &rg->line_dir_grid_step, rec_ptr+8, 1 );
			memcpy_be32toh( &rg->column_dir_grid_step, rec_ptr+12, 1 );
			memcpy( &rg->grid_origin, rec_ptr+16, 1 );
			/* printf("nlin=%d ncol=%d lstep=%f cstep=%f origin=%d\n",
			       rg->number_of_lines, rg->number_of_columns,
			       rg->line_dir_grid_step,  rg->column_dir_grid_step,
			       rg->grid_origin ); */

			rg = &header->image_description.reference_grid_hrv;
			rec_ptr = data+header_rec_off[4]+22;

			memcpy_be32toh( &rg->number_of_lines, rec_ptr, 1 );
			memcpy_be32toh( &rg->number_of_columns, rec_ptr+4, 1 );
			memcpy_be32toh( &rg->line_dir_grid_step, rec_ptr+8, 1 );
			memcpy_be32toh( &rg->column_dir_grid_step, rec_ptr+12, 1 );
			memcpy( &rg->grid_origin, rec_ptr+16, 1 );
			/* printf("nlin=%d ncol=%d lstep=%f cstep=%f origin=%d\n", 
			       rg->number_of_lines, rg->number_of_columns, 
			       rg->line_dir_grid_step,  rg->column_dir_grid_step, 
			       rg->grid_origin ); */
		} { /* planned coverage */
			struct msevi_l15_coverage *cov;

			/* planned VIS/IR coverage */
			cov = &header->image_description.planned_coverage_vis_ir;
			rec_ptr = data+header_rec_off[4]+39;
			memcpy_be32toh( &cov->southern_line, rec_ptr, 1 );
			memcpy_be32toh( &cov->northern_line, rec_ptr+4, 1 );
			memcpy_be32toh( &cov->eastern_column, rec_ptr+8, 1 );
			memcpy_be32toh( &cov->western_column, rec_ptr+12, 1 );

			/* planned lower HRV coverage */
			cov =  &header->image_description.planned_coverage_hrv_lower;
			rec_ptr = data+header_rec_off[4]+55;
			memcpy_be32toh( &cov->southern_line, rec_ptr, 1 );
			memcpy_be32toh( &cov->northern_line, rec_ptr+4, 1 );
			memcpy_be32toh( &cov->eastern_column, rec_ptr+8, 1 );
			memcpy_be32toh( &cov->western_column, rec_ptr+12, 1 );
			
			/* planned upper HRV coverage */
			cov =  &header->image_description.planned_coverage_hrv_upper;
			rec_ptr = data+header_rec_off[4]+71;
			memcpy_be32toh( &cov->southern_line, rec_ptr, 1 );
			memcpy_be32toh( &cov->northern_line, rec_ptr+4, 1 );
			memcpy_be32toh( &cov->eastern_column, rec_ptr+8, 1 );
			memcpy_be32toh( &cov->western_column, rec_ptr+12, 1 );
		} { /* l15 image production */
			struct _l15_image_production *ip = &header->image_description.l15_image_production;
			rec_ptr = data+header_rec_off[4]+87;
			memcpy( &ip->image_proc_direction, rec_ptr, 1 );
			memcpy( &ip->pixel_gen_direction, rec_ptr+1, 1 );
			memcpy( &ip->planned_chan_processing, rec_ptr+2, 12 );
		}
	}

	{ /* get radiometeric_processing record */
		{ /* L15 image calibration */
			struct _l15_image_calibration *cal = header->radiometric_processing.l15_image_calibration;
			rec_ptr = data+header_rec_off[5]+72;

			for( i=0; i<MSEVI_NCHAN; i++ ) {
				memcpy_be64toh(&cal->cal_slope, rec_ptr+(2*i)*sizeof(double), 1);
				memcpy_be64toh(&cal->cal_offset, rec_ptr+(2*i+1)*sizeof(double), 1);
				cal++;
			}
		}
	}

	{ /* get geometric processing record */
		{ /* Optical Axis Distance */
			int i;
			struct _opt_axis_distance *oad = &header->geometric_processing.opt_axis_distance;
			rec_ptr = data+header_rec_off[6];
			for( i=0; i<MSEVI_NR_CHAN; i++ ) memcpy_be32toh(oad->ew_focal_plane+i, rec_ptr+i*sizeof(float), 1);
			for( i=0; i<MSEVI_NR_CHAN; i++ ) memcpy_be32toh(oad->ns_focal_plane+i, rec_ptr+(42+i)*sizeof(float), 1);
		}
		{ /* Earth model */
			struct _earth_model *em= &header->geometric_processing.earth_model;
			rec_ptr = data+header_rec_off[6]+42*2*sizeof(float);
			memcpy( &em->type, rec_ptr, 1 );
			memcpy_be64toh( &em->equatorial_radius, rec_ptr+1, 1);
			memcpy_be64toh( &em->north_polar_radius, rec_ptr+9, 1);
			memcpy_be64toh( &em->south_polar_radius, rec_ptr+17, 1);
		}
	}
	xrit_fclose( pro );
	free( data );
	return header;

err_out:
	return NULL;
}

struct msevi_l15_trailer *msevi_l15hrit_read_epilogue( char *file )
{
	struct msevi_l15_trailer *trailer;
	struct xrit_file *epi = NULL;
	void *data = NULL;
	void *rec_ptr;

	/* allocate structure */
	trailer = calloc(1,sizeof(*trailer));
	if( trailer==NULL ) goto err_out;

	epi = xrit_fopen( file, "rb" );
	if( epi==NULL || epi->ftype != MSEVI_L15HRIT_EPILOGUE ) {
		fprintf( stderr, "ERROR: %s not a SEVIRI epilogue file\n", file );
		goto err_out;
	}

	/* read data section */
	data = xrit_read_data( epi );
	if(data==NULL) goto err_out;

	/* get version */
	rec_ptr = data;
	memcpy( &trailer->version, rec_ptr, 1);
	
	{ /* image production stats */

		{ /* satellite id */
			struct _image_production_stats *ips = &trailer->image_production_stats;
			rec_ptr = data+1;
			memcpy_be16toh( &ips->satellite_id, rec_ptr, 1);
		} { /* actual scanning summary */
			struct _actual_scanning_summary *ass;

			ass = &trailer->image_production_stats.actual_scanning_summary;
			rec_ptr = data+3;

			memcpy( &ass->nominal_image_scanning, rec_ptr, 1);
			memcpy( &ass->reduced_scan, rec_ptr+1, 1);
			memcpy_be16toh( &ass->forward_scan_start.days, rec_ptr+2, 1);
			memcpy_be32toh( &ass->forward_scan_start.msec, rec_ptr+4, 1);
			memcpy_be16toh( &ass->forward_scan_end.days, rec_ptr+ 8, 1);
			memcpy_be32toh( &ass->forward_scan_end.msec, rec_ptr+10, 1);
			
		} { /* reception summary stats */
			int i;
			struct _reception_summary_stats *rss = &trailer->image_production_stats.reception_summary_stats;
			rec_ptr = data+29;

			for(i=0; i<MSEVI_NR_CHAN; i++) {
				memcpy_be32toh( rss->planned_number_of_l10_lines+i, rec_ptr+i*sizeof(uint32_t), 1);
				memcpy_be32toh( rss->number_of_missing_l10_lines+i, rec_ptr+(12+i)*sizeof(uint32_t), 1);
				memcpy_be32toh( rss->number_of_corrupted_l10_lines+i, rec_ptr+(24+i)*sizeof(uint32_t), 1);
				memcpy_be32toh( rss->number_of_replaced_l10_lines+i, rec_ptr+(36+i)*sizeof(uint32_t), 1);
			}
		} { /* l15 image validity */
			int i;
			struct _l15_image_validity *liv;

			liv = trailer->image_production_stats.l15_image_validity;
			rec_ptr = data+221;

			for(i=0;i<MSEVI_NR_CHAN; i++) {
				memcpy( liv, rec_ptr, 6);
				liv++;
				rec_ptr += 6;
			}
		} { /* coverage */
			struct msevi_l15_coverage *cov;

			cov = &trailer->image_production_stats.actual_coverage_vis_ir;
			rec_ptr = data+293;

			memcpy_be32toh( &cov->southern_line,  rec_ptr, 1 );
			memcpy_be32toh( &cov->northern_line,  rec_ptr+1*sizeof(uint32_t), 1 );
			memcpy_be32toh( &cov->eastern_column, rec_ptr+2*sizeof(uint32_t), 1 );
			memcpy_be32toh( &cov->western_column, rec_ptr+3*sizeof(uint32_t), 1 );
			
			cov = &trailer->image_production_stats.actual_coverage_lower_hrv;
			rec_ptr += 4*sizeof(uint32_t);
			memcpy_be32toh( &cov->southern_line,  rec_ptr, 1 );
			memcpy_be32toh( &cov->northern_line,  rec_ptr+1*sizeof(uint32_t), 1 );
			memcpy_be32toh( &cov->eastern_column, rec_ptr+2*sizeof(uint32_t), 1 );
			memcpy_be32toh( &cov->western_column, rec_ptr+3*sizeof(uint32_t), 1 );

			cov = &trailer->image_production_stats.actual_coverage_upper_hrv;
			rec_ptr += 4*sizeof(uint32_t);
			memcpy_be32toh( &cov->southern_line,  rec_ptr, 1 );
			memcpy_be32toh( &cov->northern_line,  rec_ptr+1*sizeof(uint32_t), 1 );
			memcpy_be32toh( &cov->eastern_column, rec_ptr+2*sizeof(uint32_t), 1 );
			memcpy_be32toh( &cov->western_column, rec_ptr+3*sizeof(uint32_t), 1);
		}
		
	}

	/* cleanup and return */
	xrit_fclose( epi );
	return trailer;

err_out:
	xrit_fclose( epi );
	free(data);
	return NULL;
}

int msevi_l15_fprintf_header( FILE *f, struct msevi_l15_header *hdr )
{
	{   /* satellite definition */
		struct _satellite_definition *sd = &hdr->satellite_status.satellite_definition;
		fprintf(f, "status: sat_id=%d ssp_lon=%f status=%d\n",
			sd->satellite_id, sd->nominal_longitude, sd->satellite_status );
	} { /* type of projection */
		struct _projection_description *pd = &hdr->image_description.projection_description;
		fprintf(f, "projection=%d ssp=%f\n", pd->type_of_projection, pd->longitude_of_ssp);
	}

	return 0;
}

int msevi_l15_fprintf_trailer( FILE *f, struct msevi_l15_trailer *tr )
{
	{   /* version */

		fprintf(f, "version=%d\n", tr->version );

	} { /* image production stats */
		struct _image_production_stats *ips = &tr->image_production_stats;

		fprintf(f, "sat_id=%d\n", ips->satellite_id );

	} { /* actual scanning summary */
		struct _actual_scanning_summary *ass;

		ass = &tr->image_production_stats.actual_scanning_summary;
		fprintf(f, "nominal_image_scanning=%d\n", ass->nominal_image_scanning );
		fprintf(f, "reduced_scan=%d\n", ass->reduced_scan );
	} { /* reception summary stats */
		int i;
		struct _reception_summary_stats *rss; 

		rss = &tr->image_production_stats.reception_summary_stats;
		for(i=0; i<MSEVI_NR_CHAN; i++) {
			fprintf(f, "chan=%d planned_lines=%d missing_lines=%d corrupted_lines=%d replaced_line=%d\n", 
				i, rss->planned_number_of_l10_lines[i], rss->number_of_missing_l10_lines[i],
				rss->number_of_corrupted_l10_lines[i], rss->number_of_replaced_l10_lines[i] );
		}
	} { /* l15 image validity */
		int i;
		struct _l15_image_validity *liv;

		liv = tr->image_production_stats.l15_image_validity;
		for(i=0;i<MSEVI_NR_CHAN; i++) {
			fprintf(f, "nominal=%d incomplete=%d radiometric=%d geometric=%d timeliness=%d"
				" incomplete_l15=%d\n", liv->nominal_image, liv->non_nominal_because_incomplete,
				liv->non_nominal_radiometric_quality, liv->non_nominal_geometric_quality,
				liv->non_nominal_timeliness, liv->non_nominal_incomplete_l15 );
		}
	} { /* coverage */
		struct msevi_l15_coverage *cov;

		cov = &tr->image_production_stats.actual_coverage_vis_ir;
		fprintf(f, "coverage_actual_vis_ir: sl=%d nl=%d ec=%d wc=%d\n",
			cov->southern_line, cov->northern_line, cov->eastern_column, cov->western_column );

		cov = &tr->image_production_stats.actual_coverage_lower_hrv;
		fprintf(f, "coverage_actual_lower_hrv: sl=%d nl=%d ec=%d wc=%d\n",
			cov->southern_line, cov->northern_line, cov->eastern_column, cov->western_column );

		cov = &tr->image_production_stats.actual_coverage_upper_hrv;
		fprintf(f, "coverage_actual_lower_hrv: sl=%d nl=%d ec=%d wc=%d\n",
			cov->southern_line, cov->northern_line, cov->eastern_column, cov->western_column );
	}
	return 0;
}

int msevi_l15hrit_annotate_image( struct msevi_l15_image   *img,
				  struct msevi_l15_header  *hdr,
				  struct msevi_l15_trailer *tra,
				  struct msevi_chaninf *chaninf )
{
	double esd, jd;

	/* set slope/offset */
	img->cal_slope  = hdr->radiometric_processing.l15_image_calibration[img->channel_id-1].cal_slope;
	img->cal_offset = hdr->radiometric_processing.l15_image_calibration[img->channel_id-1].cal_offset;

	/* set satellite id constant */
	img->spacecraft_id = hdr->satellite_status.satellite_definition.satellite_id;

	/* channel attributes */
	img->f0        = chaninf->f0;
	img->lambda_c  = chaninf->lambda_c;
	if( chaninf->nu_c>0.0 ) {
		img->nu_c      = chaninf->nu_c;
	} else {
		img->nu_c = 0.01/img->lambda_c;
	}
	img->alpha     = chaninf->alpha;
	img->beta      = chaninf->beta;
	if (img->f0>0) {
		jd = time_cds2jday( &hdr->image_acquisition.planned_acquisition_time.true_repeat_cycle_start, EPOCH_TAI );
		esd = sun_earth_distance(jd);
		img->refl_slope = img->cal_slope * M_PI * SQR(esd)/img->f0;
		img->refl_offset = img->cal_offset * M_PI * SQR(esd)/img->f0;
	} else {
		img->refl_slope   = 0.0;
		img->refl_offset  = 0.0;
	}
	return 0;
}
