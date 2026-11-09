#pragma once

#include <cerrno>
#include <cstddef>
#include <cstdint>

#include <basetsd.h>
#include <io.h>
#include <windows.h>

namespace skkk {
	typedef SSIZE_T ssize_t;

	inline void setErrno(DWORD win_error) {
		switch (win_error) {
			case ERROR_RETRY:
				errno = EAGAIN;
				break;
			case ERROR_INVALID_HANDLE:
				errno = EBADF;
				break;
			case ERROR_ACCESS_DENIED:
				errno = EACCES;
				break;
			case ERROR_INVALID_PARAMETER:
				errno = EINVAL;
				break;
			case ERROR_OPERATION_ABORTED:
				errno = EINTR;
				break;
			default:
				errno = EIO;
				break;
		}
	}

	inline ssize_t win_pread(int __fd, void *__buf, size_t __nbytes, uint64_t __offset) {
		HANDLE hFile = reinterpret_cast<HANDLE>(_get_osfhandle(__fd));
		if (hFile == INVALID_HANDLE_VALUE) {
			errno = EBADF;
			return -1;
		}
		OVERLAPPED overlapped = {};
		overlapped.Offset = static_cast<DWORD>(__offset & 0xFFFFFFFF);
		overlapped.OffsetHigh = static_cast<DWORD>((__offset >> 32) & 0xFFFFFFFF);
		DWORD bytesRead = 0;
		BOOL result = ReadFile(hFile, __buf, static_cast<DWORD>(__nbytes), &bytesRead, &overlapped);
		if (!result) {
			DWORD win_error = GetLastError();
			if (win_error == ERROR_IO_PENDING) {
				result = GetOverlappedResult(hFile, &overlapped, &bytesRead, TRUE);
				if (!result) {
					win_error = GetLastError();
				} else {
					return static_cast<ssize_t>(bytesRead);
				}
			}
			if (win_error == ERROR_HANDLE_EOF) return 0;
			setErrno(win_error);
			return -1;
		}
		return static_cast<ssize_t>(bytesRead);
	}

	inline ssize_t win_pwrite(int __fd, void *__buf, size_t __nbytes, uint64_t __offset) {
		HANDLE hFile = reinterpret_cast<HANDLE>(_get_osfhandle(__fd));
		if (hFile == INVALID_HANDLE_VALUE) {
			errno = EBADF;
			return -1;
		}
		OVERLAPPED overlapped = {};
		overlapped.Offset = static_cast<DWORD>(__offset & 0xFFFFFFFF);
		overlapped.OffsetHigh = static_cast<DWORD>((__offset >> 32) & 0xFFFFFFFF);
		DWORD bytesWritten = 0;
		BOOL result = WriteFile(hFile, __buf, static_cast<DWORD>(__nbytes), &bytesWritten, &overlapped);
		if (!result) {
			DWORD win_error = GetLastError();
			if (win_error == ERROR_IO_PENDING) {
				result = GetOverlappedResult(hFile, &overlapped, &bytesWritten, TRUE);
				if (!result) {
					win_error = GetLastError();
				} else {
					return static_cast<ssize_t>(bytesWritten);
				}
			}
			setErrno(win_error);
			return -1;
		}
		return static_cast<ssize_t>(bytesWritten);
	}

	inline ssize_t io_pread(int fd, void *data, uint64_t pos, size_t len) {
		HANDLE hFile = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
		if (hFile == INVALID_HANDLE_VALUE) {
			return -EBADF;
		}
		OVERLAPPED overlapped = {};
		SSIZE_T read_total = 0;
		DWORD bytes_to_read = 0;
		DWORD bytes_read = 0;
		BOOL success = FALSE;
		DWORD win_error = 0;
		do {
			overlapped.Internal = 0;
			overlapped.InternalHigh = 0;
			overlapped.Offset = static_cast<DWORD>(pos & 0xFFFFFFFF);
			overlapped.OffsetHigh = static_cast<DWORD>((pos >> 32) & 0xFFFFFFFF);
			bytes_to_read = static_cast<DWORD>(len - read_total);
			bytes_read = 0;
			success = ReadFile(hFile, data, bytes_to_read, &bytes_read, &overlapped);
			if (!success) {
				win_error = GetLastError();
				if (win_error == ERROR_HANDLE_EOF) {
					break;
				}
				if (win_error == ERROR_RETRY) {
					continue;
				}
				setErrno(win_error);
				return -errno;
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

	inline ssize_t io_pwrite(int fd, const void *data, uint64_t pos, size_t len) {
		HANDLE hFile = reinterpret_cast<HANDLE>(_get_osfhandle(fd));
		if (hFile == INVALID_HANDLE_VALUE) {
			return -EBADF;
		}
		OVERLAPPED overlapped = {};
		ssize_t write_total = 0;
		DWORD bytes_to_write = 0;
		DWORD bytes_written = 0;
		BOOL success = FALSE;
		DWORD win_error = 0;
		do {
			overlapped.Internal = 0;
			overlapped.InternalHigh = 0;
			overlapped.Offset = static_cast<DWORD>(pos & 0xFFFFFFFF);
			overlapped.OffsetHigh = static_cast<DWORD>((pos >> 32) & 0xFFFFFFFF);
			bytes_to_write = static_cast<DWORD>(len - write_total);
			bytes_written = 0;
			success = WriteFile(hFile, data, bytes_to_write, &bytes_written, &overlapped);
			if (!success) {
				win_error = GetLastError();
				if (win_error == ERROR_RETRY) {
					continue;
				}
				if (win_error == ERROR_DISK_FULL || win_error == ERROR_HANDLE_DISK_FULL) {
					return -ENOSPC;
				}
				setErrno(win_error);
				return -errno;
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
}
