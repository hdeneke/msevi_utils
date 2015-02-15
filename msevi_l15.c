/* System includes */
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* Local includes */
#include "parson.h"
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

struct msevi_chaninf *msevi_get_chaninf( struct msevi_satinf *satinf, int chan_id )
{
	int i;
        for( i=0; i<MSEVI_NR_CHAN; i++ ) {
                if( satinf->chaninf[i].id == chan_id )
                        return &(satinf->chaninf[i]);
        }
        return NULL;
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

static int json2chaninf(JSON_Object *chan_obj, struct msevi_chaninf *ci )
{
	int chan_id;
	char *chan_nam;
	const double vnan = nan("");

	chan_id = (int) json_object_get_number(chan_obj, "id");
	ci->id = chan_id;

	chan_nam = (char *) json_object_get_string(chan_obj, "name");
	strncpy(ci->name, chan_nam, 7);

	ci->lambda_c = (float) json_object_get_number(chan_obj, "lambda_c");

	/* solar channels */
	if( (    chan_id>=MSEVI_CHAN_VIS006 && chan_id<=MSEVI_CHAN_IR_039)
	      || chan_id==MSEVI_CHAN_HRV ) { /* solar */
		ci->f0  = json_object_get_number(chan_obj, "f0");
	} else {
		ci->f0 = 0.0;
	}

	/* thermal channels */
	if( chan_id>=MSEVI_CHAN_IR_039 && chan_id<=MSEVI_CHAN_IR_134 ) {
		ci->nu_c  = json_object_get_number(chan_obj, "nu_c");
		ci->alpha = json_object_get_number(chan_obj, "alpha");
		ci->beta  = json_object_get_number(chan_obj, "beta");
	} else {
		ci->alpha = vnan;
		ci->beta  = vnan;
	}

	return 0;
}

struct msevi_satinf *msevi_read_satinf( char *file, int sat_id )
{
	struct msevi_satinf *satinf = NULL;
        JSON_Value   *root_val;
        JSON_Object  *root_obj, *sat_obj, *chan_obj;
	JSON_Array   *sat_arr, *chan_arr;
	size_t i,j,n,m;

       /* open json file, get root object */
        root_val = json_parse_file( file );
	if(root_val==NULL) goto err_out;

	root_obj = json_value_get_object(root_val);
        if(root_obj==NULL) goto err_out;

	/* get satellite array */
	sat_arr = json_object_get_array(root_obj, "satellites" );
        if(sat_arr==NULL) goto err_out;

	m = json_array_get_count(sat_arr);
	for( j=0; j<m; j++ ) {
		int id;

		sat_obj = json_array_get_object(sat_arr, j);
		id = (int)json_object_get_number(sat_obj, "id" );

		if(id==sat_id) {
			char *sat_nam;

			satinf = calloc(1,sizeof(struct msevi_satinf));
			if(satinf==NULL) goto err_out;

			satinf->id = sat_id;

			sat_nam = json_object_get_string(sat_obj, "name");
			if(sat_nam==NULL) goto err_out;
			strcpy(satinf->name, sat_nam);

			sat_nam = json_object_get_string(sat_obj, "long_name");
			if(sat_nam==NULL) goto err_out;
			strcpy(satinf->long_name, sat_nam);

			chan_arr = json_object_get_array(sat_obj, "channel" );
			if(chan_arr==NULL) goto err_out;

			n = json_array_get_count(chan_arr);
			for( i=0; i<n; i++ ) {
				/* get chan object */
				chan_obj = json_array_get_object(chan_arr, i);
				if(chan_obj==NULL) goto err_out;

				json2chaninf(chan_obj, &satinf->chaninf[i]);
			}
		}
	}

err_out:
	if(root_val) json_value_free(root_val);
	return satinf;
}

struct msevi_region *msevi_read_region( char *file, char *svc, char *region )
{

	struct msevi_region *mreg = NULL;
        JSON_Value   *root_val;
        JSON_Object  *root_obj, *reg_obj;
	JSON_Array   *reg_arr;
	size_t i, n;

       /* open json file, get root object */
	printf("Open file\n");
        root_val = json_parse_file( file );
        if(root_val==NULL) goto err_out;
	printf("Parse file\n");

	root_obj = json_value_get_object(root_val);
        if(root_obj==NULL) goto err_out;
	printf("Got root\n");

	/* get region array for satellite service */
	reg_arr = json_object_get_array(root_obj, svc);
        if(reg_arr==NULL) goto err_out;
	printf("Service: %s\n",svc);

	/* get array member with matching region name */
        n = json_array_get_count(reg_arr);
	for( i=0; i<n; i++ ) {
		const char *reg_name;

		/* get region object */
                reg_obj = json_array_get_object(reg_arr, i);
		if(reg_obj==NULL) goto err_out;

		reg_name = json_object_get_string(reg_obj, "name");
		if( strncmp(reg_name,region,16)==0 ) {

			mreg = calloc(1,sizeof(struct msevi_region));
			if(mreg==NULL) goto err_out;

			strncpy(mreg->name, reg_name, 15);
			mreg->lin0 = json_object_get_number(reg_obj, "lin0");
			mreg->col0 = json_object_get_number(reg_obj, "col0");
			mreg->nlin = json_object_get_number(reg_obj, "nlin");
			mreg->ncol = json_object_get_number(reg_obj, "ncol");
			break;
		}
	}

err_out:
	if(root_val) json_value_free(root_val);
	return mreg;
}
