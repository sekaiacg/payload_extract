#include <algorithm>
#include <cerrno>
#include <future>
#include <memory>
#include <print>
#include <ranges>

#include "common/LogProgress.h"
#include "common/threadpool.h"
#include "payload/FileWriter.h"
#include "payload/PartitionWriter.h"
#include "payload/Utils.h"
#include "files/common/io.h"

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

	void PartitionWriter::printPayloadInfo() const {
		payloadInfo->printInfo();
		payloadInfo->dynamicPartitionMetadata.printInfo();
		for (const auto &partition: partitions) {
			partition.printSimpleInfo();
		}
	}

	bool PartitionWriter::createOutDir() const {
		auto &outDir = payloadInfo->getConfig().getOldDir();
		if (!dirExists(outDir)) {
			if (mkdirs(outDir.c_str(), 0755)) {
				LOGCE("create out dir fail: '{}'({})", outDir, strerror(errno));
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
		return fd > 0 ? fd : -errno;
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

	std::shared_ptr<VerifyWriter> PartitionWriter::getVerifyWriter() const {
		std::unique_lock lock{_mutex};
		if (!verifyWriter) {
			verifyWriter = std::make_shared<VerifyWriter>(partitions, config);
		}
		return verifyWriter;
	}

	std::shared_ptr<FileWriter> PartitionWriter::getFileWriter() const {
		std::unique_lock lock{_mutex};
		if (!fileWriter) {
			fileWriter = std::make_shared<FileWriter>(config.httpDownload);
		}
		return fileWriter;
	}

#define PRINT_PROGRESS_FMT \
	BROWN2_BOLD("Extract: ") "%s" \
	GREEN2_BOLD("[ ") RED2("%2d%%") GREEN2_BOLD(" ]") \
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

	static bool handleParamFd(const PartitionInfo &info, bool isIncremental, int &oldFd, int &outFd) {
		int ret = 0;
		if (isIncremental) {
			oldFd = PartitionWriter::initInFd(info.oldFilePath);
			if (oldFd < 0) {
				info.initExcInfoByInitFd(info.oldFilePath, ret);
				ret = oldFd;
				goto exit;
			}
		}
		outFd = PartitionWriter::initOutFd(info.outFilePath, info.size);
		if (outFd < 0) {
			info.initExcInfoByInitFd(info.outFilePath, outFd);
			ret = oldFd;
		}
	exit:
		return ret == 0;
	}

	bool PartitionWriter::extractByInfo(const PartitionInfo &info) const {
		int ret = -1, oldFd = -1, outFd = -1;
		const auto &extractProgress = info.extractProgress;
		std::future<void> progressThread;
		const auto fw = getFileWriter();
		std::shared_ptr<WriteParam> writeParam = nullptr;

		if (!handleParamFd(info, config.isIncremental, oldFd, outFd)) {
			goto exit;
		}

		progressThread = std::async(std::launch::async, printProgressMT, config.isSilent, info.name,
		                            info.size, info.operations.size(), std::ref(*extractProgress), true);

		writeParam = std::make_shared<WriteParam>(payloadInfo->getPayloadFd(), oldFd, outFd);
		for (const auto &operation: info.operations) {
			ret = fw->writeDataByType(*writeParam, operation);
			if (ret) {
				operation.initExcInfo(ret);
			}
			++*extractProgress;
		}
		if (progressThread.valid()) progressThread.wait();
		info.initExcInfos();

	exit:
		closeFd(oldFd);
		closeFd(outFd);
		return info.checkExtractionSuccessful();
	}

	static void extractTask(const PartitionWriteContext &ctx) {
		int ret = -1;
		const auto &fileWriter = ctx.fileWriter;
		const auto &operation = ctx.operation;
		const auto &extractProgress = ctx.partitionInfo.extractProgress;
		const auto &writeParam = ctx.writeParam;

		ret = fileWriter.writeDataByType(writeParam, operation);
		if (ret) {
			operation.initExcInfo(ret);
		}
		++*extractProgress;
	}

	bool PartitionWriter::extractByInfoMT(const PartitionInfo &info) const {
		int oldFd = -1, outFd = -1;
		const auto isIncremental = config.isIncremental;
		const auto &extractProgress = info.extractProgress;
		std::future<void> progressThread;
		const auto fw = getFileWriter();
		std::shared_ptr<WriteParam> writeParam = nullptr;

		if (!handleParamFd(info, config.isIncremental, oldFd, outFd)) {
			goto exit;
		}

		// wait
		{
			writeParam = std::make_shared<WriteParam>(payloadInfo->getPayloadFd(), oldFd, outFd);
			uint64_t opSize = info.operations.size();
			std::vector<PartitionWriteContext> ctxs;
			ctxs.reserve(opSize);
			std::threadpool tp{config.threadNum};
			for (const auto &operation: info.operations) {
				auto &ctx = ctxs.emplace_back(info, *fw, operation, *writeParam, isIncremental);
				tp.commit(extractTask, std::ref(ctx));
			}
			printProgressMT(config.isSilent, info.name, info.size, opSize,
			                *extractProgress, true);
		}
		info.initExcInfos();
	exit:
		closeFd(oldFd);
		closeFd(outFd);
		return info.checkExtractionSuccessful();
	}

	bool PartitionWriter::extractPartitionByName(const std::string &name) {
		auto it = std::ranges::find(partitions, name, &PartitionInfo::name);
		if (it != partitions.end()) {
			const auto threadNum = config.threadNum;
			if (threadNum > 1) {
				return extractByInfoMT(*it);
			}
			return extractByInfo(*it);
		}
		return false;
	}

	static void printExtractConfig(uint32_t threadNum, bool isIncremental) {
		LOGCI(GREEN2_BOLD("Using ") RED2("{}")
		      GREEN2_BOLD(" threads, Payload ") RED2("{}") GREEN2_BOLD(" mode"),
		      threadNum, !isIncremental ? "FULL" : "INCREMENTAL");
	}

	static void printExtractResult(const std::string &name, int ret) {
		LOGCI("{:18}" BROWN2_BOLD(" result: ") "{}",
		      name, ret ? GREEN2_BOLD("success") : RED2("fail"));
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
