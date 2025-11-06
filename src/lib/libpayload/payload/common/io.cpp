#include "defs.h"
#include "io.h"

namespace skkk {
	int blobRead(int fd, void *data, uint64_t pos, uint64_t len) {
		int64_t ret = 0, read = 0;

		if (!data) {
			return -EINVAL;
		}

		do {
			ret = payload_pread(fd, data, len, static_cast<off64_t>(pos));
			if (ret <= 0) {
				if (!ret)
					break;
				if (errno != EINTR) {
					return -errno;
				}
				ret = 0;
			}
			data = static_cast<char *>(data) + ret;
			pos += ret;
			read += ret;
		} while (read < len);

		return read != len ? -EIO : 0;
	}

	int blobWrite(int fd, const void *data, uint64_t pos, uint64_t len) {
		int64_t ret = 0, written = 0;

		if (!data) {
			return -EINVAL;
		}

		do {
			ret = payload_pwrite(fd, data, len, static_cast<off64_t>(pos));
			if (ret <= 0) {
				if (!ret)
					break;
				if (errno != EINTR) {
					return -errno;
				}
				ret = 0;
			}
			data = static_cast<const char *>(data) + ret;
			pos += ret;
			written += ret;
		} while (written < len);

		return written != len ? -EIO : 0;
	}

	int blobFallocate(int fd, off64_t pos, off64_t len) {
		int ret = payload_fallocate(fd, FALLOC_FL_ZERO_RANGE, pos, len);
		return ret;
	}
}
