/* system includes */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h> 
#include <string.h> 
#include <assert.h>
#include <math.h>

#include <hdf5.h>
#include <hdf5_hl.h>


/* local includes */
#include "h5utils.h"
#include "fileutils.h"
#include "geos.h"


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

int main (int argc, char **argv)
{
	hid_t fid;
	hid_t gid;
	int r;
	hsize_t dim[2];
	float *lat, *lon, *muS, *azS;
	char *reg_str = "3712x3712+0+0";
	struct geos_ctx *gc;
	uint8_t *buf;
	char *lsmask = "msevi-lsmask-rss.bin";

	/* Create file ... */
	fid = H5Fcreate( argv[1], H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
	if(fid<0) goto err_out;

	gc = geos_init_context( reg_str );
	lat = calloc(gc->nlin*gc->ncol, sizeof(float));
	lon = calloc(gc->nlin*gc->ncol, sizeof(float));
	//muS = calloc(gc->nlin*gc->ncol, sizeof(float));
	//azS = calloc(gc->nlin*gc->ncol, sizeof(float));
	//if( lat==NULL || lon==NULL || muS==NULL || azS==NULL )
	if( lat==NULL || lon==NULL )
		goto err_out;
	
	/* calculate geolocation and save */
#if 1
	gc->proj_ss_lon = 9.5;
	lsmask = "msevi-lsmask-rss.bin";
#else
	gc->proj_ss_lon = 0.0;
	lsmask = "msevi-lsmask-hrs.bin";
#endif

	geos_latlon( gc, lat, lon );
	//gid = H5Gcreate2( fid, "geolocation", 0, H5P_DEFAULT, H5P_DEFAULT );
	//if(gid<0) goto err_out;
	gid = fid;
	
	dim[0] = gc->nlin;
	dim[1] = gc->ncol;
	r = H5UTmake_dataset( gid, "latitude", 2, dim, H5T_NATIVE_FLOAT, lat, 6 );
	if(r<0) goto err_out;
	r = sdset_annotate( gid, "latitude", "latitude north", "degrees", 1.0, 0.0 );
	if(r<0) goto err_out;

	r = H5UTmake_dataset( gid, "longitude", 2, dim, H5T_NATIVE_FLOAT, lon, 6 );
	if(r<0) goto err_out;
	r = sdset_annotate(gid, "longitude", "longitude east", "degrees", 1.0, 0.0 );
	if(r<0) goto err_out;

	//r = H5Gclose(gid);
	//if(r<0) goto err_out;
	buf = calloc(1,3712*3712);
	r = fread_binary( lsmask, buf, 3712*3712 );
	if(r<0) goto err_out;
	r = H5UTmake_dataset( gid, "land_sea_mask", 2, dim, H5T_NATIVE_UINT8, buf, 6 );
	if(r<0) goto err_out;
	
#if 0
	geos_satpos( gc, lat, lon, muS, azS );

	gid = H5Gcreate2( fid, "angles", 0, H5P_DEFAULT, H5P_DEFAULT );
	if(gid<0) goto err_out;
	r = H5UTmake_dataset( gid, "muS", 2, dim, H5T_NATIVE_FLOAT, muS, 6 );
	if(r<0) goto err_out;
	r = sdset_annotate( gid, "muS", "cosine of satellite zenith angle", "none", 1.0, 0.0 );
	if(r<0) goto err_out;

	r = H5UTmake_dataset( gid, "azS", 2, dim, H5T_NATIVE_FLOAT, azS, 6 );
	if(r<0) goto err_out;
	r = sdset_annotate( gid, "azS", "satellite azimuth angle", "degrees", 1.0, 0.0 );
	if(r<0) goto err_out;

	r = H5Gclose(gid);
	if(r<0) goto err_out;

	/* close group/file */
	//H5Gclose( gid );
#endif
	H5Fclose( fid );
	free(lat);
	free(lon);
	//free(muS);
	//free(azS);

	return  0;

err_out:
	printf("Error\n");
	return -1;
	
}


