/*****************************************************************************/
/*!
  \file         fileutils.h
  \brief        include file for fileutils.c, see that file for details
  \author       Hartwig Deneke
  \date         2008/06/10
*/
/*****************************************************************************/

#ifndef FILEUTILS_H
#define FILEUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/***** MACRO definitions *****************************************************/

/**
 * \def file_exists(fn)
 * \brief checks whether a file with name fn exists
 */
#define  file_exists(fn) (0==file_info(fn,NULL,NULL))

/**
 * \def dir_exists(fn)
 * \brief checks whether a dirfile with name dn exists
 */
#define  dir_exists(dn) (0==dir_info(dn,NULL))

/**
 * \def file_size(fn,sptr)
 * \brief gets the size of file fn
 */
#define  file_size(fn,sizeptr) (0==file_info(fn,NULL,sizeptr))

/***** end MACRO definitions *************************************************/


/***** function prototypes ***************************************************/
int dir_info (const char *path, const char *perm);
int file_info (const char *path, const char *perm, off_t *size);
int fread_binary (const char *fname, void *data, size_t size);
/***** end function prototypes ***********************************************/

#ifdef __cplusplus
}
#endif

#endif /* fileutils.h */
