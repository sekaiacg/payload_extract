#pragma once

#include <cerrno>
#include <string>
#include <cstdint>
#include <vector>
#include <sys/types.h>

#if defined(_WIN32) || defined(_WIN64)
#include <basetsd.h>
typedef SSIZE_T ssize_t;
#endif

#include "common/defs.h"

namespace skkk {
	class IO {
		public:
			static bool dirExists(const std::string &path);

			static bool fileExists(const std::string &path);

			static uint64_t getFileSize(const std::string &dirPath);

			static int mkdirs(const std::string &path, mode_t mode);

			static int createFile(const std::string &path, bool isTruncate = true);

			static int openFileRD(const std::string &path);

			static int openFileRW(const std::string &path);

			static void closeFd(int &fd);

			static FILE *fopenFileRW(const std::string &path);

			static void fclose(FILE *&file);

			static int blobRead(int fd, void *outData, uint64_t offset, size_t length);

			static int blobWrite(int fd, const void *srcData, uint64_t offset, size_t length);

			static int blobFtruncate(int fd, uint64_t length);

			static bool readToString(const std::string &filePath, std::string &result);

			static bool readAllLines(const std::string &filePath, std::vector<std::string> &result);
	};
}
