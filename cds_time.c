#include <stdint.h>
#include <time.h>
#include "timeutils.h"
#include "cds_time.h"

/* days between 1/1/1970 and 1/1/1958 */
const static int day_off = (EPOCH_UNIX-EPOCH_TAI);

time_t time_cds2unix(struct cds_time *ct)
{
	uint16_t days;
	days = ct->days-day_off;
	return( (time_t)(ct->msec+500)/1000+days*86400 );
}

void time_unix2cds(time_t time, struct cds_time *ct)
{
	ct->days  = time/86400;	
	ct->msec =  (time%86400) * 1000;
	ct->days += day_off;
	return;
}

double time_cds2jday(struct cds_time *ct, double epoch)
{
	return(EPOCH_TAI-epoch+ct->days+(ct->msec/8.64e7));
}

double difftime_cds(struct cds_time *t1, struct cds_time *t2)
{
	double dt;

	dt = t2->days - t1->days;
	dt += ((double)t2->msec-t1->msec)/8.64e7;
	return(dt);
}
