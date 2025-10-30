#include "payload/defs.h"
#include "payload/io.h"

namespace skkk {
	int blobRead(int fd, void *buf, uint64_t offset, uint64_t len) {
		int64_t ret = 0, read = 0;

		if (!buf) {
			return -EINVAL;
		}

		do {
			ret = payload_pread(fd, buf, len, static_cast<off64_t>(offset));
			if (ret <= 0) {
				if (!ret)
					break;
				if (errno != EINTR) {
					return -errno;
				}
				ret = 0;
			}
			buf = static_cast<char *>(buf) + ret;
			offset += ret;
			read += ret;
		} while (read < len);

		return read != len ? -EIO : 0;
	}

	int blobWrite(int fd, const void *buf, uint64_t offset, uint64_t len) {
		int64_t ret = 0, written = 0;

		if (!buf) {
			return -EINVAL;
		}

		do {
			ret = payload_pwrite(fd, buf, len, static_cast<off64_t>(offset));
			if (ret <= 0) {
				if (!ret)
					break;
				if (errno != EINTR) {
					return -errno;
				}
				ret = 0;
			}
			buf = static_cast<const char *>(buf) + ret;
			offset += ret;
			written += ret;
		} while (written < len);

		return written != len ? -EIO : 0;
	}
}
