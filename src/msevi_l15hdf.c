/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>

#include <hdf5.h>
#include <hdf5_hl.h>

#include "timeutils.h"
#include "cds_time.h"
#include "h5utils.h"
#include "msevi_l15.h"
#include "msevi_l15hrit.h"

/* group names */
const char *msevi_l15hdf_img_grp  = "l15_images";
const char *msevi_l15hdf_meta_grp = "meta";
const char *msevi_l15hdf_lsi_grp  = "line_side_info";


/* write MSG SEIVIR image */
int msevi_l15hdf_write_image( hid_t gid, struct msevi_l15_image *img )
{
	hsize_t dim[2] = { img->nlin, img->ncol };
	char dset[32], long_name[64];
	int r;

	/* create dataset name */
	snprintf(dset, 32, "image_%s", msevi_id2chan(img->channel_id) );

	/* write dataset */
	r = H5UTmake_dataset( gid, dset, 2, dim, H5T_NATIVE_UINT16, img->counts, 6 );
	if(r<0) goto err_out;

	/* add attributes */
	r = H5LTset_attribute_double(gid, dset, "cal_slope", &img->cal_slope, 1);
	r = H5LTset_attribute_double(gid, dset, "cal_offset", &img->cal_offset, 1);
	r = H5LTset_attribute_ushort(gid, dset, "channel_id", &img->channel_id, 1);
	r = H5LTset_attribute_string(gid, dset, "units",  "mWm-2sr-1(cm-1)-1" );
	snprintf( long_name, 64, "toa_spectral_radiance_%s",
		  msevi_id2chan(img->channel_id) );
	r = H5LTset_attribute_string(gid, dset, "long_name",  long_name );
	r = H5LTset_attribute_double(gid, dset, "lambda_c", &img->lambda_c, 1);
	if( img->channel_id>=MSEVI_CHAN_IR_039 && img->channel_id<=MSEVI_CHAN_IR_134 ) {
		/* thermal channels */
		r = H5LTset_attribute_double(gid, dset, "nu_c", &img->nu_c, 1);
		r = H5LTset_attribute_double(gid, dset, "alpha", &img->alpha, 1);
		r = H5LTset_attribute_double(gid, dset, "beta", &img->beta, 1);
	}
	if( img->channel_id<=MSEVI_CHAN_IR_039 || img->channel_id==MSEVI_CHAN_HRV ) {
		r = H5LTset_attribute_double(gid, dset, "f0", &img->f0, 1);
		r = H5LTset_attribute_double(gid, dset, "refl_offset", &img->refl_offset, 1);
		r = H5LTset_attribute_double(gid, dset, "refl_slope", &img->refl_slope, 1);
	}

	/*  add HDF image attributes */
	r = H5LTset_attribute_string(gid, dset, "CLASS", "IMAGE" );
	r = H5LTset_attribute_string(gid, dset, "IMAGE_SUBCLASS", "IMAGE_GRAYSCALE" );
	r = H5LTset_attribute_string(gid, dset, "IMAGE_VERSION", "1.2" );

	return 0;

err_out:
	return -1;
}

/* read MSG SEIVIR image */
struct msevi_l15_image *msevi_l15hdf_read_image( hid_t fid, int chan_id )
{
	struct msevi_l15_image *img;
	char dset[32];
	int r, ndim=2, dim[2];
	hid_t gid_img, gid_lsi;

	/* get name of channel dataset and open groups */
	snprintf( dset, 32, "image_%s", msevi_id2chan(chan_id) );
	gid_img = H5Gopen2( fid, msevi_l15hdf_img_grp, H5P_DEFAULT );
	gid_lsi = H5Gopen2( fid, "/meta/line_side_info", H5P_DEFAULT );
	if( gid_img<0 || gid_lsi<0 ) goto err_out;

	/* find field dimensions */
	r = H5UTdataset_get_info( gid_img, dset, &ndim, dim, NULL );
	if(r<0) goto err_out;

	/* allocate memory */
	img = msevi_l15_image_alloc( dim[0], dim[1] );
	if(img==NULL) goto err_out;

	/* read data from HDF file */
	r = H5UTread_dataset ( gid_img, dset, img->counts, NULL, NULL);
	if (r<0)  goto err_out;

	/* read attributes */
	r = H5LTget_attribute_double ( gid_img, dset, "cal_slope", &img->cal_slope );
	if (r<0)  goto err_out;
	r = H5LTget_attribute_double ( gid_img, dset, "cal_offset", &img->cal_offset );
	if (r<0)  goto err_out;
	r = H5LTget_attribute_ushort ( gid_img, dset, "channel_id", &img->channel_id );
	if (r<0)  goto err_out;
	if( chan_id<=MSEVI_CHAN_IR_039 || chan_id==MSEVI_CHAN_HRV) { /* solar channels */
		r = H5LTget_attribute_double ( gid_img, dset, "f0", &img->f0 );
		if (r<0)  goto err_out;
		r = H5LTget_attribute_double ( gid_img, dset, "refl_slope", &img->refl_slope );
		if (r<0)  goto err_out;
		r = H5LTget_attribute_double ( gid_img, dset, "refl_offset", &img->refl_offset );
		if (r<0)  goto err_out;
	}
	if( chan_id>=MSEVI_CHAN_IR_039 && chan_id<=MSEVI_CHAN_IR_134 ) { /* thermal channels */
		r = H5LTget_attribute_double ( gid_img, dset, "alpha", &img->alpha );
		if (r<0)  goto err_out;
		r = H5LTget_attribute_double ( gid_img, dset, "beta", &img->beta );
		if (r<0)  goto err_out;
		r = H5LTget_attribute_double ( gid_img, dset, "nu_c", &img->nu_c );
		if (r<0)  goto err_out;
	}
	return img;

err_out:
	return NULL;
}

/* line side info */
const static size_t lsi_size = sizeof( struct msevi_l15_line_side_info );

const static size_t lsi_offsets[6] = {
	offsetof( struct msevi_l15_line_side_info, nr_in_grid ),
	offsetof( struct msevi_l15_line_side_info, acquisition_time) + offsetof(struct cds_time, days),
	offsetof( struct msevi_l15_line_side_info, acquisition_time) + offsetof(struct cds_time, msec),
	offsetof( struct msevi_l15_line_side_info, validity ),
	offsetof( struct msevi_l15_line_side_info, radiometric_quality ),
	offsetof( struct msevi_l15_line_side_info, geometric_quality )
};

const static size_t lsi_membsize[6] = {
	sizeof( uint32_t ),
	sizeof( uint16_t ),
	sizeof( uint32_t ),
	sizeof( uint8_t ),
	sizeof( uint8_t ),
	sizeof( uint8_t ),
};

const static char *lsi_names[6] = { "line_nr_in_grid",	"days", "msec", "validity",
			     "radiometric_quality", "geometric_quality" };

int msevi_l15hdf_write_line_side_info( hid_t lsi_gid, struct msevi_l15_image *img )
{
	hid_t lsi_types[6] = { H5T_NATIVE_INT32, H5T_NATIVE_UINT16, H5T_NATIVE_UINT32,
			       H5T_NATIVE_UINT8, H5T_NATIVE_UINT8, H5T_NATIVE_UINT8 };
	char tab_nam[32];

	sprintf( tab_nam, "line_side_info_%s", msevi_id2chan(img->channel_id) );
	H5TBmake_table( tab_nam, lsi_gid, tab_nam, 6, img->nlin, lsi_size,
			lsi_names, lsi_offsets, lsi_types,
			64, NULL, 1, (void *)img->line_side_info );
	return 0;
}

int msevi_l15hdf_read_line_side_info( hid_t lsi_gid, struct msevi_l15_image *img )
{
	int r;
	char tab_nam[32];
	hsize_t nrec, nfield;

	sprintf( tab_nam, "line_side_info_%s", msevi_id2chan(img->channel_id) );
	r = H5TBget_table_info( lsi_gid, tab_nam, &nrec, &nfield );
	if(r<0) goto err_out;
	r = H5TBread_records( lsi_gid, tab_nam, 0, nrec, lsi_size, lsi_offsets,
			      lsi_membsize, img->line_side_info );
	if(r<0) goto err_out;
	return 0;
err_out:
	return -1;
}

const static size_t cov_size     = sizeof(struct msevi_l15_coverage);

const static size_t cov_offsets[5]   = {
	offsetof(struct msevi_l15_coverage, channel),
	offsetof(struct msevi_l15_coverage, southern_line),
	offsetof(struct msevi_l15_coverage, northern_line),
	offsetof(struct msevi_l15_coverage, eastern_column),
	offsetof(struct msevi_l15_coverage, western_column)
};
const static size_t cov_membsize[5]  = { 8, sizeof(uint32_t), sizeof(uint32_t), sizeof(uint32_t), sizeof(uint32_t) };
const char *cov_names[5] = { "channel", "southern_line", "northern_line", "eastern_column", "western_column" };


int msevi_l15hdf_write_coverage( hid_t hid, char *name, struct msevi_l15_coverage *cov )
{
	int r;
	hid_t cov_types[5] = { 0, H5T_NATIVE_UINT32, H5T_NATIVE_UINT32, H5T_NATIVE_UINT32,
			       H5T_NATIVE_UINT32 };
	hid_t strtype;

	strtype = H5Tcopy( H5T_C_S1 );
	H5Tset_size( strtype, 8 );
	cov_types[0] = strtype;
	r = H5TBmake_table( "msevi_l15_coverage", hid, name, 5, 1, cov_size, cov_names,
			    cov_offsets, cov_types, 1, NULL, 0, cov );
	H5Tclose( strtype);
	return r;
}

int msevi_l15hdf_append_coverage( hid_t hid, char *name, struct msevi_l15_coverage *cov )
{
	int r;
	hid_t cov_types[5] = { 0, H5T_NATIVE_UINT32, H5T_NATIVE_UINT32, H5T_NATIVE_UINT32,
			       H5T_NATIVE_UINT32 };
	hid_t strtype;

	strtype = H5Tcopy( H5T_C_S1 );
	H5Tset_size( strtype, 8 );
	cov_types[0] = strtype;
	r = H5TBappend_records( hid, name, 1, cov_size, cov_offsets, cov_membsize, cov );
	H5Tclose( strtype);
	return r;
}

struct msevi_l15_coverage *msevi_l15hdf_read_coverage( hid_t gid, char *name, char *chan )
{
	int i, r;
	hsize_t nrec, nf;
	struct msevi_l15_coverage *cov;

	/* allocate return value */
	cov = calloc(1,sizeof(struct msevi_l15_coverage));
	if(cov==NULL) goto err_out;
	/* get number of records */
	r = H5TBget_table_info( gid, name, &nf, &nrec);
	if(r<0) goto err_out;
	for(i=0;i<nrec;i++) {
		r = H5TBread_records( gid, name, 0, 1, cov_size, cov_offsets,
				      cov_membsize, cov );
		if(strncmp(cov->channel,chan,8)==0) break;
	}

err_out:
	return cov;
}

const static size_t chaninf_size     = sizeof(struct msevi_chaninf);

const static size_t chaninf_offsets[11]   = {
	offsetof(struct msevi_chaninf, name),
	offsetof(struct msevi_chaninf, id),
	offsetof(struct msevi_chaninf, cal_slope),
	offsetof(struct msevi_chaninf, cal_offset),
	offsetof(struct msevi_chaninf, lambda_c),
	offsetof(struct msevi_chaninf, f0),
	offsetof(struct msevi_chaninf, refl_slope),
	offsetof(struct msevi_chaninf, refl_offset),
	offsetof(struct msevi_chaninf, nu_c),
	offsetof(struct msevi_chaninf, alpha),
	offsetof(struct msevi_chaninf, beta)
};
const static size_t chaninf_membsize[11]  = { 8, sizeof(uint16_t), sizeof(double), sizeof(double),
					     sizeof(double), sizeof(double), sizeof(double), sizeof(double),
					     sizeof(double), sizeof(double), sizeof(double) };
const char *chaninf_names[11] = { "name", "id", "cal_slope", "cal_offset", "lambda_c", "f0", "refl_slope", "refl_offset",
				 "nu_c", "alpha", "beta" };

int msevi_l15hdf_create_chaninf( hid_t hid, char *name, struct msevi_chaninf *ci )
{
	int r;
	hid_t chaninf_types[11] = { 0, H5T_NATIVE_UINT16, H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE,
				   H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE,
				   H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE };
	hid_t strtype;

	strtype = H5Tcopy( H5T_C_S1 );
	H5Tset_size( strtype, 8 );
	chaninf_types[0] = strtype;

	r = H5TBmake_table( "msevi_chaninf", hid, name, 11, 1, chaninf_size, chaninf_names,
			    chaninf_offsets, chaninf_types, 1, NULL, 0, ci );
	H5Tclose( strtype);
	return r;
}

int msevi_l15hdf_append_chaninf( hid_t hid, char *name, struct msevi_chaninf *ci )
{
	int r;
	hid_t chaninf_types[11] = { 0, H5T_NATIVE_UINT16, H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE,
				    H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE,
				    H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE, H5T_NATIVE_DOUBLE };
	hid_t strtype;

	strtype = H5Tcopy( H5T_C_S1 );
	H5Tset_size( strtype, 8 );
	chaninf_types[0] = strtype;

	r = H5TBappend_records( hid, name, 1, chaninf_size, chaninf_offsets, chaninf_membsize, ci );
	H5Tclose( strtype);
	return r;
}
