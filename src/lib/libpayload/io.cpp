#include "payload/defs.h"
#include "payload/io.h"

namespace skkk {
	int blobRead(int fd, void *buf, uint64_t offset, size_t len) {
		ssize_t read_count = 0;

		if (!buf) {
			return -EINVAL;
		}

		while (len > 0) {
			read_count = payload_pread(fd, buf, len, offset);
			if (read_count < 1) {
				if (!read_count) {
					memset(buf, 0, len);
					return 0;
				} else if (errno != EINTR) {
					return -errno;
				}
			}
			offset += read_count;
			len -= read_count;
			buf = static_cast<char *>(buf) + read_count;
		}
		return 0;
	}

	int blobWrite(int fd, const void *buf, uint64_t offset, size_t len) {
		if (!buf) {
			return -EINVAL;
		}

		int ret = payload_pwrite(fd, buf, len, offset);
		if (ret != len) {
			if (ret < 0) {
				return -errno;
			}
			return -ERANGE;
		}
		return 0;
	}
}
