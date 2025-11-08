#ifndef PAYLOAD_EXTRACT_UTILS_H
#define PAYLOAD_EXTRACT_UTILS_H

#include <algorithm>
#include <cinttypes>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>
#include <sys/stat.h>

static std::string bytesToHexString(const uint8_t *bytes, uint32_t len) {
	std::string result(len * 2, ' '); // 预分配空间
	for (size_t i = 0; i < len; ++i) {
		constexpr const char *charTable = "0123456789abcdef";
		result[i * 2] = charTable[bytes[i] >> 4 & 0xF];
		result[i * 2 + 1] = charTable[bytes[i] & 0xF];
	}
	return result;
}

static uint64_t getFileSize(const std::string &dirPath) {
	struct stat st = {};
	if (stat(dirPath.c_str(), &st) == 0) {
		return st.st_size;
	}
	return 0;
}

static bool dirExists(const std::string &dirPath) {
	struct stat st = {};
	if (stat(dirPath.c_str(), &st) == 0) {
		return S_ISDIR(st.st_mode);
	}
	return false;
}

static bool fileExists(const std::string_view &filePath) {
	struct stat st = {};

	if (stat(filePath.data(), &st) == 0) {
		return !S_ISDIR(st.st_mode);
	}
	return false;
}

static int mkdirs(const char *dirPath, mode_t mode) {
	int len, err = 0;
	char str[PATH_MAX + 1] = {};
	strncpy(str, dirPath, PATH_MAX);
	len = strlen(str);
	for (int i = 0; i < len; i++) {
#ifndef _WIN32
		if (str[i] == '/' && i > 0) {
#else
			if (str[i] == '/' && i > 0 && str[i - 1] != ':') {
#endif
			str[i] = '\0';
			if (access(str, F_OK) != 0) {
#if defined(_WIN32)
				err = mkdir(str);
#else
				err = mkdir(str, mode);
#endif
				if (err) return err;
			}
			str[i] = '/';
		}
	}
	if (len > 0 && access(str, F_OK) != 0) {
#if defined(_WIN32)
		err = mkdir(str);
#else
		err = mkdir(str, mode);
#endif
	}
	return err;
}

static void strTrim(std::string &str) {
	if (!str.empty()) {
		str.erase(0, str.find_first_not_of(" \n\r\t\v\f"));
		str.erase(str.find_last_not_of(" \n\r\t\v\f") + 1);
	}
}

static bool startsWithIgnoreCase(const std::string &str, const std::string &prefix) {
	if (str.size() < prefix.size()) return false;
	auto lowerStr = str;
	auto lowerPrefix = prefix;
	std::ranges::transform(lowerStr, lowerStr.begin(), ::tolower);
	std::ranges::transform(lowerPrefix, lowerPrefix.begin(), ::tolower);
	return std::equal(lowerPrefix.begin(), lowerPrefix.end(), lowerStr.begin(),
	                  [](const char a, const char b) { return a == b; });
}

static void splitString(std::vector<std::string> &result, const std::string &str,
                        const std::string &delimiter, bool removeEmpty) {
	int64_t idx = 0, idx_last = 0;

	while (idx < str.size()) {
		idx = str.find_first_of(delimiter, idx_last);
		if (idx == std::string::npos)
			idx = str.size();

		if (idx - idx_last != 0 || !removeEmpty)
			result.push_back(std::move(str.substr(idx_last, idx - idx_last)));

		idx_last = idx + delimiter.size();
	}
}

static void splitSv(std::vector<std::string_view> &result, const std::string_view &strSv,
                    const std::string_view &delimiter, bool removeEmpty) {
	int64_t idx = 0, idx_last = 0;

	while (idx < strSv.size()) {
		idx = strSv.find_first_of(delimiter, idx_last);
		if (idx == std::string_view::npos)
			idx = strSv.size();

		if (idx - idx_last != 0 || !removeEmpty)
			result.emplace_back(strSv.substr(idx_last, idx - idx_last));

		idx_last = idx + delimiter.size();
	}
}

static void strReplaceAll(std::string &str, const std::string &oldValue, const std::string &newValue) {
	auto oldValueSize = oldValue.size();
	auto newValueSize = newValue.size();
	auto pos = str.find(oldValue);
	while (pos != std::string::npos) {
		str.replace(pos, oldValueSize, newValue);
		pos = str.find(oldValue, pos + newValueSize);
	}
}

static void handleWinFilePath(std::string &path) {
	strReplaceAll(path, "\\", "/");
	strReplaceAll(path, "./", ".\\/");
}

static void handleWinPath(std::string &path) {
#if defined(_WIN32)
	handleWinFilePath(path);
#endif
}

#endif  // PAYLOAD_EXTRACT_UTILS_H end
