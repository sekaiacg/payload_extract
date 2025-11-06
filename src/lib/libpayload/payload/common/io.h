#ifndef PAYLOAD_EXTRACT_IO_H
#define PAYLOAD_EXTRACT_IO_H

#include <fcntl.h>
#include <cerrno>
#include <cinttypes>
#include <cstring>

#include "ioDefs.h"

namespace skkk {
	int blobRead(int fd, void *data, uint64_t pos, uint64_t len);

	int blobWrite(int fd, const void *data, uint64_t pos, uint64_t len);

	int blobFallocate(int fd, off64_t offset, off64_t len);
}

#endif //PAYLOAD_EXTRACT_IO_H
