#include "defs.h"
#include "io.h"

namespace skkk {
	int openFileRD(const std::string &path) {
		return open(path.c_str(), O_RDONLY | O_BINARY);
	}

	int openFileRW(const std::string &path) {
		return open(path.c_str(), O_RDWR | O_BINARY);
	}

	void closeFd(int fd) {
		if (fd > 0) close(fd);
	}

	int blobRead(int fd, void *data, uint64_t offset, uint64_t length) {
		int64_t ret = 0, read = 0;

		if (!data) {
			return -EINVAL;
		}

		do {
			ret = payload_pread(fd, data, length, static_cast<off64_t>(offset));
			if (ret <= 0) {
				if (!ret)
					break;
				if (errno != EINTR) {
					return -errno;
				}
				ret = 0;
			}
			data = static_cast<char *>(data) + ret;
			offset += ret;
			read += ret;
		} while (read < length);

		return read != length ? -EIO : 0;
	}

	int blobWrite(int fd, const void *data, uint64_t offset, uint64_t length) {
		int64_t ret = 0, written = 0;

		if (!data) {
			return -EINVAL;
		}

		do {
			ret = payload_pwrite(fd, data, length, static_cast<off64_t>(offset));
			if (ret <= 0) {
				if (!ret)
					break;
				if (errno != EINTR) {
					return -errno;
				}
				ret = 0;
			}
			data = static_cast<const char *>(data) + ret;
			offset += ret;
			written += ret;
		} while (written < length);

		return written != length ? -EIO : 0;
	}

	int blobFallocate(int fd, off64_t offset, off64_t length) {
		int ret = payload_fallocate(fd, FALLOC_FL_ZERO_RANGE, offset, length);
		return ret;
	}
}
