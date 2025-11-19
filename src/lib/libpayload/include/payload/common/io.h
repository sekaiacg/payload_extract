#ifndef PAYLOAD_EXTRACT_IO_H
#define PAYLOAD_EXTRACT_IO_H

#include <cerrno>
#include <cinttypes>
#include <cstring>
#include <fcntl.h>
#include <string>

#include "ioDefs.h"

namespace skkk {
	int openFileRD(const std::string &path);

	int openFileRW(const std::string &path);

	void closeFd(int &fd);

	int blobRead(int fd, void *data, uint64_t offset, uint64_t length);

	int blobWrite(int fd, const void *data, uint64_t offset, uint64_t length);

	int blobFallocate(int fd, off64_t offset, off64_t length);

	bool readToString(const std::string &filePath, std::string &result);
}

#endif //PAYLOAD_EXTRACT_IO_H
