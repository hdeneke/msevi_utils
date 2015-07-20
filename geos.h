/*****************************************************************************/
/**
  \file         geos.h
  \brief        include file for geos.c, see c-file for details
  \author       Hartwig Deneke
  \date         2011/07/02
 */
/*****************************************************************************/

#ifndef _GEOS_H_
#define _GEOS_H_

/***** datatype declarations  ************************************************/

/**
 * \struct geos_ctx
 * \brief structure with context/parameter info for geostationary satellite
 *        projection
 */
struct geos_param {
	float h;
	float a;
	float b;
	float c1, c2, c3, c4;
	float proj_ss_lon, true_ss_lon;
	float delta;
	int nlin, ncol, lin0, col0;
	float x0, y0, dx, dy;
};

/***** end datatype declarations  ********************************************/


/***** function prototypes ***************************************************/

struct geos_param* geos_init( float x0, float y0, float dx, float dy );
void geos_free( struct geos_param* gp );

int geos_latlon2d( struct geos_param *gp, float sslon, int nlin, int ncol,
		   float *lat, float *lon );
int geos_satpos2d( struct geos_param *gp, float sslon, int nlin, int ncol,
		   float *lat, float *lon, float *muS, float *azS );
/***** end function prototypes ***********************************************/

#endif /* _GEOS_H_ */
