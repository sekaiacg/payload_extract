#include <algorithm>
#include <cmath>
#include <future>
#include <ranges>

#include "common/io.h"
#include "common/LogProgress.h"
#include "common/threadpool.h"
#include "payload/FileWriter.h"
#include "payload/PartitionWriter.h"
#include "payload/Utils.h"

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
				LOGCE("create out dir fail: '%s'", outDir.c_str());
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

	bool PartitionWriter::extractByInfo(const PartitionInfo &info) const {
		int inFd = -1, outFd = -1;
		int payloadFd = payloadInfo->getPayloadFd();
		FileWriter fw{config.httpDownload};
		std::future<void> progressThread;
		std::shared_ptr<std::atomic_int> extractProgress = info.extractProgress;
		outFd = initOutFd(info.outFilePath, info.size);
		if (outFd <= 0) {
			info.initExcInfoByInitFd(info.outFilePath, outFd);
			goto exit;
		}
		if (config.isIncremental) {
			inFd = initInFd(info.oldFilePath);
			if (inFd <= 0) {
				info.initExcInfoByInitFd(info.oldFilePath, inFd);
				goto exit;
			}
		}
		progressThread = std::async(std::launch::async, printProgressMT, config.isSilent, info.name,
		                            info.size, info.operations.size(), std::ref(*extractProgress), true);
		for (const auto &operation: info.operations) {
			int ret = fw.writeDataByType(payloadFd, inFd, outFd, operation);
			if (ret) {
				operation.initExcInfo(ret);
			}
			++*extractProgress;
		}
		if (progressThread.valid()) progressThread.wait();
		info.initExcInfos();

	exit:
		closeFd(inFd);
		closeFd(outFd);
		return info.checkExtractionSuccessful();
	}

	static bool handleWinFd(const PartitionWriteContext &ctx, const PartitionInfo &info) {
		const auto &payloadPath = ctx.payloadPath;
		const auto &oldFilePath = info.oldFilePath;
		const auto &outFilePath = info.outFilePath;
		auto &payloadFd = ctx.payloadFd;
		auto &inFd = ctx.inFd;
		auto &outFd = ctx.outFd;
		auto isIncremental = ctx.isIncremental;
		auto isUrl = ctx.isUrl;
		if (!isUrl) {
			payloadFd = openFileRD(payloadPath);
			if (payloadFd <= 0) {
				info.initExcInfoByInitFd(payloadPath, payloadFd);
				return false;
			}
		}
		if (isIncremental) {
			inFd = PartitionWriter::initInFd(oldFilePath);
			if (inFd <= 0) {
				info.initExcInfoByInitFd(oldFilePath, inFd);
				return false;
			}
		}
		outFd = PartitionWriter::initOutFd(outFilePath, info.size, true);
		if (outFd <= 0) {
			info.initExcInfoByInitFd(outFilePath, outFd);
			return false;
		}
		return true;
	}

	static void extractTask(const PartitionWriteContext &ctx) {
		int ret = -1;
		auto &info = ctx.partitionInfo;
		auto &fileWriter = ctx.fileWriter;
		auto &operation = ctx.operation;
		auto &extractProgress = info.extractProgress;
		const auto &payloadFd = ctx.payloadFd;
		const auto &inFd = ctx.inFd;
		const auto &outFd = ctx.outFd;

		if constexpr (ExtractConfig::isWin) {
			if (!handleWinFd(ctx, info)) goto exit;
		}
		ret = fileWriter.writeDataByType(payloadFd, inFd,
		                                 outFd, operation);
		if (ret) {
			operation.initExcInfo(ret);
		}

	exit:
		++*extractProgress;
		if constexpr (ExtractConfig::isWin) {
			closeFd(payloadFd);
			closeFd(inFd);
			closeFd(outFd);
		}
	}

	bool PartitionWriter::extractByInfoMT(const PartitionInfo &info) const {
		int payloadFd = -1, inFd = -1, outFd = -1;
		payloadFd = payloadInfo->getPayloadFd();
		auto &extractProgress = info.extractProgress;
		FileWriter fw{config.httpDownload};

		if constexpr (!ExtractConfig::isWin) {
			outFd = initOutFd(info.outFilePath, info.size);
			if (outFd <= 0) {
				info.initExcInfoByInitFd(info.outFilePath, outFd);
				goto exit;
			}
			if (config.isIncremental) {
				inFd = initInFd(info.oldFilePath);
				if (inFd <= 0) {
					info.initExcInfoByInitFd(info.oldFilePath, inFd);
					goto exit;
				}
			}
		}

		// wait
		{
			uint64_t opSize = info.operations.size();
			std::vector<PartitionWriteContext> ctxs;
			ctxs.reserve(std::floor(static_cast<float>(opSize) * 1.5));
			std::threadpool tp(config.threadNum);
			for (const auto &operation: info.operations) {
				auto &ctx = ctxs.emplace_back(config.getPayloadPath(),
				                              info, fw, operation, payloadFd,
				                              inFd, outFd, config.isIncremental,
				                              config.isUrl);
				tp.commit(extractTask, std::ref(ctx));
			}
			printProgressMT(config.isSilent, info.name, info.size, opSize,
			                *extractProgress, true);
		}
		info.initExcInfos();

	exit:
		if constexpr (!ExtractConfig::isWin) {
			closeFd(inFd);
			closeFd(outFd);
		}
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
