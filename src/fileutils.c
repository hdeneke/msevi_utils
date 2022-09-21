/**
    \file    fileutils.c
    \brief   Utility functions for file handling

    \author  Hartwig Deneke
    \date    2008/06/10
*/

/* include system headers */
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef CPP_ENABLE_GZIP
#include <zlib.h>
#endif

/* include project headers */
#include "fileutils.h"


/******* local forward declarations ******************************************/
static int checkperm (const char *path, const char *perm);
/******* end local forward declarations **************************************/


/**
 * \brief return information about a directory
 *
 * \param[in]  path    the path pointing to the file
 * \param[out] perm    a character array of file permissions to test,
 *                     (r=read, w=write, x=execute), or NULL if no permissions
 *                     should be tested
 *
 * \return zero on success, otherwise -1 and errno is set appropriately
 *
 * \author Hartwig Deneke
 */
int dir_info (const char *path, const char *perm)
{
	int retval;
	struct stat stat_data;

	/* fill-in stat structure */
	retval = stat (path, &stat_data);
	if (retval < 0) goto ret_out;

	/* check if we have a regular file */
	if (!S_ISDIR(stat_data.st_mode)) {
		retval = -1;
		goto ret_out;
	}

	/* check file permissions if requested */
	if (perm) {
		retval = checkperm (path, perm);
	}

	/* return to caller */
ret_out:
	return retval;
}


/**
 * \brief return information about a regular file
 *
 * \param[in]  path    the path pointing to the file
 * \param[out] perm    a character array of file permissions to test,
 *                     (r=read, w=write, x=execute), or NULL if no permissions
                       should be tested
 * \param[out] size    the size of the file, or NULL
 *
 * \return zero on success, otherwise -1 and errno is set appropriately
 */
int file_info (const char *path, const char *perm, off_t *size)
{
	int retval;
	struct stat stat_data;

	/* fill-in stat structure */
	retval = stat (path, &stat_data);
	if (retval < 0) goto ret_out;

	/* check if we have a regular file */
	if (!S_ISREG(stat_data.st_mode)) {
		retval = -1;
		goto ret_out;
	}

	/* return size if requested */
	if (size) {
		*size = stat_data.st_size;
	}

	/* check file permissions if requested */
	if (perm) {
		retval = checkperm (path, perm);
	}

	/* return to caller */
ret_out:
	return retval;
}


/**
 * \brief reads in all the data from a binary file into allocated memory
 *
 * \param[in]  fname     the name of the file to read
 * \param[out] data      a pointer to store the pointer to the data
 * \param[in]  size      the number of bytes read
 *
 * \return               0 on success, -1 on error
 */
int fread_binary (const char *fname, void *data, size_t size)
{
	int fd, retval=0;
	ssize_t fsize, cnt, len;

	/* check file-permission and -size */
	retval = file_info (fname, "r", &fsize);
	if (retval < 0) goto err_out;

	len = strlen(fname);
	if (    strncmp (fname+len-3,".gz", 3) != 0
	     && strncmp (fname+len-3,".GZ", 3) != 0) {
		/* open file and read data */
		fd = open (fname, O_RDONLY);
		if (fd < 0) goto err_out;

		cnt = read (fd, data, size);
		if (cnt < size) retval = -1;

		close (fd);
	} else {
#ifdef CPP_ENABLE_GZIP
		gzFile *fp;

                fp = gzopen (fname, "r");
		if (fp == NULL) goto err_out;

                cnt = gzread (fp, data, size);
                gzclose (fp);
		if (cnt < size) retval = -1;
#endif
#ifndef CPP_ENABLE_GZIP
		fprintf(stderr, "Trying to read GZIP compressed file %s"
			 ", and GZIP support not enabled\n", fname );
		goto err_out;
#endif
	}

err_out:
	/* return to caller */
	return retval;
}


/**
 * \brief local helper function to check file permissions
 */
static int checkperm (const char *path, const char *perm)
{
	int i, mode = 0;

	for (i=0; i<strlen(perm); i++) {
		switch (perm[i]) {
		case 'r':
			mode &= R_OK;
			break;
		case 'w':
			mode &= W_OK;
			break;
		case 'x':
			mode &= X_OK;
			break;
		default:
			/*ignore unknown permission characters */
			break;
		}
	}
	return access (path, mode);
}
