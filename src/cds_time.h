#ifndef _CDS_TIME_H_
#define _CDS_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

struct cds_time {
	uint16_t days;
	uint32_t msec;
};

time_t   time_cds2unix( struct cds_time *ct );
void     time_unix2cds( time_t time, struct cds_time *ct );
double   time_cds2jday( struct cds_time *ct, double epoch );
double difftime_cds( struct cds_time *t1, struct cds_time *t2 );

#ifdef __cplusplus
}
#endif

#endif /* _CDS_TIME_H_ */
