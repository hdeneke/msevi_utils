#include <string.h>
#include <stddef.h>

#include "mseviri.h"

/* SEVIRI channel names */
const static char *msevi_chan[12] = { 
	"vis006", "vis008", "ir_016", "ir_039", "wv_062", "wv_073",
	"ir_087", "ir_097", "ir_108", "ir_120", "ir_134", "hrv"
};

const int msevi_chan2id(const char *chan)
{
	int i;
	for( i=0; i<12; i++ ){
		if( strncasecmp(msevi_chan[i],chan,strlen(msevi_chan[i]))==0 ) 
			return i+1;
	}
	return -1;

}

const char *msevi_id2chan(int id)
{
	const char *c = NULL;

	if( id>=MSEVI_CHAN_VIS006 && id<=MSEVI_CHAN_HRV ) c = msevi_chan[id-1];
	return c;
}

struct msevi_chan_info {
	double f0, alpha, beta;
};

#if 0
int msevi_get_chan_info(int sat_id, int chan_id, struct msevi_chan_info *ci )
{

	double f0_msg1[] = {65.2296, 73.0127, 62.3715, 15.4566, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 78.8952 };
	double f0_msg2[] = {65.2065, 73.1869, 61.9923, 15.4566, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 79.0113 };

	double lambda_msg1[] = {0.6402, 0.8093, -1.0, 3.8951, 6.2574, 7.3417, 8.7027, 9.6680, 10.7452, 11.9096, 13.2910, 0.7082 };
	double lambda_msg2[] = {0.6403, 0.8082, -1.0, 3.8928, 6.2479, 7.3512, 8.7061, 9.6591, 10.7331, 11.9554, 13.3016, 0.7064 };


	double alpha_msg1[] = {-1.0, -1.0, -1.0, 0.9956, 0.9962, 0.9991, 0.9996, 0.9999, 0.9983, 0.9988, 0.9981, -1.0 };
	double alpha_msg2[] = {-1.0, -1.0, -1.0, 0.9954, 0.9963, 0.9991, 0.9996, 0.9999, 0.9983, 0.9988, 0.9981, -1.0 };
	double beta_msg1[] = {-1.0, -1.0, -1.0, 3.410, 2.218, 0.478, 0.178, 0.060, 0.625, 0.397, 0.578, -1.0 };
	double beta_msg2[] = {-1.0, -1.0, -1.0, 3.438, 2.185, 0.470, 0.179, 0.056, 0.640, 0.408, 0.561, -1.0 };

	switch(sat_id) {
	case 321:
		ci->f0    = f0_msg1[chan_id-1];
		ci->alpha = alpha_msg1[chan_id-1];
		ci->beta = beta_msg1[chan_id-1];
		break;
	case 322:
		ci->f0 = f0_msg2[chan_id-1];
		ci->alpha = alpha_msg2[chan_id-1];
		ci->beta = beta_msg2[chan_id-1];
		break;
	default:
		ci->f0 = -1.0;
	}
	return f0;
}

#endif
