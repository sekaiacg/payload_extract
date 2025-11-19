#include <algorithm>
#include <cerrno>
#include <future>
#include <memory>
#include <ranges>

#include "common/LogProgress.h"
#include "common/threadpool.h"
#include "payload/FileWriter.h"
#include "payload/PartitionWriter.h"
#include "payload/Utils.h"
#include "payload/common/io.h"
#include "payload/mman/mmap.hpp"

namespace skkk {
	PartitionWriter::PartitionWriter(const std::shared_ptr<PayloadInfo> &payloadInfo)
		: payloadInfo(payloadInfo),
		  config(payloadInfo->getConfig()) {
	}

	bool PartitionWriter::initPartitions() {
		for (auto &partitionInfoMap = payloadInfo->partitionInfoMap;
		     const auto &info: partitionInfoMap | std::views::values) {
			partitions.emplace_back(info);
		}
		return !partitions.empty();
	}

	bool PartitionWriter::initPartitionsByTarget() {
		auto &targetNames = config.getTargets();
		auto &partitionInfoMap = payloadInfo->partitionInfoMap;
		if (config.isExcludeMode) {
			for (const auto &[name, info]: partitionInfoMap) {
				if (std::ranges::find(targetNames, name) == targetNames.end()) {
					partitions.emplace_back(info);
				}
			}
		} else {
			for (const auto &name: targetNames)
				if (partitionInfoMap.contains(name)) {
					partitions.emplace_back(partitionInfoMap[name]);
				}
		}
		return !partitions.empty();
	}

	void PartitionWriter::printPartitionsInfo() const {
		auto &header = payloadInfo->pHeader;
		std::cout << std::format("PartitionSize: {:3} MinorVersion: {:2} SecurityPatchLevel: {}",
		                         payloadInfo->partitionInfoMap.size(), header.minorVersion,
		                         header.securityPatchLevel)
				<< std::endl;
		for (const auto &partition: partitions) {
			partition.printInfo();
		}
	}

	bool PartitionWriter::createOutDir() const {
		auto &outDir = payloadInfo->getConfig().getOldDir();
		if (!dirExists(outDir)) {
			if (mkdirs(outDir.c_str(), 0755)) {
				LOGCE("create out dir fail: '%s'(%s)", outDir.c_str(), strerror(errno));
				return false;
			}
		}
		return true;
	}

	int PartitionWriter::createOutFile(const std::string &path, uint64_t fileSize) {
		int fd = open(path.c_str(),
		              O_CREAT | O_RDWR | O_TRUNC | O_BINARY, 0644);
		if (fd > 0) {
			if (!payload_ftruncate(fd, fileSize)) return fd;
		}
		return fd;
	}

	int PartitionWriter::initInFd(const std::string &path) {
		int inFd = openFileRD(path);
		return inFd;
	}

	int PartitionWriter::initOutFd(const std::string &path, uint64_t fileSize, bool isReOpen) {
		int outFd = -1;
		if (isReOpen && fileExists(path)) {
			outFd = openFileRW(path);
		} else {
			outFd = createOutFile(path, fileSize);
		}
		return outFd;
	}

	const std::vector<PartitionInfo> &PartitionWriter::getPartitions() {
		return partitions;
	}

	std::shared_ptr<VerifyWriter> PartitionWriter::getVerifyWriter() {
		std::unique_lock lock{_mutex};
		if (!verifyWriter) {
			verifyWriter = std::make_shared<VerifyWriter>(partitions, config);
		}
		return verifyWriter;
	}

#define PRINT_PROGRESS_FMT \
	BROWN2_BOLD "Extract: " COLOR_NONE "%s" \
	GREEN2_BOLD "[ " COLOR_NONE RED2 "%2d%%" LOG_RESET_COLOR GREEN2_BOLD " ]" COLOR_NONE \
	"\r"

	static std::string getPrintMsg(const std::string &partName, uint64_t partSize) {
		const std::string msg = std::format("{:18} size: {:<12}",
		                                    partName, partSize);
		return msg;
	}

	static void printProgressMT(bool isSilent, const std::string &partName, uint64_t partSize,
	                            uint64_t totalSize, const std::atomic_int &progress,
	                            bool hasEnter) {
		if (!isSilent) {
			std::string tag = getPrintMsg(partName, partSize);
			progressMT(PRINT_PROGRESS_FMT, tag,
			           totalSize, progress, hasEnter);
		}
	}

	static bool handleData(const PartitionInfo &info, bool isIncremental, int &inFd, int &outFd,
	                       const uint8_t *&inData, uint64_t &inDataSize, uint8_t *&outData, uint64_t &outDataSize) {
		int ret = -1;
		if (isIncremental) {
			ret = mapRdByPath(inFd, info.oldFilePath, inData, inDataSize);
			if (ret) {
				info.initExcInfoByInitFd(info.oldFilePath, ret);
				goto exit;
			}
		}
		outFd = PartitionWriter::initOutFd(info.outFilePath, info.size);
		if (outFd < 0) {
			info.initExcInfoByInitFd(info.outFilePath, outFd);
			goto exit;
		}
		ret = mapRwByPath(outFd, info.outFilePath, outData, outDataSize);
		if (ret) {
			info.initExcInfoByInitFd(info.outFilePath, ret);
		}
	exit:
		return !ret;
	}

	bool PartitionWriter::extractByInfo(const PartitionInfo &info) const {
		int ret = -1, inFd = -1, outFd = -1;
		const auto *payloadBinData = payloadInfo->getPayloadData();
		FileWriter fw{config.httpDownload};
		std::future<void> progressThread;
		std::shared_ptr<std::atomic_int> extractProgress = info.extractProgress;
		uint64_t inDataSize = 0;
		const uint8_t *inData = nullptr;
		uint64_t outDataSize = 0;
		uint8_t *outData = nullptr;

		if (!handleData(info, config.isIncremental, inFd, outFd,
		                inData, inDataSize, outData, outDataSize)) {
			goto exit;
		}

		progressThread = std::async(std::launch::async, printProgressMT, config.isSilent, info.name,
		                            info.size, info.operations.size(), std::ref(*extractProgress), true);
		for (const auto &operation: info.operations) {
			ret = fw.writeDataByType(payloadBinData, inData, outData, operation);
			if (ret) {
				operation.initExcInfo(ret);
			}
			++*extractProgress;
		}
		if (progressThread.valid()) progressThread.wait();
		info.initExcInfos();

	exit:
		unmap(inData, inDataSize);
		unmap(outData, outDataSize);
		closeFd(inFd);
		closeFd(outFd);
		return info.checkExtractionSuccessful();
	}

	static void extractTask(const PartitionWriteContext &ctx) {
		int ret = -1;
		const auto &fileWriter = ctx.fileWriter;
		const auto &operation = ctx.operation;
		const auto &extractProgress = ctx.partitionInfo.extractProgress;
		const auto *payloadData = ctx.payloadData;
		const auto *inData = ctx.inData;
		auto *outData = ctx.outData;

		ret = fileWriter.writeDataByType(payloadData, inData, outData, operation);
		if (ret) {
			operation.initExcInfo(ret);
		}
		++*extractProgress;
	}

	bool PartitionWriter::extractByInfoMT(const PartitionInfo &info) const {
		int inFd = -1, outFd = -1;
		const auto payloadData = payloadInfo->getPayloadData();
		const auto &extractProgress = info.extractProgress;
		const auto isIncremental = config.isIncremental;
		FileWriter fw{config.httpDownload};
		uint64_t inDataSize = 0;
		const uint8_t *inData = nullptr;
		uint64_t outDataSize = 0;
		uint8_t *outData = nullptr;

		if (!handleData(info, config.isIncremental, inFd, outFd,
		                inData, inDataSize, outData, outDataSize)) {
			goto exit;
		}

		// wait
		{
			uint64_t opSize = info.operations.size();
			std::vector<PartitionWriteContext> ctxs;
			ctxs.reserve(opSize);
			std::threadpool tp(config.threadNum);
			for (const auto &operation: info.operations) {
				auto &ctx = ctxs.emplace_back(info, fw, operation, payloadData,
				                              inData, outData, isIncremental);
				tp.commit(extractTask, std::ref(ctx));
			}
			printProgressMT(config.isSilent, info.name, info.size, opSize,
			                *extractProgress, true);
		}
		info.initExcInfos();

	exit:
		unmap(inData, inDataSize);
		unmap(outData, outDataSize);
		closeFd(inFd);
		closeFd(outFd);
		return info.checkExtractionSuccessful();
	}

	bool PartitionWriter::extractByPartitionByName(const std::string &name) {
		auto it = std::ranges::find(partitions, name, &PartitionInfo::name);
		if (it != partitions.end()) {
			return extractByInfo(*it);
		}
		return false;
	}

	static void printExtractConfig(uint32_t threadNum, bool isIncremental) {
		LOGCI(GREEN2_BOLD "Using " COLOR_NONE RED2 "%" PRIu32 COLOR_NONE
		      GREEN2_BOLD " threads, Payload " COLOR_NONE COLOR_NONE RED2 "%s" COLOR_NONE GREEN2_BOLD " mode"
		      COLOR_NONE, threadNum, !isIncremental ? "FULL" : "INCREMENTAL");
	}

	static void printExtractResult(const std::string &name, int ret) {
		LOGCI("%-18s" BROWN2_BOLD " result: " COLOR_NONE "%s",
		      name.c_str(), ret ? GREEN2_BOLD "success" COLOR_NONE:
		      RED2 "fail" COLOR_NONE);
	}

	void PartitionWriter::extractPartitions() const {
		if (!partitions.empty()) {
			bool ret = false;
			const auto threadNum = config.threadNum;
			const auto isIncremental = config.isIncremental;
			printExtractConfig(threadNum, isIncremental);
			if (threadNum > 1) {
				for (const auto &info: partitions) {
					ret = extractByInfoMT(info);
					if (!ret) {
						info.ifExcExistsWrite2File();
					}
					printExtractResult(info.name, ret);
				}
			} else {
				for (const auto &info: partitions) {
					ret = extractByInfo(info);
					if (!ret) {
						info.ifExcExistsWrite2File();
					}
					printExtractResult(info.name, ret);
				}
			}
		}
	}
}
