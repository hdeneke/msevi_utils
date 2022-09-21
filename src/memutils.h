/*****************************************************************************/
/*!
  \file         memutils.h
  \brief        include file for memutils.c, see that file for details
  \author       Hartwig Deneke
  \date         2008/06/10
 */
/*****************************************************************************/

#ifndef memutils_h
#define memutils_h

#ifdef __cplusplus
extern "C" {
#endif

/***** MACRO definitions *****************************************************/
/**
  \def   ARRAY_LEN(a)
  \brief Macro to determine the length of a static array \a a
 */
#define ARRAY_LEN(a) (sizeof(a)/sizeof(*a))
/***** end MACRO definitions *************************************************/

/***** function prototypes ***************************************************/
void *calloc_2d(size_t nx, size_t ny, size_t size);
void *calloc_ndim(size_t ndim, size_t *dim, size_t size);
void *deref_ptr(void *p, int n);
void free_ptr_array(size_t n, void **ptr_arr);
void unpack_10bit_to_16bit (void *src, uint16_t *dest, size_t off, size_t cnt);
/***** end function prototypes ***********************************************/

#ifdef __cplusplus
}
#endif

#endif /* memutils_h */
