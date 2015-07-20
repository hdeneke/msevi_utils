#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <endian.h>

#include "memcpy_endian.h"
#include "cgms_xrit.h"

/**
 * \brief  Close a XRIT file
 *
 * \param[in]  file    the filename
 * \param[in]  mode    the mode for opening the file
 *
 * \return     a pointer to the XRIT file descriptor, or NULL on error
 */
struct xrit_file *xrit_fopen( char *file, char *mode )
{
	int r;
	uint8_t buf[32];
	struct xrit_file *xf;

	xf = calloc( 1, sizeof(*xf) );
	if( xf==NULL ) goto err_out;

	xf->fp = fopen( file, mode );
	if( xf->fp==NULL ) goto err_out;

	r = fread( buf, 16, 1, xf->fp );
	if(r<0) goto err_out;

	xf->ftype      = buf[3];
	xf->header_len = be32toh( *((uint32_t *)(buf+4)) );
	xf->data_len   = be64toh( *((uint64_t *)(buf+8)) );

	r = fseek( xf->fp, xf->header_len, SEEK_SET );
	if(r<0) goto err_out;

	return xf;

err_out:
	if( xf && (xf->fp!=NULL)) fclose(xf->fp);
	free(xf);
	return NULL;
}

/**
 * \brief  Close a XRIT file
 *
 * \param[in]  xf     the XRIT file to close
 *
 * \return     0 on success, or EOF on error
 */
int xrit_fclose( struct xrit_file *xf )
{
	int r;

	r = fclose( xf->fp ) ;
	free( xf );
	return r;
}

/**
 * \brief  Read the XRIT header
 *
 * \param[in]  xf     a XRIT file
 *
 * \return     an malloc'ed buffer with the XRIT header, or NULL on failure
 */
void *xrit_read_header( struct xrit_file *xf)
{
	size_t r;
	void *hdr;

	hdr  = calloc( 1, xf->header_len );
	if(hdr==NULL) goto err_out;

	fseek( xf->fp, 0, SEEK_SET );
	r = fread( hdr, xf->header_len, 1, xf->fp );
	if(r<1) goto err_out;

	return hdr;

err_out:
	free(hdr);
	return NULL;
}

/**
 * \brief  Read the XRIT data
 *
 * \param[in]  xf     a XRIT file
 *
 * \return     an malloc'ed buffer with the XRIT data, or NULL on failure
 */
void *xrit_read_data( struct xrit_file *xf)
{
	size_t r, nbyte;
	uint8_t  *data;

	nbyte   = (xf->data_len+7)/8;
	data  = calloc( 1, nbyte );
	if(data==NULL) goto err_out;

	fseek( xf->fp, xf->header_len, SEEK_SET );
	r = fread( data, nbyte, 1, xf->fp );
	if(r<1) goto err_out;

	return data;

err_out:
	free(data);
	return NULL;
}

/**
 * \brief  Locate a XRIT header record
 *
 * \param[in]  hdr        a pointer to the header
 * \param[in]  len        the length of the header
 * \param[in]  hrec_type  the header type to locate
 *
 * \return     the offset of the header record, in bytes
 */
void *xrit_find_hrec( void *hdr, size_t len, int hrec_type )
{
	void    *cur_hrec = hdr;
	uint8_t  cur_type;
	uint16_t cur_len;

	/* find header record */
	do{
		memcpy(&cur_type, cur_hrec, 1);
		memcpy_be16toh(&cur_len, cur_hrec+1, 1 );
		if(cur_type==hrec_type) break;
		cur_hrec += cur_len;
	}while( cur_hrec<(hdr+len) );

	if(cur_type==hrec_type) return cur_hrec;
	return NULL;
}

/**
 * \brief  Decode a XRIT header record
 *
 * \param[in]  hdr        a pointer to the header
 * \param[in]  len        the length of the header
 * \param[in]  hrec_type  the header type
 *
 * \return     an malloc'ed buffer containing the header record, or NULL
 *             on failure/if not found
 */
void *xrit_decode_hrec( void *hrec )
{
	uint8_t  hrec_type;
	uint16_t hrec_len;


	memcpy(&hrec_type, hrec, 1);
	memcpy_be16toh(&hrec_len, hrec+1, 1);

	switch (hrec_type) {
	case XRIT_HREC_PRIMARY: {
		struct xrit_hrec_primary *pri;

		pri = calloc(1,sizeof(struct xrit_hrec_primary));
		pri->hrec_type = hrec_type;
		pri->hrec_len = hrec_len;
		memcpy(&pri->file_type, hrec+3, 1);
		memcpy_be32toh(&pri->header_len, hrec+4, 1);
		memcpy_be64toh(&pri->data_len, hrec+8, 1);
		return pri;
	}
	case XRIT_HREC_IMAGE_STRUCTURE: {
		struct xrit_hrec_image_structure *is;

		is = calloc(1,sizeof(struct xrit_hrec_image_structure));
		is->hrec_type = hrec_type;
		is->hrec_len = hrec_len;
		memcpy(&is->bpp, hrec+3, 1);
		memcpy_be16toh(&is->ncol, hrec+4, 1);
		memcpy_be16toh(&is->nlin, hrec+6, 1);
		memcpy(&is->compression, hrec+8, 1);
		return is;
	}
	case XRIT_HREC_IMAGE_NAVIGATION: {
		struct xrit_hrec_image_navigation *in;

		in = calloc(1,sizeof(struct xrit_hrec_image_navigation));
		in->hrec_type = hrec_type;
		in->hrec_len  = hrec_len;
		memcpy(&in->projection, hrec+3, 32);
		memcpy_be32toh(&in->cfac, hrec+35, 1);
		memcpy_be32toh(&in->lfac,  hrec+39, 1);
		memcpy_be32toh(&in->coff, hrec+43, 1);
		memcpy_be32toh(&in->loff, hrec+47, 1);
		return in;
	}
	default:
		return NULL;
	}
}
