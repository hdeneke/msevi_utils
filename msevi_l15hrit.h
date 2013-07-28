#define MSEVI_L15HRIT_PROLOGUE     128
#define MSEVI_L15HRIT_EPILOGUE     129

#define MSEVI_HREC_SEGMENT_IDENTIFICATION 128
#define MSEVI_HREC_SEGMENT_LINE_QUALITY   129

#define MSEVI_NCHAN 12
#define MSEVI_NSEG  24

struct msevi_l15hrit_flist {
 	int  nseg[MSEVI_NCHAN+2];
	char *prologue;
	char *epilogue;
	char *channel[MSEVI_NCHAN+2][MSEVI_NSEG+2];
};

struct msevi_hrec_segment_identification {
	uint8_t  hrec_type;
	uint16_t hrec_len;
	uint8_t  sat_id;
	uint8_t  channel_id;
	uint16_t segm_seq_nr;
	uint16_t planned_start_segm_seq_nr;
	uint16_t planned_end_segm_seq_nr;
	uint8_t  data_field_representation;
};

struct msevi_hrec_segment_line_quality {
	uint8_t  hrec_type;
	uint16_t hrec_len;
	struct msevi_l15_line_side_info *line_side_info;
};

struct msevi_l15hrit_flist* msevi_l15hrit_get_flist(char *dir, time_t *time );
void   msevi_l15hrit_free_flist( struct msevi_l15hrit_flist *fl );

struct msevi_l15_image *msevi_l15hrit_read_image( int nfile, char **files, struct msevi_l15_coverage *cov );
struct msevi_l15_header  *msevi_l15hrit_read_prologue( char *file );
struct msevi_l15_trailer *msevi_l15hrit_read_epilogue( char *file );
int msevi_l15hrit_annotate_image( struct msevi_l15_image   *img,
				  struct msevi_l15_header  *hdr,
				  struct msevi_l15_trailer *tra	);

int msevi_l15_fprintf_header( FILE *f, struct msevi_l15_header *hdr );
int msevi_l15_fprintf_trailer( FILE *f, struct msevi_l15_trailer *tr );
void *msevi_l15_hrit_decode_hrec( void *hrec );
