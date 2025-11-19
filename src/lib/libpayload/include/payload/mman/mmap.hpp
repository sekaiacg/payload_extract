#ifndef PAYLOAD_EXTRACT_MMAP_HPP
#define PAYLOAD_EXTRACT_MMAP_HPP

#include <cerrno>
#if !defined(_WIN32)
#include <sys/mman.h>
#else
#include "mman.h"
#endif

#include "payload/Utils.h"
#include "payload/common/io.h"

namespace skkk {
	inline void *mapByFd(int fd, uint64_t size, bool isRDOnly) {
		auto *data = static_cast<uint8_t *>(mmap(nullptr, size, isRDOnly ? PROT_READ : PROT_READ | PROT_WRITE,
		                                         isRDOnly ? MAP_PRIVATE : MAP_SHARED, fd, 0));
		if (data != MAP_FAILED) {
			return data;
		}
		return nullptr;
	}

	template<typename T>
	int mapSync(T *data, uint64_t dataSize) {
		if (data && dataSize > 0) {
			return msync(const_cast<uint8_t *>(data), dataSize, MS_SYNC);
		}
		return -1;
	}

	template<typename T>
	int unmap(T *&data, uint64_t size) {
		int ret = -1;
		if (data && size > 0) {
			ret = munmap(const_cast<uint8_t *>(data), size);
			data = nullptr;
		}
		return ret;
	}

	template<typename T>
	int mapByPath(int &fd, const std::string &path, T *&data, uint64_t &dataSize, bool isRDOnly) {
		int ret = 0;
		if (fd < 0) {
			isRDOnly ? fd = openFileRD(path) : fd = openFileRW(path);
		}
		if (fd > 0) {
			dataSize = getFileSize(path);
			data = static_cast<T *>(mapByFd(fd, dataSize, isRDOnly));
			if (!(dataSize > 0 && data)) {
				ret = -EIO;
			}
		} else {
			ret = -errno;
		}
		return ret;
	}

	inline int mapRdByPath(int &fd, const std::string &path, const uint8_t *&data, uint64_t &dataSize) {
		return mapByPath(fd, path, data, dataSize, true);
	}

	inline int mapRwByPath(int &fd, const std::string &path, uint8_t *&data, uint64_t &dataSize) {
		return mapByPath(fd, path, data, dataSize, false);
	}
}

#endif //PAYLOAD_EXTRACT_MMAP_HPP
