#ifndef _MSEVI_L15HDF_H_
#define _MSEVI_L15HDF_H_

#ifdef __cplusplus
extern "C" {
#endif

extern const char *msevi_l15hdf_img_grp;
extern const char *msevi_l15hdf_meta_grp;
extern const char *msevi_l15hdf_lsi_grp;

int msevi_l15hdf_write_image( hid_t gid, struct msevi_l15_image *img );
struct msevi_l15_image *msevi_l15hdf_read_image( hid_t gid, int chanid );

int msevi_l15hdf_write_line_side_info( hid_t lsi_gid, struct msevi_l15_image *img );

int msevi_l15hdf_write_coverage( hid_t hid, char *name, struct msevi_l15_coverage *cov );
int msevi_l15hdf_append_coverage( hid_t hid, char *name, struct msevi_l15_coverage *cov );
struct msevi_l15_coverage *msevi_l15hdf_read_coverage( hid_t gid, char *name, char *chan );

int msevi_l15hdf_create_chaninf( hid_t hid, char *name, struct msevi_chaninf *ci );
int msevi_l15hdf_append_chaninf( hid_t hid, char *name, struct msevi_chaninf *ci );
int msevi_l15hdf_read_chaninf( hid_t hid, char *name, int chan_id, struct msevi_chaninf *ci );

#endif /* _MSEVI_L15HDF_H_ */
