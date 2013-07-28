#ifndef _MSEVI_L15_H_
#define _MSEVI_L15_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Macro definitions */
#define MSEVI_NR_CHAN   12

#define MSEVI_CHAN_VIS006   1
#define MSEVI_CHAN_VIS008   2
#define MSEVI_CHAN_IR_016   3
#define MSEVI_CHAN_IR_039   4
#define MSEVI_CHAN_WV_062   5
#define MSEVI_CHAN_WV_073   6
#define MSEVI_CHAN_WV_087   7
#define MSEVI_CHAN_IR_098   8
#define MSEVI_CHAN_IR_108   9
#define MSEVI_CHAN_IR_120  10
#define MSEVI_CHAN_IR_134  11
#define MSEVI_CHAN_HRV     12

typedef uint8_t bool_t;

struct msevi_l15_coverage {
	char     channel[8];
	uint32_t southern_line;
	uint32_t northern_line;
	uint32_t eastern_column;
	uint32_t western_column;
};

struct msevi_l15_line_side_info {
	uint32_t nr_in_grid;
	struct cds_time acquisition_time;
	uint8_t  validity;
	uint8_t  radiometric_quality;
	uint8_t  geometric_quality;
};

struct msevi_l15_image {

	uint32_t  nlin;
	uint32_t  ncol;
	uint8_t   depth;

	uint16_t  spacecraft_id;
	uint16_t  channel_id;
	uint16_t  segment_id;
	double    cal_slope, cal_offset;

	uint16_t  *counts;
	struct msevi_l15_coverage coverage;
	struct msevi_l15_line_side_info *line_side_info;
};

struct msevi_l15_header {

	uint8_t version;

	struct _satellite_status {
		struct _satellite_definition {
			uint16_t satellite_id;
			float    nominal_longitude;
			uint8_t  satellite_status;
		} satellite_definition;

		struct _satellite_operations {
			struct _manoeuvre {
				uint8_t  flag;
				time_t   start_time;
				time_t   end_time;
				uint8_t  type;
			} last_manoeuvre, next_manoeuvre;
		} satellite_operations;

		struct _orbit {
			struct cds_time period_start_time;
			struct cds_time period_end_time;
			struct _orbitcoef {
				struct cds_time start_time;
				struct cds_time end_time;
				double x[8];
				double y[8];
				double z[8];
				double vx[8];
				double vy[8];
				double vz[8];
			} orbitcoef[100];
		} orbit;

		struct _attitude{
			time_t period_start_time;
			time_t period_end_time;
			struct _attitudecoef {
				time_t start_time;
				time_t end_time;
				double principle_axis_offset_angle;
				double XofSpinAxis[8];
				double YofSpinAxis[8];
				double ZofSpinAxis[8];
			} attitudecoef[100];
		} attitude;

		struct utc_correlation{
			time_t period_start_time;
			time_t period_end_time;
			double onboard_time_start;
			double var_onboard_time_start;
			double a1;
			double var_a1;
			double a2;
			double var_a2;
		} utc_correlation;

	} satellite_status;

	struct _image_acquisition {
	} image_acquisition;

	struct _celestial_events {
	} celestial_events;

	struct _image_description {

		struct _projection_description {
			uint8_t type_of_projection;
			float   longitude_of_ssp;
		} projection_description;
		
		struct _reference_grid {
			uint32_t number_of_lines;
			uint32_t number_of_columns;
			float    line_dir_grid_step;
			float    column_dir_grid_step;
			uint8_t  grid_origin;
		} reference_grid_vis_ir, reference_grid_hrv ;

		struct msevi_l15_coverage planned_coverage_vis_ir;  
		struct msevi_l15_coverage planned_coverage_hrv_lower;
		struct msevi_l15_coverage planned_coverage_hrv_upper;

		struct _l15_image_production {
			uint8_t image_proc_direction;
			uint8_t pixel_gen_direction;
			uint8_t planned_chan_processing[MSEVI_NR_CHAN];
		} l15_image_production;
	} image_description;

	struct _radiometric_processing {
		struct _rp_summary {
			uint8_t radiance_linearization[MSEVI_NR_CHAN];
			uint8_t detector_equalization[MSEVI_NR_CHAN];
			uint8_t onboard_calibration_result[MSEVI_NR_CHAN];
			uint8_t mpef_cal_feedback[MSEVI_NR_CHAN];
			uint8_t mtf_adaption[MSEVI_NR_CHAN];
			uint8_t straylight_correction_flag[MSEVI_NR_CHAN];
		} rp_summary;

		struct _l15_image_calibration {
			double cal_slope;
			double cal_offset;
		} l15_image_calibration[MSEVI_NR_CHAN];

	} radiometric_processing;

	struct _geometric_processing {
		struct _opt_axis_distance {
			float ew_focal_plane[42];
			float ns_focal_plane[42];
		} opt_axis_distance;

		struct _earth_model {
			uint8_t type;
			double  equatorial_radius;
			double  north_polar_radius;
			double  south_polar_radius;
		} earth_model;

	} geometric_processing;

	struct _impf_configuration {

	} impf_configuration;

};

struct msevi_l15_trailer {

	uint8_t version;

	struct _image_production_stats{

		uint16_t satellite_id;

		struct _actual_scanning_summary {
			uint8_t nominal_image_scanning;
			uint8_t reduced_scan;
			struct cds_time forward_scan_start, forward_scan_end;
		} actual_scanning_summary;

		struct _radiometric_behaviour {
			uint8_t nominal_behaviour;
		} radiometric_behaviour;

		struct _reception_summary_stats {
			uint32_t planned_number_of_l10_lines[MSEVI_NR_CHAN];
			uint32_t number_of_missing_l10_lines[MSEVI_NR_CHAN];
			uint32_t number_of_corrupted_l10_lines[MSEVI_NR_CHAN];
			uint32_t number_of_replaced_l10_lines[MSEVI_NR_CHAN];
		} reception_summary_stats;

		struct _l15_image_validity {
			uint8_t nominal_image;
			uint8_t non_nominal_because_incomplete;
			uint8_t non_nominal_radiometric_quality;
			uint8_t non_nominal_geometric_quality;
			uint8_t non_nominal_timeliness;
			uint8_t non_nominal_incomplete_l15;
		} l15_image_validity[MSEVI_NR_CHAN];

		struct msevi_l15_coverage actual_coverage_vis_ir;
		struct msevi_l15_coverage actual_coverage_lower_hrv;
		struct msevi_l15_coverage actual_coverage_upper_hrv;

	} image_production_stats;
	
	struct _navigation_extraction_results {
	} navigation_extraction_results;

	struct _radiometric_quality {
	} radiometric_quality;

	struct _geometric_quality {
	} geometric_quality;

	struct _timeliness_and_completeness {
	} timeliness_and_completeness;
};

struct msevi_chaninf {
	char      name[8];
	uint16_t  id;
	double    cal_slope;
	double    cal_offset;
	double    nu_c;
	double    f0;
	double    alpha;
	double    beta;
};
 
struct msevi_l15_image *msevi_l15_image_alloc( int nlin, int ncol );
void msevi_l15_image_free( struct msevi_l15_image *img );

int msevi_get_chaninf( int sat_id, int chan_id, struct msevi_chaninf *ci );
const int msevi_chan2id( const char *chan );
const char* msevi_id2chan( int id );

#ifdef __cplusplus
}
#endif

#endif /* _MSEVI_L15_H_ */
