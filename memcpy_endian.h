static inline void memcpy_be16toh( void *dest, void *src, size_t n )
{
#if __LITTLE_ENDIAN
	uint16_t val;
	size_t   i;
	uint16_t *s = src;
	uint16_t *d = dest;

	for( i=0; i<n; i++ ) {
		val = be16toh(*s);
		memcpy(d, &val, sizeof(uint16_t));
		s++; d++;
	}
#else
	memcpy(dest, src, n*sizeof(uint16_t));
#endif
	return;
}

static inline void memcpy_be32toh( void *dest, void *src, size_t n )
{
#if __LITTLE_ENDIAN
	uint32_t val;
	size_t   i;
	uint32_t *s = src;
	uint32_t *d = dest;

	for( i=0; i<n; i++ ) {
		val = be32toh(*s);
		memcpy(d, &val, sizeof(uint32_t));
		s++; d++;
	}
#else
	memcpy(dest, src, n*sizeof(uint32_t));
#endif
	return;
}

static inline void memcpy_be64toh( void *dest, void *src, size_t n )
{
#if __LITTLE_ENDIAN
	uint64_t val;
	size_t   i;
	uint64_t *s = src;
	uint64_t *d = dest;

	for( i=0; i<n; i++ ) {
		val = be64toh(*s);
		memcpy(d, &val, sizeof(uint64_t));
		s++; d++;
	}
#else
	memcpy(dest, src, n*sizeof(uint64_t));
#endif
	return;
}

#undef __LITTLE_ENDIAN
