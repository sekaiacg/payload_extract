#ifndef PAYLOAD_EXTRACT_DEFS_H
#define PAYLOAD_EXTRACT_DEFS_H

#include <unistd.h>

#ifndef O_BINARY
#define O_BINARY    0
#endif

#if defined(__APPLE__)
#define off64_t off_t
#endif

#if defined(HAVE_FTRUNCATE64)
#define payload_ftruncate ftruncate64
#elif defined(HAVE_FTRUNCATE)
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
