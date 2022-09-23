#define _GNU_SOURCE

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "mathutils.h"
#include "cds_time.h"
#include "sunpos.h"

/*********************************************************************
 * Functions for calculating the position of the sun, based on:      *
 * WMO Guide to instruments and methods of observation, 7th edition  *
 * Annex 7.d, which in turn cites Michalsky (1988a,b)                *
 *********************************************************************/

/* calc. mean solar longitude in degrees */
static inline float jday2mnlon( double jd )
{
	float mnlon;
	mnlon  = 280.460 + 0.9856474*jd;
	mnlon  = DEG2PVAL(mnlon);
	return(mnlon);
}

/* calc. mean solar anomaly in degrees */
static inline float jday2mnanom( double jd )
{
	float mnanom;
	mnanom = 357.528 + 0.9856003*jd;
	mnanom = DEG2PVAL(mnanom);
	return(mnanom);
}

/**
 * \brief  Calc. Greenwich mean sideral time
 *
 * \param[in] jd   The Julian day starting at epoch JD2000_0
 *
 * \return   Greenwich mean sideral time
 */
double jday2gmst(double jd)
{
	double hh;
	double gmst;
	hh = fmod(jd-0.5,1.0)*24;
	gmst = 6.697375 + 0.0657098242 * jd + hh;
	return gmst;
}

/**
 * \brief  Calc. declination and right ascenion of the sun
 *
 * \param[in]  jd      the julian day
 * \param[out] dec     the declination, in radians
 * \param[out] ra      the right ascension, in radians
 *
 * \return  nothing
 */
void sun_dec_ra( double jd, float *dec, float *ra )
{
	float mnlon, mnanom;
	float sin_mnanom, cos_mnanom;
	float eclon, sin_eclon, cos_eclon;
	float oblqec, sin_oblqec, cos_oblqec;
	float num, den;

	mnlon  = jday2mnlon(jd); 	    /* mean longitude in  degrees */
	mnanom = DEG2RAD(jday2mnanom(jd));  /* mean anomaly in radians    */

	/* ecliptic longitude and obliquity  *
	 * of ecliptic in radians            */

	/* OLD VERSION:
	   eclon = mnlon + 1.915*sin(mnanom) + 0.020*sin(2.*mnanom);
	   converted with: sin(2x)=2*sin(x)*cos(x)  */
	sincosf(mnanom, &sin_mnanom, &cos_mnanom);
	eclon = mnlon + sin_mnanom*(1.915 + 0.040*cos_mnanom);
	eclon  = DEG2RAD(eclon);

	oblqec = 23.439 - 0.0000004*jd;
	oblqec = DEG2RAD(oblqec);

	sincosf(eclon, &sin_eclon, &cos_eclon);
	sincosf(oblqec, &sin_oblqec, &cos_oblqec);

	/* declination */
	*dec = asinf(sin_oblqec*sin_eclon);

	/* right ascension */
	num = cos_oblqec * sin_eclon;
	den = cos_eclon;
	*ra = atan2f(num, den);
	if(*ra < 0.0) *ra += 2.*M_PI;

	return;
}

/**
 * \brief  Calc. the sun position for a specific time/location
 *
 * \param[in]  jd      the time as julian day
 * \param[in]  lat     the latitude [in degrees north]
 * \param[in]  lon     the longitude [in degrees east]
 * \param[out] mu0     the cosine of the solar zenith angle
 * \param[out] az0     the azimuth angle [in degrees]
 *
 * \return  nothing
 */
void sunpos ( double jd, float lat, float lon, float *mu0, float *az0 )
{
        float dec, sin_dec, cos_dec;
        float sin_lat, cos_lat, sin_lon, cos_lon;
        float ra, gmst;
        float sin_gha, cos_gha, sin_ha, cos_ha;

	/* calc. solar declination/right ascension */
	sun_dec_ra( jd, &dec, &ra );
	sincosf(dec, &sin_dec, &cos_dec);

	/* calculate Greenwich mean sideral time, hour angle */
	gmst = jday2gmst(jd);
	sincosf(DEG2RAD(gmst*15.0)-ra, &sin_gha, &cos_gha);

	/* trig. funcs of location */
	sincosf(DEG2RAD(lat), &sin_lat, &cos_lat);
	sincosf(DEG2RAD(lon), &sin_lon, &cos_lon);

	/* trig. funcs of hour angle */
	sin_ha = sin_gha*cos_lon + cos_gha*sin_lon;
	cos_ha = cos_gha*cos_lon - sin_gha*sin_lon;

	/* calc. sun position */
	*mu0 = sin_dec*sin_lat + cos_dec*cos_lat*cos_ha;
	*az0 = asin(-cos_dec*sin_ha/sqrt(1.0-SQR(*mu0)));
	if( sin_dec >= *mu0 * sin_lat ) {
		if( *az0 < 0.0 ) *az0 += 2.0*M_PI;
	} else {
		*az0 = M_PI - *az0;
	}
	*az0 = RAD2DEG(*az0);

        return;
}

/**
 * \brief  Calc. the sun position for a 2D satellite image
 *
 * \param[in]  jd      the time as julian day
 * \param[in]  dt      the time difference between successive lines [in days]
 * \param[in]  nlin    the number of lines of the image
 * \param[in]  ncol    the number of columns of the image
 * \param[in]  lat     the latitude [in degrees north]
 * \param[in]  lon     the longitude [in degrees east]
 * \param[out] zen0     the cosine of the solar zenith angle
 * \param[out] az0     the azimuth angle [in degrees]
 *
 * \return  nothing
 */
void sunpos2d ( struct cds_time *ct, int nlin, int ncol,
		float *lat, float *lon, uint16_t *zen0, uint16_t *az0 )
{
        int i, l, c;
        float dec, sin_dec, cos_dec;
        float sin_lat, cos_lat, sin_lon, cos_lon;
        float ra, gmst;
        float sin_gha, cos_gha, sin_ha, cos_ha;
	float mu0, azf;
	double jd;

        for( l=0; l<nlin; l++ ) {
		if(ct[l].days==0) continue;
		jd = (ct[l].days-15340)-0.5 + ct[l].msec/8.64e+07;
		//printf("%d %d %f\n",l, ct[l].days, jd);
                sun_dec_ra( jd, &dec, &ra );
                sincosf(dec, &sin_dec, &cos_dec);

                gmst = jday2gmst(jd);
                sincosf(DEG2RAD(gmst*15.0)-ra, &sin_gha, &cos_gha);

                for( c=0; c<ncol; c++ ) {

                        /* get index */
                        i = (l*ncol)+c;

			/* trig. funcs of location */
                        sincosf(DEG2RAD(lat[i]), &sin_lat, &cos_lat);
                        sincosf(DEG2RAD(lon[i]), &sin_lon, &cos_lon);

                        /* trig. funcs of hour angle */
                        sin_ha = sin_gha*cos_lon + cos_gha*sin_lon;
                        cos_ha = cos_gha*cos_lon - sin_gha*sin_lon;

                        /* calc. sun position */
                        mu0 = sin_dec*sin_lat + cos_dec*cos_lat*cos_ha;
                        azf = asin(-cos_dec*sin_ha/sqrt(1.0-SQR(mu0)));

                        if( sin_dec >= mu0*sin_lat ) {
                                if( azf < 0.0 ) azf += 2.0*M_PI;
                        } else {
                                azf = M_PI - azf;
                        }

			// printf("%f %f\n",RAD2DEG(acosf(mu0)),RAD2DEG(azf));

			zen0[i] = (uint16_t) roundf(RAD2DEG(acosf(mu0))*100.0);
			az0[i]  = (uint16_t) roundf(RAD2DEG(azf)*100.0);
		}
        }
        return;
}

/**
 * \brief  Calc. the distance between earth and sun
 *
 * \param[in]  jd      the time as julian day
 *
 * \return    the distance [in AU]
 */
float sun_earth_distance( double jd )
{
	float esd, g;

	g = DEG2RAD(jday2mnanom(jd));
	esd = 1.00014-0.01671*cosf(g)+0.00014*cosf(2.0*g);
	return( esd );
}
