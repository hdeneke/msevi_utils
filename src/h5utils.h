/*****************************************************************************/
/*!
  \file         h5utils.h
  \brief        include file for h5utils.c, see that file for details
  \author       Hartwig Deneke
  \date         2008/06/10
 */
/*****************************************************************************/

#ifndef h5utils_h
#define h5utils_h

#ifdef __cplusplus
extern "C" {
#endif

/***** MACRO definitions *****************************************************/

#define H5UTset_attribute H5LT_set_attribute_numerical

/* the H5T_NATIVE_XXX types are not constants, but
   a strange macro. Hence, use hid_t * as type, and use this macro */
#define H5UT_SET_TYPE(_type) ( &_type##_g )

/***** end MACRO definitions *************************************************/

/***** function prototypes ***************************************************/
hid_t H5UTfile_open (const char *fname, unsigned int flags, hid_t create_id,
		     hid_t access_id);

int H5UTget_attribute_strlen (hid_t loc_id,char *obj_name, char *attr_name);

char *H5UTget_attribute_string (hid_t loc_id,char *obj_name, char *attr_name);

int H5UTdataset_get_info (hid_t loc_id, char *obj_name, int *ndim,
			  int *dims, int *type_size);

herr_t H5UTread_dataset (hid_t loc_id, char *obj_name, void *data,
			 int *offset, int *count);

hid_t H5UTget_dataset_type (hid_t loc_id, char *obj_name);

herr_t H5UTfind_attribute (hid_t loc_id, char *obj_name, char *attr_name);

int H5UTset_attribute_string_array (int loc_id, char *obj_name,
				    char *attr_name, int nstr, char **str);

int H5UTwrite_dataset (hid_t loc_id, const char *dset_name, void *data,
		       int *offset, int *count);

int H5UTmake_dataset (hid_t loc_id, const char *dset_name, int rank, const hsize_t *dims,
		      const hid_t type_id, const void *data,  int compression);

int H5UTset_attribute_string_padded (int loc_id, char *obj_name,
				     char *attr_name, char *str, int len);

int H5UTset_scal_attribute( int loc_id, char *obj_name, char *attr_name,
			    hid_t type_id, void *data );
/***** end function prototypes ***********************************************/

#ifdef __cplusplus
}
#endif

#endif /* h5utils_h */
