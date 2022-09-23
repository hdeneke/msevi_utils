/*****************************************************************************/
/**
  \file         sunpos.h
  \brief        include file for sunpos.c, see c-file for details
  \author       Hartwig Deneke
  \date         2012/06/21
 */
/*****************************************************************************/
#ifndef _SUNPOS_H_
#define _SUNPOS_H_

double jday2gmst( double jd );
void   sun_dec_ra( double jd, float *dec, float *ra );
void   sunpos( double jd, float lat, float lon, float *mu0, float *az0 );
void   sunpos2d ( struct cds_time *ct, int nlin, int ncol,
		  float *lat, float *lon, uint16_t *zen0, uint16_t *az0 );
float  sun_earth_distance( double jd );
#endif /* _SUNPOS_H_ */
