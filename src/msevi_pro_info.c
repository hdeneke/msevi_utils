/* system includes */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include <hdf5.h>
#include <hdf5_hl.h>

/* local includes */
#include "mathutils.h"
#include "timeutils.h"
#include "h5utils.h"
#include "timeutils.h"
#include "cgms_xrit.h"
#include "cds_time.h"
#include "msevi_l15data.h"
#include "msevi_l15hrit.h"
#include "eumwavelet.h"


double cheb_eval( double x, int n, double *c, double a, double b )
{
	double xp, xp2, y;
	double b0=0.0, b1=0.0, b2=0.0;

	xp  = (2.0*x-a-b)/(b-a); 	/* map interval [a,b] to [-1,1] */
	xp2 = 2.0*xp;

	for( int i=0; i<n; i++ ){
		b2 = b1;
		b1 = b0;
		b0 = xp2*b1-b2+c[n-1-i];
	}
	return (b0-b2)*0.5;
}

int main( int argc, char **argv )
{
	int i, iorb;
	struct msevi_l15_header  *hdr;
	struct msevi_l15_trailer *tra;
	time_t tstart, tend, teval;
	time_t tscan;
	char *tfmt = "%Y-%m-%dT%H:%M:%SZ", tstr[32];

	/* read pro/epilogue */
	hdr = msevi_l15hrit_read_prologue( argv[1] );
	printf("read pro\n");
	tra = msevi_l15hrit_read_epilogue( argv[2] );
	printf("read epi\n");
	if(hdr==NULL || tra==NULL) goto err_out;

	printf( "satellite_id=%d\n", hdr->satellite_status.satellite_definition.satellite_id );
	printf( "nominal_longitude=%.3f\n", hdr->satellite_status.satellite_definition.nominal_longitude );
	printf( "satellite_status=%d\n", hdr->satellite_status.satellite_definition.satellite_status );
	tstart = time_cds2unix( &tra->image_production_stats.actual_scanning_summary.forward_scan_start );
	tend   = time_cds2unix( &tra->image_production_stats.actual_scanning_summary.forward_scan_end );
	tscan  = (tstart+1)/2 + (tend+1)/2;
	strftime(tstr, 32, tfmt, gmtime(&tscan) );
	printf( "average_scan_time=%s\n", tstr);
	for( i=0; i<12; i++ ) {
		printf("chan_id=%d,cal_slope=%.8f,cal_offset=%.8f\n", i+1,
		       hdr->radiometric_processing.l15_image_calibration[i].cal_slope,
		       hdr->radiometric_processing.l15_image_calibration[i].cal_offset );
	}

	for( i=0; i<100; i++ ) {
		tstart = time_cds2unix( &hdr->satellite_status.orbit.orbitcoef[i].start_time );
		tend = time_cds2unix( &hdr->satellite_status.orbit.orbitcoef[i].end_time );
		if( (tscan>=tstart) && (tscan<tend) ){
			iorb=i;
			break;
		}

	}
	tstart = time_cds2unix( &hdr->satellite_status.orbit.period_start_time );
	tend   = time_cds2unix( &hdr->satellite_status.orbit.period_end_time );
	strftime( tstr, 32, tfmt, gmtime(&tstart) );
	printf( "period_start=%s, ", tstr );
	strftime( tstr, 32, tfmt, gmtime(&tend) );
	printf( "period_end=%s\n", tstr );

	tstart = time_cds2unix( &hdr->satellite_status.orbit.orbitcoef[iorb].start_time );
	strftime( tstr, 32, tfmt, gmtime(&tstart) );
	printf( "orbit_start=%s, ", tstr );
	tend   = time_cds2unix( &hdr->satellite_status.orbit.orbitcoef[iorb].end_time );
	strftime( tstr, 32, tfmt, gmtime(&tend) );
	printf( "orbit_end=%s\n", tstr );

	printf( "x=%.3f ", cheb_eval((double) tscan, 8, hdr->satellite_status.orbit.orbitcoef[iorb].x, (double)tstart, (double)tend ));
	printf( "y=%.3f ", cheb_eval((double) tscan, 8, hdr->satellite_status.orbit.orbitcoef[iorb].y, (double)tstart, (double)tend ));
	printf( "z=%.3f\n", cheb_eval((double) tscan, 8, hdr->satellite_status.orbit.orbitcoef[iorb].z, (double)tstart, (double)tend ));

#if 0
	teval  = time_cds2unix( &hdr->satellite_status.orbit.orbitcoef[iorb].start_time );
	teval  = time_cds2unix( &hdr->satellite_status.orbit.orbitcoef[iorb].end_time );
	printf( "x=%.3f", cheb_eval((double) teval, 8, hdr->satellite_status.orbit.orbitcoef[i].x, (double)tstart, (double)tend ));
	printf( "y=%.3f ", cheb_eval((double) teval, 8, hdr->satellite_status.orbit.orbitcoef[i].y, (double)tstart, (double)tend ));
	printf( "z=%.3f\n", cheb_eval((double) teval, 8, hdr->satellite_status.orbit.orbitcoef[i].z, (double)tstart, (double)tend ));

	printf( "x=%.3f ", cheb_eval((double) teval, 8, hdr->satellite_status.orbit.orbitcoef[i].x, (double)tstart, (double)tend ));
	printf( "y=%.3f ", cheb_eval((double) teval,  8, hdr->satellite_status.orbit.orbitcoef[i].y, (double)tstart, (double)tend ));
	printf( "z=%.3f\n", cheb_eval((double) teval,  8, hdr->satellite_status.orbit.orbitcoef[i].z, (double)tstart, (double)tend ));

	teval  = time_cds2unix( &hdr->satellite_status.orbit.orbitcoef[i+1].start_time );
	printf( "x=%.3f ", cheb_eval((double) teval, 8, hdr->satellite_status.orbit.orbitcoef[i+1].x, (double)tstart, (double)tend ));
	printf( "y=%.3f ", cheb_eval((double) teval, 8, hdr->satellite_status.orbit.orbitcoef[i+1].y, (double)tstart, (double)tend ));
	printf( "z=%.3f\n", cheb_eval((double) tstart, 8, hdr->satellite_status.orbit.orbitcoef[i+1].z, (double)tstart, (double)tend ));
#endif

	/* cleanup */
	free(hdr);

	return  0;

err_out:
	printf("Error\n");
	return -1;

}


