/*****************************************************************************/
/*!
  \file         timeutils.h
  \brief        include file for memutils.c, see that file for details
  \author       Hartwig Deneke
  \date         2008/11/30
 */
/*****************************************************************************/

#ifndef timeutils_h
#define timeutils_h
 
#ifdef __cplusplus
extern "C" {
#endif

/***** MACRO definitions of epochs *******************************************/
#define EPOCH_TAI           2436204.5    /* 1/1/1958 00:00Z */
#define EPOCH_UNIX          2440587.5    /* 1/1/1970 00:00Z */
#define EPOCH_J2000_0       2451545.0    /* 1/1/2000 12:00Z */

/***** function prototypes ***************************************************/
int parse_utc_timestr (const char *timestr, const char *fmt, 
		       time_t *pt);
char *get_utc_timestr( const char *fmt, const time_t t );
int snprint_utc_timestr( char *str, size_t size, const char *fmt,
			 const time_t t );
time_t mod_jday( time_t t, int dmod );
/***** end function prototypes ***********************************************/

#ifdef __cplusplus
}
#endif
 
#endif /* timeutils_h */
