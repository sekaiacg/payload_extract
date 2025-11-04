#include <algorithm>
#include <cmath>

#include "common/defs.h"
#include "common/threadpool.h"
#include "payload/FileWriter.h"
#include "payload/PartitionWriter.h"
#include "payload/Utils.h"

namespace skkk {
	bool PartitionWriter::initPartitions() {
		for (auto &partitionInfoMap = payloadInfo->partitionInfoMap;
		     const auto &[name, info]: partitionInfoMap) {
			partitions.emplace_back(info);
		}
		return !partitions.empty();
	}

	bool PartitionWriter::initPartitionsByTarget() {
		auto &targetNames = extractConfig.getTargets();
		auto &partitionInfoMap = payloadInfo->partitionInfoMap;
		if (extractConfig.isExcludeMode) {
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

	int PartitionWriter::openFileRDOnly(const std::string &path) {
		return open(path.c_str(), O_RDONLY | O_BINARY);
	}

	int PartitionWriter::openFileRW(const std::string &path) {
		return open(path.c_str(), O_RDWR | O_BINARY);
	}

	bool PartitionWriter::createOutDir() const {
		auto &outDir = payloadInfo->getExtractConfig().getOldDir();
		if (!dirExists(outDir)) {
			if (mkdirs(outDir.c_str(), 0755)) {
				LOGCE("create out dir fail: '%s'", outDir.c_str());
				return false;
			}
		}
		return true;
	}

	inline int payload_fallocate(int fd, off64_t offset, off64_t length, int flags) {
		int ret = -1;
#if defined(HAVE_FALLOCATE)
#if defined(HAVE_FALLOCATE64)
		ret = fallocate64(fd, 0, 0, length);
#else
		ret = fallocate(fd, 0, 0, length);
#endif
#endif
		return ret;
	}

	int PartitionWriter::createOutFile(const std::string &path, uint64_t fileSize) {
		int fd = open(path.c_str(),
		              O_CREAT | O_RDWR | O_TRUNC | O_BINARY, 0644);
		if (fd > 0) {
			if (!payload_fallocate(fd, 0, 0, fileSize)) return fd;
			if (!payload_ftruncate(fd, fileSize)) return fd;
		}
		return fd;
	}

	int PartitionWriter::initInFd(const std::string &path) {
		int inFd = openFileRW(path);
		return inFd;
	}

	int PartitionWriter::initOutFd(const std::string &path, uint64_t fileSize) {
		int outFd = -1;
		if (fileExists(path)) {
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

	static void printProgress(const std::string &partName, uint64_t partSize,
	                          uint64_t totalSize, uint64_t index, bool hasEnter) {
		constexpr int perPrint = 3;
		auto format = getPrintMsg(partName, partSize);
		const char *msg = format.c_str();

		if (index % perPrint == 0 || index == totalSize) {
			uint32_t percentage = 0;
			percentage = std::floor(static_cast<float>(index) / static_cast<float>(totalSize) * 100.0F);
			printf(PRINT_PROGRESS_FMT, msg, percentage);
			if (hasEnter && percentage == 100) [[unlikely]] {
				printf("\n");
			}
		}
	}

	static void printProgress(bool isSilent, const std::string &partName, uint64_t partSize,
	                          uint64_t totalSize, uint64_t index, bool hasEnter) {
		if (!isSilent) [[likely]]{
			printProgress(partName, partSize, totalSize, index, hasEnter);
		}
	}

	static void printProgressMT(const std::string &partName, uint64_t partSize, uint64_t totalSize,
	                            const std::atomic_int &progress,
	                            bool hasEnter) {
		auto format = getPrintMsg(partName, partSize);
		const char *msg = format.c_str();
		uint32_t currProgress = 0, previousPercentage = 0, percentage = 0;
		do {
			if (currProgress != progress) {
				percentage = std::floor(static_cast<float>(progress) / static_cast<float>(totalSize) * 100.0F);
				if (percentage > previousPercentage) {
					printf(PRINT_PROGRESS_FMT, msg, percentage);
					if (hasEnter && percentage == 100) [[unlikely]] {
						printf("\n");
					}
					previousPercentage = percentage;
				}
				currProgress = progress;
				sleep(0);
			}
			sleep(0);
		} while (currProgress != totalSize);
	}

	bool PartitionWriter::extractByInfo(const PartitionInfo &info) const {
		int outFd = -1;
		int payloadFd = payloadInfo->getPayloadFd();
		std::shared_ptr<std::atomic_int> extractProgress = info.extractProgress;
		FileWriter fw{extractConfig.httpDownload};
		outFd = initOutFd(info.outFilePath, info.size);
		if (outFd > 0) {
			for (const auto &operation: info.operations) {
				int ret = fw.writeDataByType(payloadFd, -1, outFd, operation);
				if (ret) {
					operation.initExceptionInfo(ret);
				}
				(*extractProgress)++;
				printProgress(extractConfig.isSilent, info.name, info.size,
				              info.operations.size(), *extractProgress, true);
			}
			printProgress(extractConfig.isSilent, info.name, info.size,
			              info.operations.size(), *extractProgress, true);
			close(outFd);
		} else {
			info.initExceptionInfoByInitFd(info.outFilePath, outFd);
		}
		return info.checkExtractionSuccessful();
	}

	bool PartitionWriter::incrementalExtractByInfo(const PartitionInfo &info) const {
		int ret = -1, inFd = -1, outFd = -1, i = 0;
		int payloadFd = payloadInfo->getPayloadFd();
		std::shared_ptr<std::atomic_int> extractProgress = info.extractProgress;
		FileWriter fw{extractConfig.httpDownload};
		outFd = initOutFd(info.outFilePath, info.size);
		if (outFd <= 0) {
			info.initExceptionInfoByInitFd(info.outFilePath, outFd);
			goto exit;
		}
		inFd = initInFd(info.oldFilePath);
		if (inFd <= 0) {
			ret = outFd;
			info.initExceptionInfoByInitFd(info.oldFilePath, inFd);
			goto exit;
		}
		for (auto &operation: info.operations) {
			ret = fw.writeDataByType(payloadFd, inFd, outFd, operation);
			if (ret) {
				operation.initExceptionInfo(ret);
			}
			(*extractProgress)++;
			printProgress(extractConfig.isSilent, info.name, info.size,
			              info.operations.size(), *extractProgress, true);
		}
		printProgress(extractConfig.isSilent, info.name, info.size,
		              info.operations.size(), *extractProgress, true);
		info.initExceptionInfo();

	exit:
		if (inFd > 0) close(inFd);
		if (outFd > 0) close(outFd);
		return info.checkExtractionSuccessful();
	}

	static void extractTask(const FileWriter &fileWriter, const FileOperation &operation,
	                        const std::shared_ptr<std::atomic_int> &extractProgress,
	                        int payloadFd, int inFd, int outFd) {
		int ret = fileWriter.writeDataByType(payloadFd, inFd,
		                                     outFd, operation);
		if (ret) {
			operation.initExceptionInfo(ret);
		}
		++*extractProgress;
	}

	bool PartitionWriter::extractByInfoMT(const PartitionInfo &info) const {
		int payloadFd = -1, outFd = -1;
		payloadFd = payloadInfo->getPayloadFd();
		FileWriter fw{extractConfig.httpDownload};
		outFd = initOutFd(info.outFilePath, info.size);
		if (outFd <= 0) {
			info.initExceptionInfoByInitFd(info.outFilePath, outFd);
			goto exit;
		}
		// wait
		{
			std::threadpool tp(extractConfig.threadNum);
			for (auto &operation: info.operations) {
				tp.commit(extractTask, std::ref(fw), std::ref(operation),
				          info.extractProgress, payloadFd, -1, outFd);
			}

			if (!extractConfig.isSilent)
				printProgressMT(info.name, info.operations.size(), info.size,
				                *info.extractProgress, true);
		}
		info.initExceptionInfo();

	exit:
		if (outFd > 0) close(outFd);
		return info.checkExtractionSuccessful();
	}

	bool PartitionWriter::incrementalExtractByInfoMT(const PartitionInfo &info) const {
		int inFd = -1, outFd = -1;
		int payloadFd = payloadInfo->getPayloadFd();
		FileWriter fw{extractConfig.httpDownload};
		outFd = initOutFd(info.outFilePath, info.size);
		if (outFd <= 0) {
			info.initExceptionInfoByInitFd(info.outFilePath, outFd);
			goto exit;
		}
		inFd = initInFd(info.oldFilePath);
		if (inFd <= 0) {
			info.initExceptionInfoByInitFd(info.oldFilePath, inFd);
			goto exit;
		}
		// wait
		{
			std::threadpool tp{extractConfig.threadNum};
			for (auto &operation: info.operations) {
				tp.commit(extractTask, std::ref(fw), std::ref(operation),
				          std::ref(info.extractProgress), payloadFd, inFd, outFd);
			}
			if (!extractConfig.isSilent)
				printProgressMT(info.name, info.size, info.operations.size(),
				                *info.extractProgress, true);
		}
		info.initExceptionInfo();

	exit:
		if (inFd > 0) close(inFd);
		if (outFd > 0) close(outFd);
		return info.checkExtractionSuccessful();
	}

	static void extractTaskInWin(const ExtractConfig &extractConfig, const PartitionInfo &info,
	                             const FileOperation &operation,
	                             const std::shared_ptr<std::atomic_int> &extractProgress) {
		int ret = -1, payloadFd = -1, inFd = -1, outFd = -1;
		FileWriter fw{extractConfig.httpDownload};
		outFd = PartitionWriter::initOutFd(info.outFilePath, info.size);
		if (outFd <= 0) {
			info.initExceptionInfoByInitFd(info.outFilePath, outFd);
			goto exit;
		}
		if (extractConfig.isIncremental) {
			inFd = PartitionWriter::initInFd(info.oldFilePath);
			if (inFd <= 0) {
				info.initExceptionInfoByInitFd(info.oldFilePath, inFd);
				goto exit;
			}
		}

		if (!extractConfig.isUrl) {
			payloadFd = PartitionWriter::openFileRW(extractConfig.getPayloadPath());
			if (payloadFd <= 0) {
				info.initExceptionInfoByInitFd(extractConfig.getPayloadPath(), payloadFd);
				goto exit;
			}
		}
		ret = fw.writeDataByType(payloadFd, inFd, outFd, operation);
		if (ret) {
			operation.initExceptionInfo(ret);
		}

	exit:
		++*extractProgress;
		if (payloadFd > 0) close(payloadFd);
		if (inFd > 0) close(inFd);
		if (outFd > 0) close(outFd);
	}

	bool PartitionWriter::extractByInfoMTInWin(const PartitionInfo &info) const {
		// wait
		{
			std::threadpool tp(extractConfig.threadNum);
			for (auto &operation: info.operations) {
				tp.commit(extractTaskInWin, std::ref(extractConfig), std::ref(info), std::ref(operation),
				          std::ref(info.extractProgress));
			}
		}
		info.initExceptionInfo();
		return info.checkExtractionSuccessful();
	}

	bool PartitionWriter::incrementalExtractByInfoMTInWin(const PartitionInfo &info) const {
		// wait
		{
			std::threadpool tp(extractConfig.threadNum);
			for (auto &operation: info.operations) {
				tp.commit(extractTaskInWin, std::ref(extractConfig), std::ref(info),
				          std::ref(operation), std::ref(info.extractProgress));
			}
		}
		info.initExceptionInfo();
		return info.checkExtractionSuccessful();
	}


	bool PartitionWriter::extractByPartitionByName(const std::string &name) {
		auto it = std::ranges::find(partitions, name, &PartitionInfo::name);
		if (it != partitions.end()) {
			return extractByInfo(*it);
		}
		return false;
	}

	static void printfExtractConfig(uint32_t threadNum, bool isIncremental) {
		LOGCI(GREEN2_BOLD "Using " COLOR_NONE RED2 "%" PRIu32 COLOR_NONE
		      GREEN2_BOLD " threads, Payload " COLOR_NONE COLOR_NONE RED2 "%s" COLOR_NONE GREEN2_BOLD " mode"
		      COLOR_NONE, threadNum, !isIncremental ? "FULL" : "INCREMENTAL");
	}

	void PartitionWriter::extractPartitions() const {
		if (!partitions.empty()) {
			bool ret = false;
			const auto threadNum = extractConfig.threadNum;
			const auto isIncremental = extractConfig.isIncremental;
			printfExtractConfig(threadNum, isIncremental);
			if (threadNum > 1) {
				for (const auto &info: partitions) {
#if !defined(_WIN32)
					ret = !isIncremental ? extractByInfoMT(info) : incrementalExtractByInfoMT(info);
#else
					ret = !isIncremental ? extractByInfoMTInWin(info) : incrementalExtractByInfoMTInWin(info);
#endif
					if (!ret) {
						info.ifExceptionExistsWrite2File();
					}
					LOGCI("%-18s result: %s", info.name.c_str(), ret ? GREEN2_BOLD "success" COLOR_NONE:
					      RED2 "fail" COLOR_NONE);
				}
			} else {
				for (const auto &info: partitions) {
					ret = !isIncremental ? extractByInfo(info) : incrementalExtractByInfo(info);
					if (!ret) {
						info.ifExceptionExistsWrite2File();
					}
					LOGCI("%-18s result: %s", info.name.c_str(), ret ? GREEN2_BOLD "success" COLOR_NONE:
					      RED2 "fail" COLOR_NONE);
				}
			}
		}
	}
}
