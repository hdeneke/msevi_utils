#ifndef _CGMS_XRIT_H_
#define _CGMS_XRIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/* definition of XRIT file types */
#define XRIT_FTPYE_IMAGE           0
#define XRIT_FTPYE_GTS_MSG         1
#define XRIT_FTPYE_ALPHANUM        2
#define XRIT_FTPYE_ENCRYPTION_KEY  3

struct xrit_file {
	FILE*    fp;
	uint8_t  ftype;
	uint32_t header_len;
	uint64_t data_len;
};

/* definition of XRIT header record types */
#define XRIT_HREC_PRIMARY              0
#define XRIT_HREC_IMAGE_STRUCTURE      1
#define XRIT_HREC_IMAGE_NAVIGATION     2
#define XRIT_HREC_IMAGE_DATA_FUNCTION  3
#define XRIT_HREC_ANNOTATION           4
#define XRIT_HREC_TIME_STAMP           5
#define XRIT_HREC_ANCILLARY_TEXT       6
#define XRIT_HREC_KEY_HEADER           7

struct xrit_hrec_primary {
	uint8_t  hrec_type;
	uint16_t hrec_len;
	uint8_t  file_type;
	uint32_t header_len;
	uint64_t data_len;
};

struct xrit_hrec_image_structure {
	uint8_t  hrec_type;
	uint16_t hrec_len;
	uint8_t  bpp;
	uint16_t ncol;
	uint16_t nlin;
	uint8_t  compression;
};

struct xrit_hrec_image_navigation {
	uint8_t  hrec_type;
	uint16_t hrec_len;
	char     projection[32];
	uint32_t cfac;
	uint32_t lfac;
	uint32_t coff;
	uint32_t loff;
};

struct xrit_file *xrit_fopen(char *file, char *mode);
int xrit_fclose(struct xrit_file *xf);

void *xrit_read_header(struct xrit_file *xf);
void *xrit_read_data(struct xrit_file *xf);
void *xrit_find_hrec(void *hdr, size_t len, int hrec_type);
void *xrit_decode_hrec(void *hdr);

#ifdef __cplusplus
}
#endif

#endif /* _CGMS_XRIT_H_ */
