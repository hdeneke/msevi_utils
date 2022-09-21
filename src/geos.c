/**
 *  \file    geos.c
 *  \brief   geostationary satellite projection functions
 *
 *  \author  Hartwig Deneke
 *  \date    2011/07/02
 */

#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "mathutils.h"
#include "geos.h"

static void calc_mu_azi( float lat1, float lat2, float dlon,
			 float *mu, float *azi )
{

	float sin_lat1, sin_lat2, cos_lat1, cos_lat2;
	float sin_dlon, cos_dlon;
	float e, n, u;

	/* trig. funcs of lats/dlon */
	sincosf( lat1, &sin_lat1, &cos_lat1);
	sincosf( lat2, &sin_lat2, &cos_lat2);
	sincosf( dlon, &sin_dlon, &cos_dlon);

	/* calc. cosine of zenith angle plus azimuth angle */
	e = -cos_lat2*sin_dlon;
	n = -sin_lat1*cos_lat2*cos_dlon + cos_lat1*sin_lat2;
	u = sin_lat1*sin_lat2 + cos_lat1*cos_lat2*cos_dlon;
	*mu = u;
	*azi = RAD2DEG(atan2f(e,n));
	*azi = (*azi>=0.0)? *azi : *azi+360.0;
#if 0
	*mu  = sin_lat1*sin_lat2 + cos_lat1*cos_lat2*cos_dlon;
	*azi = RAD2DEG(asin( -cos_lat2*sin_dlon/sqrt(1.0-SQR(*mu)) ));
	if( sin_lat2 >= *mu*sin_lat1 ) {
		if( *azi < 0.0 ) *azi += 2.0*M_PI;
	} else {
		*azi = M_PI - *azi;
	}
	*azi = RAD2DEG(*azi);
#endif
	return;
}


/**
 * \brief init context and parameter info for geostationary sat. projection
 *
 * \param[in]  x0
 * \param[in]  y0
 * \param[in]  dx
 * \param[in]  dy
 *
 * \author Hartwig Deneke
 */
struct geos_param* geos_init( float x0, float y0, float dx, float dy )
{
	struct geos_param *param;

	/* allocate memory */
	param = calloc(1,sizeof(struct geos_param));
	if(param==NULL) goto err_out;

	/* set constants */
	param->h = 42164.0000;
	param->a =  6378.1690;
	param->b =  6356.5838;

	param->c1 = 1.006803;
	param->c2 = 0.00675701;
	param->c3 = 0.9993243;
	param->c4 = 0.02288276;

	param->proj_ss_lon = 0.0;
	param->true_ss_lon = 0.0;

	param->x0 = x0;
	param->y0 = y0;
	param->dx = dx;
	param->dy = dy;

	return param;

err_out:
	free(param);
	return NULL;
}

/**
 * \brief get lat/longitude for geostationary satellite projection
 *
 * \param[in]  gp    parameter settings
 * \param[out] lat   latitude [in degrees]
 * \param[out] lon   longitude [in degrees]
 *
 * \author Hartwig Deneke
 */
int geos_latlon2d( struct geos_param *gp, float sslon,
		   int nlin, int ncol, float *lat, float *lon )
{
	int i, l, c;
	float vsa, sin_vsa, cos_vsa;
	float hsa, sin_hsa, cos_hsa;
	float c1, p2, q, discr, gd;
	float x, y, z, rxy, nan=nanf("");

	for( l=0; l<nlin; l++ ) {

		/* calculate scan angle in vertical plane */
		vsa = gp->y0+gp->dy*l;
		sincosf(vsa, &sin_vsa, &cos_vsa);

		for( c=0; c<ncol; c++ ) {
			i = (l*ncol)+c;

			/* calculate scan angle in horizontal plane */
			hsa = gp->x0+gp->dx*c;
			sincosf(hsa, &sin_hsa, &cos_hsa);

			/* solve quad. equation for intersection with
			   earth */
			c1 = 1.0+(gp->c1-1.0)*SQR(sin_vsa);
			p2 = cos_vsa*cos_hsa/c1;
			q = (1.0-gp->c4)/c1;
			discr = SQR(p2)-q;
			if(discr<0.0) {
				lat[i] = nan;
				lon[i] = nan;
				continue;
			}
			gd = p2-sqrt(discr);

			/* calculate ECEF coordinates */
			x = gp->h*(1.0-gd*cos_hsa*cos_vsa);
			y = gp->h*gd*sin_hsa*cos_vsa;
			z = gp->h*gd*sin_vsa;

			/* transform to geodetic coordinates */
			rxy = hypot(x,y);
			lat[i] = RAD2DEG(atan(gp->c1*z/rxy));
			lon[i] = RAD2DEG(atan(y/x));
			lon[i] += sslon;
		}
	}
	return 0;
}

/**
 * \brief get satellite position in cosine zenith and azimuth angle
 *
 * \param[in]  gp    parameter settings
 * \param[in]  sslon true satellite sub-satellite longitude
 * \param[in]  nlin  number of lines
 * \param[in]  ncol  number of columnes
 * \param[in]  lat   latitude [in degrees]
 * \param[in]  lon   longitude [in degrees]
 * \param[out] muS   cosine of zenith angle [-]
 * \param[out] azS   azimuth angle [in degrees]
 *
 * \author Hartwig Deneke
 */
int geos_satpos2d( struct geos_param *gp, float sslon, int nlin, int ncol,
		   float *lat, float *lon, float *muS, float *azS )
{
	int i, l, c;
	float x, y, z;
	float clat, sin_clat, cos_clat;
	float dlon, sin_dlon, cos_dlon;
	float re, slat, slon;
	float nan=nanf("");

	for( l=0; l<nlin; l++ ) {
		for( c=0; c<ncol; c++ ) {
			i = l*ncol+c;

			/* test if we have valid input data */
			if( lat[i]==nan || lon[i]==nan ) {
				muS[i] = nan;
				azS[i] = nan;
				continue;
			}

			/* calculate ECEF coordinates of satellite */
			clat = atanf( gp->c3*tanf( DEG2RAD(lat[i]) ) );
			sincosf(clat, &sin_clat, &cos_clat);

			dlon = DEG2RAD(lon[i] - sslon);
			sincosf(dlon, &sin_dlon, &cos_dlon);

			re = gp->b/sqrtf(1.0-gp->c2*SQR(cos_clat));

			/* calculate ECEF coordinates of obs. point */
			x = re*cos_clat*cos_dlon;
			y = re*cos_clat*sin_dlon;
			z = re*sin_clat;

			/* calculate angles to sat from obs. point */
			slat = atanf( -z/hypotf(y,(gp->h-x)) );
			slon = atanf( -y/(gp->h-x) );

			calc_mu_azi( DEG2RAD(lat[i]), slat, dlon-slon,
				     muS+i, azS+i );
		}
	}
	return 0;
}
