/**
 *  \file    h5utils.c
 *  \brief   HDF5 utility functions
 *
 *  \author  Hartwig Deneke
 */
#include <hdf5.h>
#include <hdf5_hl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include "fileutils.h"
#include "h5utils.h"


#if 0
/**
 * \brief  creates or opens a HDF5 file
 *
 * \return the hdf5 id of the file
 */
hid_t H5UTfile_open (const char *fname, unsigned int flags, hid_t create_id,
		     hid_t access_id)
{
	hid_t hid;
	int r;

	r = file_info (fname, NULL, NULL);
	if (r==0) {  /* file exists*/
		if (flags & H5F_ACC_EXCL) return -1;
		if (flags & H5F_ACC_TRUNC) {
			hid = H5Fcreate (fname, flags, create_id, access_id);
		} else {
			hid = H5Fopen (fname, flags, access_id);
		}
	} else {     /* file does not exist */
		hid = H5Fcreate (fname, flags, creade_id, access_id);
	}
	return hid;
}
#endif

/**
 * \brief returns the length of a string attribute
 *
 * \param[in]  loc_id     the location of the object containing the attribute
 * \param[in]  obj_name   the name of the object containing the attribute
 * \param[in]  attr_name  the name of the attribute
 *
 * \return zero on success, otherwise -1 and errno is set appropriately
 */
int H5UTget_attribute_strlen( hid_t loc_id,char *obj_name,char *attr_name )
{
	int r=-1, rank;
        hsize_t dims[1];
        H5T_class_t obj_class;
        size_t size;

	H5LTget_attribute_ndims (loc_id, obj_name, attr_name,  &rank);
        if (rank == 0) {
                H5LTget_attribute_info (loc_id, obj_name, attr_name, dims,
                                        &obj_class, &size);
                if (obj_class == H5T_STRING) {
                        r = size;
                }
        }
        return r;
}


/**
 * \brief returns a string attribute in an allocated buffer
 *
 * \param[in]  loc_id     the location of the object containing the attribute
 * \param[in]  obj_name   the name of the object containing the attribute
 * \param[in]  attr_name  the name of the attribute
 *
 * \return  the string in a malloc'd buffer, which should be free()'d after use
 */
char *H5UTget_attribute_string(hid_t loc_id,char *obj_name, char *attr_name )
{
        int i;
        char *buf;

        i = H5UTget_attribute_strlen (loc_id, obj_name,  attr_name);
        if (i < 0) return NULL;
        buf = calloc (1, i + 1);
        i = H5LTget_attribute_string (loc_id, obj_name,  attr_name, buf);
        if (i < 0) {
                free (buf);
                return NULL;
        }
        return buf;
}


/**
 * \brief return number of dimensions, length of dimensions and datatype size
 *
 * \param[in]    loc_id     the location of the object containing the attribute
 * \param[in]    obj_name   the name of the object, or NULL
 * \param[inout] ndim       the number of dimensions, or NULL
 * \param[out]   dims       array with length of each dimension
 *                          up to ndim, or NULL
 * \param[out]  type_size  the size of the data items
 *
 * \return zero on success, otherwise -1 and errno is set appropriately
 */
herr_t H5UTdataset_get_info (hid_t loc_id, char *obj_name, int *ndim,
			     int *dims, int *type_size)
{
	hid_t obj_id, sid, tid;
	hsize_t *hdims;
	int i, r, nd;

	obj_id = obj_name ? H5Dopen1 (loc_id, obj_name) : loc_id;
	if (obj_id<0) return -1;

	if (ndim) {

		sid = H5Dget_space (obj_id);
		nd = H5Sget_simple_extent_ndims (sid);

		if( dims ) {

			hdims = calloc (nd, sizeof(hsize_t));
			if (hdims == NULL) {
				errno = ENOMEM;
				return -1;
			}

			r = H5Sget_simple_extent_dims (sid, hdims, NULL);
			if (r < 0) return -1;

			for (i=0; i<*ndim; i++) dims[i] = hdims[i];
			free (hdims);
		}
		H5Sclose (sid);
		*ndim = nd;
	}

	if (type_size) {
		tid = H5Dget_type (obj_id);
		*type_size = H5Tget_size (tid);
		H5Tclose (tid);
	}

	/* cleanup and return */
	if (obj_name) H5Dclose (obj_id);
	return 0;
}


/**
 * \brief  creates and optionally writes data to a dataset
 *
 * \param[in]  loc_id       the location where the dataset is to be created
 * \param[in]  dset_name    the name of the dataset
 * \param[in]  rank         the number of dimensions
 * \param[in]  dims         the length of each dimension
 * \param[in]  type_id      the datatype of the dataset
 * \param[in]  data         pointer to the data to be written, or NULL of the dataset should only be created
 * \param[in]  compression  the compression level (0=none, ...9=best)
 *
 * \return zero on success, otherwise -1 and errno is set appropriately
 */
int H5UTmake_dataset (hid_t loc_id, const char *dset_name, int rank, const hsize_t *dims,
		      const hid_t type_id, const void *data,  int compression)
{

	int s, retval = 0;
	hid_t   did, sid, plist = H5P_DEFAULT;

	/* create dataspace */
	sid = H5Screate_simple (rank, dims, NULL);
	if (sid < 0) return -1;

	/* set compression if requested */
	if (compression) {
		plist = H5Pcreate (H5P_DATASET_CREATE);
		s = H5Pset_chunk (plist, rank, dims);
		s = H5Pset_deflate (plist, compression);
	}

	/* create dataset */
	did = H5Dcreate1 (loc_id, dset_name, type_id, sid, plist);
	if (did <0) goto err_exit;

	/* write dataset */
	if (data) {
		s = H5Dwrite (did, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, data );
		if (s <0) goto err_exit;
	}

	/* cleanup */
	s = H5Dclose (did);
	if (s<0) retval = -1;

	s = H5Sclose (sid);
	if (s<0) retval = -1;

	if (compression) {
		s = H5Pclose (plist);
		if (s<0) retval = -1;
	}

	return retval;

err_exit:
	H5Dclose (did);
	H5Sclose (sid);
	H5Pclose (plist);
	return -1;
}


/**
 * \brief writes data to a dataset or part thereof
 *
 * \param[in]  loc_id       the location where the dataset is to be created
 * \param[in]  obj_name    the name of the dataset, or NULL if loc_id is to be used
 * \param[in]  data         pointer to the data to be written
 * \param[in]  offset       per-dimension offset of data to be written, or NULL
 * \param[in]  count        per-dimension number of items to be written, or NULL
 *
 * \return zero on success, otherwise -1 and errno is set appropriately
 */
int H5UTwrite_dataset (hid_t loc_id, const char *obj_name, void *data, int *offset, int *count)
{
	hid_t obj_id, memspace_id, filespace_id, type_id, plist;
	hsize_t *hoffset, *hcount, dims[5];
	int i, ndim, s;

	obj_id = obj_name ? H5Dopen1 (loc_id, obj_name) : loc_id;
	if (obj_id<0) return -1;

	type_id = H5Tget_native_type (H5Dget_type(obj_id), H5T_DIR_ASCEND);
	plist = H5Dget_create_plist (obj_id);

	filespace_id = H5Dget_space (obj_id);
	ndim = H5Sget_simple_extent_ndims (filespace_id);
	H5Sget_simple_extent_dims (filespace_id, dims, NULL);

	hoffset = calloc (ndim, sizeof(hsize_t));
	if (offset) {for (i=0; i<ndim; i++) hoffset[i] = offset[i];}

	hcount = calloc (ndim, sizeof(hsize_t));
	if (count)  {
		for (i=0; i<ndim; i++) hcount[i] = count[i];
	} else {
		H5Sget_simple_extent_dims (filespace_id, hcount, NULL);
	}

	/* select subset */
	s = H5Sselect_hyperslab (filespace_id, H5S_SELECT_SET, hoffset,
				 NULL, hcount, NULL);
	memspace_id = H5Screate_simple (ndim, hcount, NULL);

	/* everything setup properly, now write data */
	s = H5Dwrite (obj_id, type_id, memspace_id, filespace_id, H5P_DEFAULT, data);

	/* clean up after me */
	free (hoffset);
	free (hcount);

	H5Sclose (memspace_id);
	H5Sclose (filespace_id);
	H5Pclose (plist);

	H5Tclose (type_id);
	if (obj_name) H5Dclose (obj_id);
	return 0;
}


/**
 * \brief  reads (part of) a dataset
 *
 * \param[in]  loc_id       the location where the dataset to be read
 * \param[in]  dset_name    the name of the dataset
 * \param[in]  data         pointer to store the data
 * \param[in]  offset       per-dimension offset of data to be written, or NULL
 * \param[in]  count        per-dimension number of items to be written, or NULL
 *
 * \return zero on success, otherwise -1 and errno is set appropriately
 */
int H5UTread_dataset (hid_t loc_id, char *obj_name, void *data,
		      int *offset, int *count)
{
	hid_t obj_id, type_id;
	hid_t src_space, trgt_space;
	hsize_t *hoffset, *hcount;
	int i, r, ndim;

	obj_id = obj_name ? H5Dopen1 (loc_id, obj_name) : loc_id;
	if (obj_id<0) return -1;

	src_space = H5Dget_space (obj_id);
	assert (src_space >= 0);

	ndim = H5Sget_simple_extent_ndims (src_space);
	hoffset = calloc(ndim, sizeof(hsize_t));
	hcount = calloc(ndim, sizeof(hsize_t));
	assert (hoffset != NULL && hcount != NULL);

	if (count != NULL) {
		for (i=0; i<ndim; i++) hcount[i] = (hsize_t) count[i];
	} else {
		H5Sget_simple_extent_dims (src_space, hcount, NULL);
	}

	if (offset != NULL) {
    		for (i=0; i<ndim; i++) hoffset[i] = (hsize_t) offset[i];
	} else {
		for (i=0; i<ndim; i++) hoffset[i] = 0;
	}

	/* select file dataspace to read and determine in-memory datatype */
	r = H5Sselect_hyperslab (src_space, H5S_SELECT_SET, hoffset, NULL,
				 hcount, NULL);
	trgt_space = H5Screate_simple (ndim, hcount, NULL);
	type_id = H5Dget_type (obj_id);

	/* read data */
	r = H5Dread (obj_id,  H5Tget_native_type(type_id,H5T_DIR_ASCEND),
		     trgt_space, src_space, H5P_DEFAULT, data);

	/* cleanup */
	H5Tclose (type_id);
	H5Sclose (src_space);
	H5Sclose (trgt_space);
	if (obj_name) H5Dclose (obj_id);

	free (hoffset);
	free (hcount);

	return 0;
}


/**
 * \brief  get type of a dataset
 *
 * \param[in]  loc_id     the location of the object containing the attribute
 * \param[in]  obj_name   the name of the object containing the attribute, or NULL
 *
 * \return  the type of the dataset
 */
hid_t H5UTget_dataset_type (hid_t loc_id, char *obj_name)
{
	hid_t obj_id, type_id;

	obj_id = obj_name ? H5Dopen1 (loc_id, obj_name) : loc_id;
	type_id = H5Dget_type(obj_id);
	if (obj_name) H5Dclose (obj_id);
	return type_id;
}


/**
 * \brief tests for the presence of an attribute
 *
 * \param[in]  loc_id     the location of the object containing the attribute
 * \param[in]  obj_name   the name of the object containing the attribute, or NULL
 * \param[in]  attr_name  the name of the attribute
 *
 * \return  the string in a malloc'd buffer, which should be free()'d after use
 */
int H5UTfind_attribute (hid_t loc_id, char *obj_name, char *attr_name)
{
	hid_t obj_id;
	int retval;

	obj_id = obj_name ? H5Dopen1 (loc_id, obj_name) : loc_id;
	retval = (int)H5LTfind_attribute (obj_id, attr_name);
	if (obj_name) H5Dclose (obj_id);
	return retval;
}

/**
 * \brief attaches an array of strings as attribute to an hdf5 object
 *
 * \param[in]  loc_id     the location of the object containing the attribute
 * \param[in]  obj_name   the name of the object containing the attribute, or NULL
 * \param[in]  attr_name  the name of the attribute
 * \param[in]  nstr       the number of strings
 * \param[in]  str        an array of strings
 *
 * \return  0 on SUCESS, and -1 on failure
 */
int H5UTset_attribute_string_array (int loc_id, char *obj_name, char *attr_name, int nstr,
				    char **str)
{
	hid_t obj_id, dataspace_id, type_id, str_array_id;
        hsize_t dims[1]={};

	obj_id = obj_name ? H5Dopen1 (loc_id, obj_name) : loc_id;

        /* create dataspace */
        dims[0] = nstr;
        dataspace_id = H5Screate_simple (1 , dims, NULL);

        /* create variable string type */
        type_id = H5Tcopy (H5T_C_S1);
        H5Tset_size (type_id, H5T_VARIABLE);

        /* create attribute and write out string data */
        str_array_id = H5Acreate1 (obj_id, attr_name, type_id,
                                  dataspace_id, H5P_DEFAULT);
        H5Awrite (str_array_id, type_id, str);

        /* cleanup */
        H5Aclose (str_array_id);
        H5Sclose (dataspace_id);
        H5Tclose (type_id);
	if (obj_name) H5Dclose (obj_id);
        return 0;
}

/**
 * \brief creates and writes a fixed-length string attribute padded with 0x0s
 *
 * \param[in]  loc_id     the location of the object containing the attribute
 * \param[in]  obj_name   the name of the object containing the attribute, or NULL
 * \param[in]  attr_name  the name of the attribute
 * \param[in]  str        the string
 * \param[in]  len        the length of the string
 *
 * \return  0 on SUCESS, and -1 on failure
 */
int H5UTset_attribute_string_padded (int loc_id, char *obj_name, char *attr_name,
				     char *str, int len)
{
	hid_t obj_id, type_id, space_id, str_id;
	char *strbuf;
	int r;

	/* copy string to zero-padded buffer */
	strbuf = malloc (len);
	strncpy (strbuf, str, len);
	strbuf[len-1] = 0x0;

	/* open object */
	obj_id = obj_name ? H5Dopen1 (loc_id, obj_name) : loc_id;

        /* create variable string type and dataspace */
        type_id = H5Tcopy (H5T_C_S1);
        H5Tset_size (type_id, len);
        space_id = H5Screate (H5S_SCALAR);

        /* create attribute and write out string data */
        str_id = H5Acreate1 (obj_id, attr_name, type_id, space_id, H5P_DEFAULT);
        r = H5Awrite (str_id, type_id, str);

        /* cleanup and return */
	free (strbuf);
        H5Aclose (str_id);
        H5Sclose (space_id);
        H5Tclose (type_id);
	if (obj_name) H5Dclose (obj_id);

        return 0;
}

/**
 * \brief creates and writes a scalar attribute
 *
 * \param[in]  loc_id     the location of the object containing the attribute
 * \param[in]  obj_name   the name of the object containing the attribute, or NULL
 * \param[in]  attr_name  the name of the attribute
 * \param[in]  type_id    the data type of the attribute
 * \param[in]  data       the pointer to the attribute data
 *
 * \return  0 on SUCESS, and -1 on failure
 */
int H5UTset_scal_attribute( int loc_id, char *obj_name, char *attr_name,
			    hid_t type_id, void *data )
{
	hid_t obj_id=-1, space_id=-1, attr_id=-1;
	int r;

	/* open object */
	obj_id = obj_name ? H5Dopen1 (loc_id, obj_name) : loc_id;

        space_id = H5Screate (H5S_SCALAR);
	if (space_id <0) goto err_out;

        /* create attribute and write out string data */
        attr_id = H5Acreate1 (obj_id, attr_name, type_id, space_id, H5P_DEFAULT);
	if (attr_id <0) goto err_out;

        r = H5Awrite (attr_id, type_id, data);
	if (r<0) goto err_out;

        /* cleanup and return */
        H5Aclose( attr_id );
        H5Sclose( space_id );
	if (obj_name) H5Dclose (obj_id);

        return 0;

 err_out:
	if (obj_name && obj_id >= 0) H5Dclose (obj_id);
	if ( attr_id >= 0 ) H5Aclose( attr_id );
	if ( space_id >= 0 ) H5Sclose( space_id );
	return -1;
}
