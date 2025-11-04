#include <cstring>
#include <format>
#include <iostream>

#include "payload/PartitionInfo.h"

namespace skkk {
	void FileOperation::initExceptionInfo(int errCode) const {
		exceptionInfo = std::format("name: {:18s}, type: {}, code({}): {:s}",
		                            partName, errCode, type, strerror(abs(errCode)));
	}

	void PartitionInfo::printInfo() const {
		std::cout << std::format("name: {:18s} size: {}", name, size) << std::endl;
	}

	bool PartitionInfo::checkExtractionSuccessful() const {
		isExtractionSuccessful = exceptionInfos.empty() &&
		                         *extractProgress == operations.size();
		return isExtractionSuccessful;
	}

	void PartitionInfo::initExceptionInfoByInitFd(const std::string &path, int errCode) const {
		std::string msg = std::format("Create out file err: '{}', code({}): {:s}",
		                              path, errCode, strerror(abs(errCode)));
		exceptionInfos.emplace_back(msg);
	}

	void PartitionInfo::initExceptionInfo() const {
		for (const auto &operation: operations) {
			auto &info = operation.exceptionInfo;
			if (!info.empty())
				exceptionInfos.emplace_back(info);
		}
	}

	void PartitionInfo::ifExceptionExistsWrite2File() const {
		if (!isExtractionSuccessful) {
			auto *file = fopen(outErrorPath.c_str(), "wb");
			if (file) {
				for (const auto &info: exceptionInfos) {
					fprintf(file, "%s\n", info.c_str());
				}
				fclose(file);
			}
		}
	}

	void PartitionInfo::resetStatus() const {
		*extractProgress = 0;
		isExtractionSuccessful = false;
		exceptionInfos.clear();
		for (const auto &operation: operations) {
			operation.exceptionInfo.clear();
		}
	}
}
