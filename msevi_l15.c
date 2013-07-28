/* System includes */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* Local includes */
#include "cds_time.h"
#include "msevi_l15.h"

/* SEVIRI channel names */
const static char *msevi_chan[12] = { 
	"vis006", "vis008", "ir_016", "ir_039", "wv_062", "wv_073",
	"ir_087", "ir_097", "ir_108", "ir_120", "ir_134", "hrv"
};

const int msevi_chan2id( const char *chan )
{
	int i;
	for( i=0; i<12; i++ ){
		if( strncasecmp(msevi_chan[i],chan,strlen(msevi_chan[i]))==0 ) 
			return i+1;
	}
	return -1;

}

const char *msevi_id2chan( int id )
{
	const char *c = NULL;

	if( id>=MSEVI_CHAN_VIS006 && id<=MSEVI_CHAN_HRV ) c = msevi_chan[id-1];
	return c;
}

/**
 * \brief  Allocate memory for a SEVIRI L15 image structure
 *
 * \param[in]  coverage the coverage of the image
 *
 * \return     a pointer to the allocated structure
 */
struct msevi_l15_image *msevi_l15_image_alloc( int nlin, int ncol )
{

	struct msevi_l15_image *img;

	img = calloc(1,sizeof(*img));
	if(img==NULL) goto err_out;

	img->nlin = nlin;
	img->ncol = ncol;
	img->counts = calloc( 2, nlin*ncol );
	img->line_side_info = calloc( nlin, 
		       sizeof(struct msevi_l15_line_side_info) );

	return img;

 err_out:
	if(img) {
		free(img->counts);
		free(img->line_side_info);
	}
	return NULL;
}

/**
 * \brief  Frees a SEVIRI L15 image structure
 *
 * \param[in]  img   a pointer to the image structure to free
 *
 * \return     nothing
 */
void msevi_l15_image_free( struct msevi_l15_image *img )
{
	if(img) {
		free(img->counts);
		free(img->line_side_info);
		free(img);
	}
	return;
}

int msevi_get_chaninf( int sat_id, int chan_id, struct msevi_chaninf *ci )
{
	
	double f0_msg1[] = { 65.2296, 73.0127, 62.3715, 15.4566, 0.0, 0.0, 
			     0.0, 0.0, 0.0, 0.0, 0.0, 78.8952 };
	double f0_msg2[] = { 65.2065, 73.1869, 61.9923, 15.4566, 0.0, 0.0, 
			     0.0, 0.0, 0.0, 0.0, 0.0, 79.0113 };
	double nu_msg1[] = { 0.0000, 0.0000, 0.0000, 2567.33, 1598.103, 1362.081, 
			     1149.069, 1034.343, 930.647, 839.660, 752.387, 0.0000 };
	double nu_msg2[] = { 0.0000, 0.0000, 0.0000, 2568.832, 1600.548, 1360.330, 
			     1148.620, 1035.289, 931.700, 836.445, 751.792, 0.0000 };

	double alpha_msg1[] = { 0.0000, 0.0000, 0.0000, 0.9956, 0.9962, 0.9991,
				0.9996, 0.9999, 0.9983, 0.9988, 0.9981, 0.0000 };
	double beta_msg1[] = { 0.000, 0.000, 0.000, 3.410, 2.218, 0.478, 
			       0.179, 0.060, 0.625, 0.397, 0.578, 0.000 };
	double alpha_msg2[] = { 0.0000, 0.0000, 0.0000, 0.9954, 0.9963, 0.9991,
				0.9996, 0.9999, 0.9983, 0.9988, 0.9981, 0.0000 };
	double beta_msg2[] = { 0.000, 0.000, 0.000, 3.438, 2.185, 0.470, 
			       0.179, 0.056, 0.640, 0.408, 0.561, 0.000 };
	double *f0, *nu, *alpha, *beta;

	if(sat_id==321) {          /* MSG1/Meteosat8 */
		f0 = f0_msg1;
		nu = nu_msg1;
		alpha = alpha_msg1;
		beta = beta_msg1;
	} else if( sat_id==322 ) { /* MSG2/Meteosat9 */
		f0 = f0_msg2;
		nu = nu_msg2;
		alpha = alpha_msg2;
		beta = beta_msg2;
	} else {
		goto err_out;
	}
	if( chan_id<1 || chan_id>12 ) goto err_out;

	memset( ci->name, 0, 8 );
	strcpy( ci->name, msevi_id2chan(chan_id) );
	ci->id = chan_id;

	ci->f0     = f0[chan_id-1];
	ci->nu_c   = nu[chan_id-1];
	ci->alpha  = alpha[chan_id-1];
	ci->beta   = beta[chan_id-1];

	return 0;
err_out:
	return -1;

}


int msevi_l15_cnt2bt( struct msevi_chaninf *ci, int n, uint16_t *cnt, float *bt )
{
	int i;
	double nu, rad;
	const double c1 = 1.19104e-5;
	const double c2 = 1.43877e-5;

	nu = ci->nu_c;
	for( i=0; i<n; i++ ) {
		rad = ci->cal_slope*cnt[i] + ci->cal_offset;
		bt[i] = c2*nu / ( ci->alpha*log(1.0+nu*nu*nu*c1/rad) ) - ci->beta;
	}
	return 0;
}
