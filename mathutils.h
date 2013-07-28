/*****************************************************************************/
/**
  \file         mathutils.h
  \brief        include file for mathutils.c, see that file for details
  \author       Hartwig Deneke
  \date         2008/06/10
 */
/*****************************************************************************/

#ifndef _MATHUTILS_H_
#define _MATHUTILS_H_

/***** MACRO definitions *****************************************************/

/**
 * \def RAD2DEG(x)
 * \brief convert angle \a x from radians to degrees
 */
#define RAD2DEG(x) ( 180.0/ M_PI * (x) )

/**
 * \def DEG2RAD(x)
 * \brief convert angle \a x from degrees to radians
 */
#define DEG2RAD(x) ( M_PI/180.0 * (x) )

/**
 * \def MAX(x,y)
 * \brief the maximum of argunts \a x and \a y
 */
#define MAX(x,y) ((x) > (y) ? (x) : (y) )

/**
 * \def MIN(x,y)
 * \brief the maximum of argunts \a x and \a y
 */
#define MIN(x,y) ((x) < (y) ? (x) : (y) )

/**
 * \def SQRT(x)
 * \brief the squareroot of \a x 
 */
#define SQRT(x) pow((x),0.5)

/**
 * \def SQR(x)
 * \brief the square of \a x 
 */
#define SQR(x) ( (x)*(x) )
/***** end MACRO definitions *************************************************/


/***** datatype declarations  ************************************************/

/** 
 * \struct index_range 
 * \brief A structure for holding an index range
 */
struct index_range{
	int min; /**< minimum index */
	int max; /**< mainimum index */
};
/***** end datatype declarations  ********************************************/


/***** function prototypes ***************************************************/
int bracket(float value, float *array, const int nr, int *i_lower, 
	    float *weight);
void get_min_max(float *v,const int nv, float *vmin,float *vmax);
int almost_equal(float A,float B, int maxUlps);
void enclosing_indices(int n,float *x,float xmin,float xmax,
		       int *ilower,int *ihigher);
void ndim_lin_interp_get_idx_wght(const int ndim, const int *ivec, 
				  const float *wvec, const int *shift, 
				  int *ivec_exp, float *wvec_exp);
float polint(int n, float *xi, float *yi, float x);
/***** end function prototypes ***********************************************/

static inline double RAD2PVAL(double a){
        double z=fmod(a,2.0*M_PI);
        return (z<0.0) ?  z+2.0*M_PI : z;
}
static inline double DEG2PVAL(double a){
        double z=fmod(a,360.0);
        return (z<0.0) ?  z+360.0 : z;
}

#endif /* _MATHUTILS_H_ */
