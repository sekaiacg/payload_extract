#ifndef PAYLOAD_EXTRACT_IO_H
#define PAYLOAD_EXTRACT_IO_H

#include <fcntl.h>
#include <cerrno>
#include <cinttypes>
#include <cstring>

namespace skkk {
	int blobRead(int fd, void *buf, uint64_t offset, uint64_t len);

	int blobWrite(int fd, const void *buf, uint64_t offset, uint64_t len);
}

#endif //PAYLOAD_EXTRACT_IO_H
