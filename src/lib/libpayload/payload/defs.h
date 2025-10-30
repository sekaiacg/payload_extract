#ifndef PAYLOAD_EXTRACT_DEFS_H
#define PAYLOAD_EXTRACT_DEFS_H

//#include <cinttypes>
//#include <cstddef>
#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY    0
#endif

//#if defined(__APPLE__)
//#  define htobe16(x) OSSwapHostToBigInt16(x)
//#  define htole16(x) OSSwapHostToLittleInt16(x)
//#  define be16toh(x) OSSwapBigToHostInt16(x)
//#  define le16toh(x) OSSwapLittleToHostInt16(x)
//
//#  define htobe32(x) OSSwapHostToBigInt32(x)
//#  define htole32(x) OSSwapHostToLittleInt32(x)
//#  define be32toh(x) OSSwapBigToHostInt32(x)
//#  define le32toh(x) OSSwapLittleToHostInt32(x)
//
//#  define htobe64(x) OSSwapHostToBigInt64(x)
//#  define htole64(x) OSSwapHostToLittleInt64(x)
//#  define be64toh(x) OSSwapBigToHostInt64(x)
//#  define le64toh(x) OSSwapLittleToHostInt64(x)
//#elif defined (_WIN32)
//#  define htobe16(x) __builtin_bswap16(x)
//#  define htole16(x) (x)
//#  define be16toh(x) __builtin_bswap16(x)
//#  define le16toh(x) (x)
//#  define htobe32(x) __builtin_bswap32(x)
//#  define htole32(x) (x)
//#  define be32toh(x) __builtin_bswap32(x)
//#  define le32toh(x) (x)
//#  define htobe64(x) __builtin_bswap64(x)
//#  define htole64(x) (x)
//#  define be64toh(x) __builtin_bswap64(x)
//#  define le64toh(x) (x)
//#endif

#if defined(HAVE_FTRUNCATE64)
#define payload_ftruncate ftruncate64
#else
#define payload_ftruncate ftruncate
#endif

#if defined(HAVE_LSEEK64)
#define payload_lseek lseek64
#else
#define payload_lseek lseek
#endif

#if defined(HAVE_PREAD64)
#define payload_pread pread64
#else
#define payload_pread pread
#endif

#if defined(HAVE_PWRITE64)
#define payload_pwrite pwrite64
#else
#define payload_pwrite pwrite
#endif


#if defined(__APPLE__)

#define off64_t off_t

#endif

#if defined(_WIN32)

inline static ssize_t pread(int fd, void *buf, size_t n, off64_t offset) {
	off64_t cur_pos;
	ssize_t num_read;

	if ((cur_pos = payload_lseek(fd, 0, SEEK_CUR)) == (off64_t) -1)
		return -1;

	if (payload_lseek(fd, offset, SEEK_SET) == (off64_t) -1)
		return -1;

	num_read = read(fd, buf, n);

	if (payload_lseek(fd, cur_pos, SEEK_SET) == (off64_t) -1)
		return -1;
	return (ssize_t) num_read;
}

inline static ssize_t pwrite(int fd, const void *buf, size_t n, off64_t offset) {
	off64_t cur_pos;
	ssize_t num_written;

	if ((cur_pos = payload_lseek(fd, 0, SEEK_CUR)) == (off64_t) -1)
		return -1;

	if (payload_lseek(fd, offset, SEEK_SET) == (off64_t) -1)
		return -1;

	num_written = write(fd, buf, n);

	if (payload_lseek(fd, cur_pos, SEEK_SET) == (off64_t) -1)
		return -1;
	return (ssize_t) num_written;
}

#endif

#endif //PAYLOAD_EXTRACT_DEFS_H
