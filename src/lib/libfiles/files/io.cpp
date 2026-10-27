#include <cinttypes>
#include <climits>
#include <fcntl.h>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <windows.h>
#endif

#include "files/io.h"

namespace skkk {
	bool IO::dirExists(const std::string &path) {
		struct stat st = {};
		if (stat(path.c_str(), &st) == 0) {
			return S_ISDIR(st.st_mode);
		}
		return false;
	}

	bool IO::fileExists(const std::string &path) {
		struct stat st = {};
		if (stat(path.data(), &st) == 0) {
			return !S_ISDIR(st.st_mode);
		}
		return false;
	}

	uint64_t IO::getFileSize(const std::string &dirPath) {
		struct stat st = {};
		if (stat(dirPath.c_str(), &st) == 0) {
			return st.st_size;
		}
		return 0;
	}

	int IO::mkdirs(const std::string &path, mode_t mode) {
		int err = 0;
		size_t len = 0;
		char str[PATH_MAX + 1] = {};
		std::strncpy(str, path.c_str(), PATH_MAX);
		len = std::strlen(str);
		for (int i = 0; i < len; i++) {
#if defined(_WIN32) || defined(_WIN64)
			if (str[i] == '/' && i > 0) {
#else
			if (str[i] == '/' && i > 0 && str[i - 1] != ':') {
#endif
				str[i] = '\0';
				if (access(str, F_OK) != 0) {
#if defined(_WIN32) || defined(_WIN64)
					err = mkdir(str) == 0 ? 0 : -errno;
#else
					err = mkdir(str, mode) == 0 ? 0 : -errno;
#endif
					if (err) return err;
				}
				str[i] = '/';
			}
		}
		if (len > 0 && access(str, F_OK) != 0) {
#if defined(_WIN32) || defined(_WIN64)
			err = mkdir(str) == 0 ? 0 : -errno;
#else
			err = mkdir(str, mode) == 0 ? 0 : -errno;
#endif
		}
		return err;
	}

	int IO::createFile(const std::string &path, bool isTruncate) {
		int flags = O_CREAT | O_RDWR | O_BINARY | O_TRUNC;
		if (!isTruncate) flags &= ~O_TRUNC;
		int fd = open(path.c_str(), flags, 0644);
		return fd > 0 ? fd : -errno;
	}

	int IO::openFileRD(const std::string &path) {
		int fd = open(path.c_str(), O_RDONLY | O_BINARY);
		return fd > 0 ? fd : -errno;
	}

	int IO::openFileRW(const std::string &path) {
		int fd = open(path.c_str(), O_RDWR | O_BINARY);
		return fd > 0 ? fd : -errno;
	}

	void IO::closeFd(int &fd) {
		if (fd > 0) {
			close(fd);
			fd = -1;
		}
	}

	FILE *IO::fopenFileRW(const std::string &path) {
		return std::fopen(path.c_str(), "wb");
	}

	void IO::fclose(FILE *&file) {
		if (file) {
			std::fclose(file);
			file = nullptr;
		}
	}

#if defined(_WIN32) || defined(_WIN64)
	static ssize_t io_pread(int fd, void *data, uint64_t pos, size_t len) {
		HANDLE hFile = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
		if (hFile == INVALID_HANDLE_VALUE) {
			return -EBADF;
		}
		OVERLAPPED overlapped = {};
		SSIZE_T read_total = 0;
		DWORD bytes_to_read = 0;
		DWORD bytes_read = 0;
		BOOL success = FALSE;
		DWORD error = 0;
		do {
			overlapped.Internal = 0;
			overlapped.InternalHigh = 0;
			overlapped.Offset = static_cast<DWORD>(pos & 0xFFFFFFFF);
			overlapped.OffsetHigh = static_cast<DWORD>((pos >> 32) & 0xFFFFFFFF);
			bytes_to_read = static_cast<DWORD>(len - read_total);
			bytes_read = 0;
			success = ReadFile(hFile, data, bytes_to_read, &bytes_read, &overlapped);
			if (!success) {
				error = GetLastError();
				if (error == ERROR_HANDLE_EOF) {
					break;
				}
				if (error == ERROR_RETRY) {
					continue;
				}
				return -static_cast<ssize_t>(error);
			}
			if (bytes_read == 0) {
				break;
			}
			data = static_cast<uint8_t *>(data) + bytes_read;
			pos += bytes_read;
			read_total += bytes_read;
		} while (static_cast<size_t>(read_total) < len);
		return read_total;
	}

	static ssize_t io_pwrite(int fd, const void *data, uint64_t pos, size_t len) {
		HANDLE hFile = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
		if (hFile == INVALID_HANDLE_VALUE) {
			return -EBADF;
		}
		OVERLAPPED overlapped = {};
		ssize_t write_total = 0;
		DWORD bytes_to_write = 0;
		DWORD bytes_written = 0;
		BOOL success = FALSE;
		DWORD error = 0;
		do {
			overlapped.Internal = 0;
			overlapped.InternalHigh = 0;
			overlapped.Offset = static_cast<DWORD>(pos & 0xFFFFFFFF);
			overlapped.OffsetHigh = static_cast<DWORD>((pos >> 32) & 0xFFFFFFFF);
			bytes_to_write = static_cast<DWORD>(len - write_total);
			bytes_written = 0;
			success = WriteFile(hFile, data, bytes_to_write, &bytes_written, &overlapped);
			if (!success) {
				error = GetLastError();
				if (error == ERROR_RETRY) {
					continue;
				}
				if (error == ERROR_DISK_FULL || error == ERROR_HANDLE_DISK_FULL) {
					return -ENOSPC;
				}
				return -static_cast<ssize_t>(error);
			}
			if (bytes_written == 0 && bytes_to_write > 0) {
				break;
			}
			data = static_cast<const uint8_t *>(data) + bytes_written;
			pos += bytes_written;
			write_total += bytes_written;
		} while (static_cast<size_t>(write_total) < len);
		return write_total;
	}
#else
	static ssize_t io_pread(int fd, void *data, uint64_t pos, size_t len) {
		ssize_t ret, read = 0;
		do {
			ret = pread(fd, data, len, static_cast<off_t>(pos));
			if (ret <= 0) {
				if (!ret)
					break;
				if (errno != EINTR) {
					return -errno;
				}
				ret = 0;
			}
			data = static_cast<uint8_t *>(data) + ret;
			pos += ret;
			read += ret;
		} while (read < len);
		return read;
	}

	static ssize_t io_pwrite(int fd, const void *data, uint64_t pos, size_t len) {
		ssize_t ret, written = 0;
		do {
			ret = pwrite(fd, data, len, static_cast<off_t>(pos));
			if (ret <= 0) {
				if (!ret)
					break;
				if (errno != EINTR) {
					return -errno;
				}
				ret = 0;
			}
			data = static_cast<const uint8_t *>(data) + ret;
			pos += ret;
			written += ret;
		} while (written < len);
		return written;
	}
#endif

	int IO::blobRead(int fd, void *outData, uint64_t offset, size_t length) {
		if (!outData) {
			return -EINVAL;
		}
		return io_pread(fd, outData, offset, length) != length ? -EIO : 0;
	}

	int IO::blobWrite(int fd, const void *srcData, uint64_t offset, size_t length) {
		if (!srcData) {
			return -EINVAL;
		}
		return io_pwrite(fd, srcData, offset, length) != length ? -EIO : 0;
	}

	int IO::blobFtruncate(int fd, uint64_t length) {
		int ret = ftruncate(fd, static_cast<off_t>(length));
		return ret == 0 ? 0 : -errno;
	}

	bool IO::readToString(const std::string &filePath, std::string &result) {
		int ret = -1, inFd = -1;
		inFd = openFileRD(filePath);
		if (inFd > 0) {
			uint64_t size = getFileSize(filePath);
			if (size > 0) {
				result.resize(size, 0);
				ret = blobRead(inFd, result.data(), 0, size) == 0;
			}
			closeFd(inFd);
		}
		return ret == 0;
	}

	bool IO::readAllLines(const std::string &filePath, std::vector<std::string> &result) {
		std::ifstream file{filePath};
		std::string str;
		while (std::getline(file, str)) {
			result.emplace_back(str);
		}
		return !result.empty();
	}
}
