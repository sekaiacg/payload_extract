#include <cstring>
#include <format>
#include <print>

#include "payload/PartitionInfo.h"
#include "payload/Utils.h"

namespace skkk {
	void FileOperation::initExcInfo(int errCode) const {
		excInfo = std::format("name: {:18}, type: {}, code({}): {:s}",
		                      partName, errCode, type, strerror(abs(errCode)));
	}

	PartitionInfo::PartitionInfo(const std::string &name, uint64_t size, const std::string &outFilePath,
	                             uint32_t blockSize, const std::string &oldHash, uint64_t oldHashSize,
	                             const std::string &newHash,
	                             uint64_t newHashSize)
		: name(name),
		  size(size),
		  outFilePath(outFilePath),
		  blockSize(blockSize),
		  oldHash(oldHash),
		  oldHashSize(oldHashSize),
		  newHash(newHash),
		  newHashSize(newHashSize) {
		oldHashHexStr = bytesToHexString(reinterpret_cast<const uint8_t *>(oldHash.data()),
		                                 oldHash.size());
		newHashHexStr = bytesToHexString(reinterpret_cast<const uint8_t *>(newHash.data()),
		                                 newHash.size());
		simpleInfo = std::format("name: {:18} size: {:<12} sha256: {}", name, size, newHashHexStr);
	}

	void PartitionInfo::printSimpleInfo() const {
		std::println("{}", simpleInfo);
	}

	bool PartitionInfo::checkExtractionSuccessful() const {
		isExtractionSuccessful = excInfos.empty() &&
		                         *extractProgress == operations.size();
		return isExtractionSuccessful;
	}

	void PartitionInfo::initExcInfoByInitFd(const std::string &path, int errCode) const {
		std::unique_lock lock{*mutex_};
		std::string msg = std::format("Create/Open file err: '{}', code({}): {:s}",
		                              path, errCode, strerror(abs(errCode)));
		excInfos.emplace_back(msg);
	}

	void PartitionInfo::initExcInfos() const {
		for (const auto &operation: operations) {
			auto &info = operation.excInfo;
			if (!info.empty())
				excInfos.emplace_back(info);
		}
	}

	void PartitionInfo::ifExcExistsWrite2File() const {
		if (!isExtractionSuccessful) {
			if (auto *file = fopen(outErrorPath.c_str(), "wb")) {
				for (const auto &info: excInfos) {
					fprintf(file, "%s\n", info.c_str());
				}
				fclose(file);
			}
		}
	}

	void PartitionInfo::resetStatus() const {
		*extractProgress = 0;
		isExtractionSuccessful = false;
		excInfos.clear();
		for (const auto &operation: operations) {
			operation.excInfo.clear();
		}
	}
}
