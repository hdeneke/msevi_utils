/**
 *  \file    timeutils.c
 *  \brief   Utility functions for time handling
 *
 *  \author  Hartwig Deneke
 */
#include <string.h>
#include <strings.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <locale.h>

#include "timeutils.h"

/**
 * static helper function for converting time strings to time_t
 */

static inline char char2tm(char *ptr, char conv, struct tm *tm)
{
	int c,retval;
	char buf[5];

	switch ( conv ){
	case 'Y':
		strncpy( buf, ptr, 4 );
		buf[4] = 0x0;
		c = sscanf( buf, "%d", &(tm->tm_year) );
		if( c != 1) goto errexit;
		/* tm_year is years after 1900 */
		tm->tm_year -= 1900;
		retval = 4;
		break;
	case 'y':
		strncpy( buf, ptr, 2 );
		buf[2] = 0x0;
		c = sscanf( buf,"%d", &(tm->tm_year) );
		if ( c != 1) goto errexit;
		/* tm_year is years after 1900 */
		if ( tm->tm_year <= 68 ){
			tm->tm_year += 100;
		}
		retval = 2;
		break;
	case 'j':
		strncpy( buf, ptr, 3 );
		buf[3] = 0x0;
		c = sscanf( ptr,"%d", &(tm->tm_yday) );
		if ( c != 1) goto errexit;
		/* tm_yday uses 0-365, %j uses 1-366 */
		tm->tm_yday -= 1;

		retval = 3;
		break;
	case 'm':
		strncpy( buf, ptr, 2 );
		buf[2] = 0x0;
		c = sscanf( buf,"%d", &(tm->tm_mon) );
		if ( c != 1) goto errexit;
		/* tm_mon uses 0-11, %m uses 1-12 */
		tm->tm_mon -= 1;
		retval = 2;
		break;
	case 'd':
		strncpy( buf, ptr, 2 );
		buf[2] = 0x0;
		c = sscanf( buf,"%d", &(tm->tm_mday) );
		if ( c != 1 ) goto errexit;
		retval = 2;
		break;
	case 'H':
		strncpy( buf, ptr, 2 );
		buf[2] = 0x0;
		c = sscanf( buf,"%d", &(tm->tm_hour) );
		if ( c != 1) goto errexit;
		retval = 2;
		break;
	case 'M':
		strncpy( buf, ptr, 2 );
		buf[2] = 0x0;
		c = sscanf( buf, "%d", &(tm->tm_min) );
		if ( c != 1) goto errexit;
		retval = 2;
		break;
	case 'S':
		strncpy( buf, ptr, 2 );
		buf[2] = 0x0;
		c = sscanf( buf,"%d", &(tm->tm_sec) );
		if ( c != 1) goto errexit;
		retval = 2;
		break;
	default:
		fprintf(stderr,"WARNING: illegal conversion specifier\n" );
		retval = -1;
	}
	return retval;

errexit:
	return -1;
}


/**
 * static helper for converting a string representing a time to a
 * struct tm
 * returns -1 on error
 */

static int utctimestr2tm( const char *s, const char *fmt, struct tm *tm)
{
	int result=0;
	char *fptr = (char *)fmt;
	char *sptr = (char *)s;

	while( *fptr && *sptr ){
		/* do we have a format field ?  */
		if( *fptr == '%'  && *(fptr+1) ) {
			/* yes, so we do the conversion*/
			result = char2tm( sptr, *(fptr+1), tm );
			if ( result > 0){
				fptr += 2;
				sptr += result;
			}else{
				return -1;
			}
		}else{
			/* no conversion, so format and string must match */
			if( *fptr != *sptr ){
				return -1;
			}
			fptr++;
			sptr++;
		}
	}
	if( !sptr && strchr(fptr,'%') ) return -1;
	return 0;
}


/**
 * local portable implementation for timegm
 */
static time_t my_timegm (struct tm *tm) {

	time_t ret;
	char *tz = NULL;

	tz = getenv ("TZ");
	if (tz) {
		tz = strdup (tz);
	}
	setenv ("TZ", "", 1);
	tzset();
	ret = mktime (tm);
	if (tz)
		setenv("TZ", tz, 1);
	else
		unsetenv("TZ");
	tzset();
	free (tz);
	return ret;
}


/**
 * \brief parse a utc time string and convert it into a time_t value
 *
 * Parse a string describing a UTC time, and convert it to a POSIX time_t
 * variable, i.e. the seconds since 0:00 January 1st, 1970.
 *
 * \param[in]  timestr the time string
 * \param[in]  fmt     the format string, describing how the time is formated
 *                     within the string. Supported format specifiers are:
 *                     '%y', '%Y', '%m', '%d', '%j', '%H', '%M', '%S',
 *                     see man strftime for their meaning
 * \param[out] pt      a pointer to a time_t variable containing the output
 * \return             0 on success or -1 on failure
 *
 * \author             Hartwig Deneke
 */

int parse_utc_timestr( const char *timestr, const char *fmt,
		       time_t *pt)
{

        struct tm t;
        int retval=-1;

        bzero (&t, sizeof(t));
	if ( 0 == utctimestr2tm (timestr, fmt, &t ) ){
		int yday =  t.tm_yday;
		*pt = my_timegm (&t);
		if (    strstr(fmt,"%d") == NULL
		     && strstr(fmt,"%m") == NULL
	  	     && strstr(fmt,"%j") != NULL) {
			*pt += (yday+1) * 86400;
		}
		return 0;
	}
        return retval;
}


/**
 * \brief   returns a newly allocated string representing a time value
 *
 * Represent a POSIX time_t variable containing seconds since
 * 1st of January, 1970, as a specifically formatted, newly allocated
 * string.
 *
 * \param[in]  fmt  the format of the time string
 * \param[in]  t    the time as a POSIX time_t variable
 *
 * \return  the time string, remember to free() it after use
 *
 * \author Hartwig Deneke
 */

char *get_utc_timestr(const char *fmt, const time_t t)
{

	static char time_str[256];
	int c;
	char *retval = NULL;
	struct tm tm;

	memcpy (&tm, gmtime( &t ), sizeof(struct tm));
	c = strftime (time_str, 256, fmt,  &tm);
	if (c < 256 && c > 0)
		retval =  strdup(time_str);
	return retval;
}

int snprint_utc_timestr( char *str, size_t size, const char *fmt,
			 const time_t t )
{

	int c;
	struct tm tm;

	memcpy( &tm, gmtime( &t ), sizeof(struct tm) );
	c = strftime( str, size, fmt,  &tm );
	return c;
}

time_t mod_jday( time_t t, int dmod )
{
	struct tm tm = {};
	time_t tmod;
	int jday;

	/* get struct tm */
	memcpy( &tm, gmtime(&t), sizeof(struct tm) );

	/* set day/month to 1st of January */
	tm.tm_mday = 1;
	tm.tm_mon  = 0;

	/* determine 0-based jday as difference between 1st of Jan and t */
	tmod = mktime( &tm );
	jday = (t - tmod)/86400;

	jday -= (jday % dmod);
	tmod += jday * 86400;

	return tmod;
}



#ifdef _TESTMAIN_

/**
 * \brief small test programme, only compiled if _TESTMAIN_ is defined
 */

int main(int argc,char **argv)
{
	time_t timeval;
	char timebuf[32]={};
	int i, reval=-1;

	if(argc==3){
		i = parse_utc_timestr( argv[2], argv[1], &timeval );
		if ( i >= 0){
			strftime( timebuf, 20, "%Y-%m-%d %H:%M:%S",
				  gmtime(&timeval) );
			printf("%d\n%s\n", (int) timeval, timebuf );
			retval = 0;
		}else{
			printf("ERROR\n");
		}
	}
	return retval;
}
#endif
