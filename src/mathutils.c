/**
 *  \file    mathutils.c
 *  \brief   Utility mathematical functions
 *
 *  \author  Hartwig Deneke
 *  \date    2008/06/10
 */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "mathutils.h"


/**
 * \brief  for an increasing array of floats, find the index of the member
 *         less than a given value
 *
 * \param[in]  value    the value to find
 * \param[in]  array    the array to search the value in
 * \param[in]  nr       the length of the array
 *
 * \param[out] ilower   index to the next-lower array member
 * \param[in]  weight   weighting factor between lower and upper array member
 *
 * \author Hartwig Deneke
 */
int bracket (float value, float *array, const int nr, int *ilower, float *weight)
{

	int i_lo = 0;
	int i_hi = nr-1;

	/* do binary search */
	while (i_hi > i_lo+1){
		int i = (i_hi + i_lo ) / 2;
		if ( array[i] > value ){
			i_hi = i;
		}else{
			i_lo = i;
		}
	}
	*ilower = i_lo;
	if( weight != NULL ){
	  *weight = (value - array[i_lo])/
	    (array[i_hi] - array[i_lo]);
	}
	return 0;
}


/**
 * \brief Find the minimum and maximum values of a float array
 *
 * \param[in]  v      the array
 * \param[in]  nv     the length of the array
 * \param[out] vmin   the minimum value
 * \param[out] vmax   the maximum value
 *
 * \author Hartwig Deneke
 */
void get_min_max(float *v,const int nv, float *vmin,float *vmax)
{
	int i;
	*vmin=*vmax=v[0];
	for(i=1;i<nv;i++){
		*vmin = (v[i]< *vmin) ? v[i] : *vmin;
		*vmax = (v[i]> *vmax) ? v[i] : *vmax;
	}
	return;
}


/**
 * \brief get indices and weights for n-dimensional linear interpolation
 *
 * \param[in]  ndim       the number of dimensions
 * \param[in]  ivec       index vector along each of the dimensions
 * \param[in]  wvec       weight vector along each of the dimensions
 * \param[in]  shift      the shift vector for the dimensions
 * \param[out] ivec_exp   index vector of contributing points
 * \param[out] wvec_exp   weight vector of contributing points
 *
 * \author Hartwig Deneke
 */
void ndim_lin_interp_get_idx_wght(const int ndim, const int *ivec,
				  const float *wvec, const int *shift,
				  int *ivec_exp, float *wvec_exp)
{
	int i,j,n;
	int ishift;
/* 	float wtest; */

	/* being clever to avoid recomputations */
	wvec_exp[0]=1.0;
	ivec_exp[0]=0;
	for(i=0;i<ndim;i++){
		n = 1 << i;  // efficient way to calc pow(2,i),
		//   which is number of elements in previous iteration
		ishift = ivec[i] * shift[i];
		for(j=0; j < n; j++){
			wvec_exp[j+n] = wvec_exp[j] * wvec[i];
			wvec_exp[j] *= (1.0 - wvec[i]);

			ivec_exp[j] += ishift;
			ivec_exp[j+n] = ivec_exp[j] + shift[i];
		}
	}
	/*wtest=0;
	for(i=0;i< (1<<ndim); i++ ){
	  printf("%d %f\n",ivec_exp[i],wvec_exp[i]);
	  wtest+= wvec_exp[i] ;
	  }
	  printf( "%f\n",wtest);*/
	/* naive imlementation follows for testing*/
	/*ishift = 0;
	n = 1 << ndim;
	wtest=1.0;
	for(i=0;i< n ;i++){
		for(j=0; j<ndim ;j++ ){
			ishift += ivec[j] * shift[j];
			if ( i &  (1<<j)  ){
				wtest *= wvec[j];
				ishift += shift[j];
			}else{
				wtest *= 1.0 - wvec[j];
			}
		}
		//assert( ivec_exp[i] == ishift );
		//assert( fabs(wvec_exp[i] -wtest)<  0.01 );
		ivec_exp[i] = ishift;
		wvec_exp[i] = wtest;
	}
	*/

	return;
}


int almost_equal(float A,float B, int maxUlps)
{
  /* compare two floating point values,
     code based on
     http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
     maxUlps is the number of float values laying between
     A and B
  */

  int aInt,bInt,intDiff;

  //assert(maxUlps > 0 && maxUlps < 4 * 1024 * 1024);

  aInt = *(int*)(void *)&A;
  if (aInt < 0)
    aInt = 0x80000000 - aInt;

  // Make bInt lexicographically ordered as a twos-complement int

  bInt = *(int*)(void *)&B;
  if (bInt < 0)
    bInt = 0x80000000 - bInt;

  intDiff = abs(aInt - bInt);
  if (intDiff <= maxUlps)

    return 1;

  return 0;

}

void enclosing_indices( int n, float *x, float xmin, float xmax,
			int *ilower, int *ihigher)
{
  bracket( xmin, x, n, ilower, NULL );
  bracket( xmax, x, n, ihigher, NULL );
  *ihigher = *ihigher +1;
  return;
}

float polint(int n, float *xi, float *yi, float x)
{
	const int maxdeg = 3; /*maximum degree of intp. polynomial */
	float p[maxdeg];
	int i, m;

	/* Implement Neville's algorithm for evaluating
	   interpolating polynomial */
	for(i=0;i<n;i++) p[i]=yi[i];
	for(m=1;m<n;m++){
		for(i=0;i<(n-m);i++){
			p[i] =    ((x-xi[i+m])*p[i] + (xi[i]-x)*p[i+1])
				/ (xi[i]-xi[i+m]);
		}
	}
	return p[0];
}
