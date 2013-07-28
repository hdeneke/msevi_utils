/** 
 *  \file    memutils.c
 *  \brief   Utility functions for memory handling
 *
 *  \author  Hartwig Deneke
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "memutils.h"


/**
 * \brief dereference a pointer n times
 *  
 * \param[in]  p   the pointer to dereference
 * \param[in]  n   the number of times to dereference the pointer
 *
 * \author Hartwig Deneke
 */
void *deref_ptr(void *p, int n)
{
	int i;

	for (i=0; i<n; i++) {
		p = *((void **)p);
	}
	return p;
}


/**
 * \brief allocate a 2D array
 * 
 * \param[in]  nx     the size of the first (fast-changing, right-most) dimension 
 * \param[in]  ny     the size of the last  (slow-changing, left-most) dimension 
 * \param[in]  size   the size of the objects in the array
 *
 * \return pointer to the 2D array
 * 
 * \author Hartwig Deneke
 */
void *calloc_2d (size_t nx, size_t ny, size_t size)
{
	void **retval;
        int i;

        /* allocate memory for pointers and data at once */
        retval = (void **) calloc( 1,  ny *( sizeof(void *) + size * nx ) );
	if ( retval ){
                /* initialize pointers to point to each row of data */
                retval[0] = (void *) (retval + ny );
                for( i=1; i < ny; i++ ){
                        retval[i] = retval[i-1] + nx * size;
                }
        }
        return retval;
}

/**
 * static helper function to allocate an n-dimensional array
 */
static void init_pointers (void **p1, size_t nmemb, void *p2, size_t shift)
{
        int i;

        for( i=0;i<nmemb; i++ ){
                p1[i] = p2 + i * shift;
        }
        return;
}


/**
 * \brief allocate an n-dimensional array 
 *
 * \param[in]  ndim    the number of dimensions
 * \param[in]  dim     an array with the size of the dimensions
 * \param[in]  size    the size of the objects in the array
 *
 * \return pointer to the n-dim array
 *
 * \author Hartwig Deneke
 */
void *calloc_ndim (size_t ndim, size_t *dim, size_t size)
{
        size_t *nr_obj;      /* nr of objects per indirection level */
        size_t mem_size=0;   /* total size required */
        int i;
        size_t obj_size;
        void *data;
        void **p1;
        void *p2;

        nr_obj = calloc (ndim+1, sizeof(size_t));

        /* calculate nr of pointers per level of dimension  */
        *nr_obj = 1; 
        nr_obj++;    /*  re-arrange so that nr_obj[-1] is 1 */

        for( i=0; i<ndim; i++ ){
		/* calculate nr of objects per level of dimension */
                nr_obj[i] = dim[i] * *(nr_obj+i-1); 
        }

        /* calculate size of memory for both pointers and data */
        for( i=0; i<ndim; i++ ){
                /* size of object at level (pointer or data?) */
                obj_size = ( i == ndim-1)? size :sizeof( void *);
		/* sum up required memory */
                mem_size += nr_obj[i] * obj_size;
        }

        /* allocate data */
        data =  calloc( mem_size, 1 );
        if( !data ){
                goto err_exit;
        }

        /* initialize pointer chains */
        p1 = (void **)data;
        for( i=0; i<ndim-1; i++ ){
                p2 = ((void *)p1) + nr_obj[i] * sizeof(void *);
                obj_size = ( i == ndim-2)? size : sizeof( void *);
                init_pointers( p1, nr_obj[i], p2,  obj_size * dim[i+1] );
                p1 = (void **) p2;
        }
err_exit:
        free( --nr_obj );
        return data;
}


/**
 * \brief free array of dynamically allocated pointers
 *
 * \param[in]   n        the number of pointers in the array
 * \param[in]   ptr_arr  the  pointer to the array of pointers to free
 * 
 * \author Hartwig Deneke
 */
void free_ptr_array(size_t n, void **ptr_arr)
{
	int i;
	
	if (ptr_arr) {
		for (i=0; i<n; i++)
			free (ptr_arr[i]);
		free (ptr_arr);
	}
	return;
}


/**
 * \brief    unpack 10bit data elements to 16bit values
 *
 * \param[in]   src   the input buffer containing 10bit data
 * \param[out]  dest  the output buffer to containt the 16bit data
 * \param[in]   off   start offset in number of 10bit elements
 * \param[in]   cnt   the number of data elements to unpack
 *
 * \return          0 on success or -1 on failure
 *
 * \author          Hartwig Deneke
 */
void unpack_10bit_to_16bit (void *src, uint16_t *dest, size_t off, size_t cnt)
{
	int i, r, soff;
	int loff[]        = {     0,      1,       2,     3};
	uint16_t lmask[]  = {0xFFC0, 0x3FF0, 0x0FFC, 0x03FF};
	uint8_t  lshift[] = {     6,      4,      2,      0};
	uint8_t *s=src;

	for (i=0; i<cnt; i++){
		/* remainder of div 4 is used as index to lookup tables 
		   for offset/mask/shift */
		r = (off+i) % 4;              
		soff = (off+i)/4*5+loff[r];
		dest[i] = ((uint16_t)s[soff]<<8) + s[soff+1];
		dest[i] &= lmask[r];
		dest[i] >>= lshift[r];
 	}
	return;
}
