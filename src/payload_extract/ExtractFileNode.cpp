#include <cinttypes>
#include <fcntl.h>
#include <cmath>
#include <cstring>
#include <unistd.h>

#include "payload/defs.h"
#include "payload/FileWriter.h"
#include "payload/threadpool.h"
#include "ExtractFileNode.h"

namespace skkk {
	ExtractFileNode::ExtractFileNode(const ExtractFileNode &extractFileNode) {
		this->payloadInfo = extractFileNode.payloadInfo;
		this->name = extractFileNode.name;
		this->outFilePath = extractFileNode.outFilePath;
		this->outErrorPath = extractFileNode.outErrorPath;
		this->fileSize = extractFileNode.fileSize;
		this->fileOperations = extractFileNode.fileOperations;
		this->extractTaskRunCount = static_cast<int>(extractFileNode.extractTaskRunCount);
		this->exceptionSize = static_cast<int>(extractFileNode.exceptionSize);
		this->exceptionInfos = extractFileNode.exceptionInfos;
	}

	ExtractFileNode::ExtractFileNode(const PayloadInfo &payloadInfo, const FileInfo &fileInfo,
	                                 const std::string &outDir) {
		this->payloadInfo = &payloadInfo;
		this->name = fileInfo.name;
		this->outFilePath = std::move(outDir + "/" + fileInfo.name + ".img");
		this->outErrorPath = std::move(outDir + "/" + fileInfo.name + "_err.txt");
		this->fileSize = fileInfo.size;
		this->fileOperations = fileInfo.operations;
		this->exceptionInfos.reserve(fileOperations.size() * 2);
	}

	int ExtractFileNode::createOutFile() const {
		int fd = open(outFilePath.c_str(),
		              O_CREAT | O_RDWR | O_TRUNC | O_BINARY, 0644);
		if (fd > 0) {
#if defined(HAVE_FALLOCATE)
#if defined(HAVE_FALLOCATE64)
			if (!fallocate64(fd, 0, 0, fileSize)) return fd;
#else
			if (!fallocate(fd, 0, 0, fileSize)) return fd;
#endif
#endif
			if (!payload_ftruncate(fd, fileSize)) return fd;
		}
		return fd;
	}

	int ExtractFileNode::openOutFileW() const {
		return open(outFilePath.c_str(), O_WRONLY | O_BINARY);
	}

#define PART_INFO_FMT "%-20s size: %-13" PRIu64

	void ExtractFileNode::printInfo() const {
		printf("name: " PART_INFO_FMT "\n", name.c_str(), fileSize);
	}

	bool ExtractFileNode::initExceptionInfo(const FileOperation &fileOperation, int errCode) {
		++extractTaskRunCount;
		if (errCode) [[unlikely]] {
			char buf[256] = {};
			snprintf(buf, 256, "name: %-15s err: %" PRId32 " [%s] "
			         "type: %" PRIu32 " "
			         "[dataOffset: %-13" PRIu64 " length: %-10" PRIu64 "] "
			         "[fileOffset: %-13" PRIu64 " length: %-10" PRIu64 "]",
			         name.c_str(), errCode, strerror(abs(errCode)),
			         fileOperation.type,
			         fileOperation.dataOffset, fileOperation.dataLength,
			         fileOperation.fileOffset, fileOperation.destLength
			);
			exceptionInfos.emplace_back(buf);
			++exceptionSize;
			return true;
		}
		return false;
	}

	void ExtractFileNode::initExceptionByCreateOutFile(int errCode) {
		char errBuf[256] = {};
		snprintf(errBuf, 256, "create out file err: '%s', code: %" PRId32 " [%s]",
		         outFilePath.c_str(), errCode, strerror(abs(errCode)));
		exceptionInfos.emplace_back(errBuf);
	}

	void ExtractFileNode::printfExceptionIfExists() const {
		if (!writeSuccess) {
			for (const auto &exceptionInfo: exceptionInfos) {
				LOGCI("%s", exceptionInfo.c_str());
			}
		}
		LOGCI("%-20s result: %s", name.c_str(),
		      writeSuccess ? GREEN2_BOLD "success" COLOR_NONE :
		      RED2 "fail" COLOR_NONE);
	}

	void ExtractFileNode::writeExceptionFileIfExists() const {
		if (!writeSuccess) {
			auto *file = fopen(outErrorPath.c_str(), "wb");
			if (file) {
				for (const auto &exceptionInfo: exceptionInfos) {
					fprintf(file, "%s\n", exceptionInfo.c_str());
				}
				fclose(file);
			}
		}
		LOGCI("%-20s result: %s", name.c_str(),
		      writeSuccess ? GREEN2_BOLD "success" COLOR_NONE :
		      RED2 "fail" COLOR_NONE);
	}

	static void printExtractProgress(const char *str, int totalSize, int index,
	                                 std::atomic_int &perIndex, bool hasEnter) {
		int p = floor((float) index / (float) totalSize * 100.0f);
		int a = 0;
		if (perIndex < p) {
			perIndex = p;
			printf(BROWN2_BOLD "Extract: " COLOR_NONE "%s"
			       GREEN2_BOLD "[ " COLOR_NONE RED2 "%2d%%" LOG_RESET_COLOR GREEN2_BOLD " ]" COLOR_NONE
			       "\r",
			       str,
			       p
			);
			fflush(stdout);
			if (p == 100 && hasEnter) [[unlikely]] {
				printf("\n");
			}
		}
	}

	bool ExtractFileNode::writeFile(bool isSilent) {
		int outFd = createOutFile();
		if (outFd > 0) {
			char tag[64] = {};
			sprintf(tag, PART_INFO_FMT, name.c_str(), fileSize);
			int totalSize = fileOperations.size();
			int payloadDevFd = payloadInfo->payloadFd;
			std::atomic_int preIndex = 0;
			if (isSilent)
				LOGCI(PART_INFO_FMT, name.c_str(), fileSize);
			for (const auto &operation: fileOperations) {
				initExceptionInfo(operation, FileWriter::writeDataByType(
					                  payloadDevFd, outFd, operation));
				if (!isSilent) {
					printExtractProgress(tag, totalSize, extractTaskRunCount,
					                     preIndex, true);
				}
			}
			close(outFd);
		} else {
			initExceptionByCreateOutFile(outFd);
		}
		writeSuccess = extractTaskRunCount == fileOperations.size() &&
		               exceptionSize == 0;
		return writeSuccess;
	}

#if !defined(_WIN32)

	static void extractTask(ExtractFileNode &efn, int payloadDevFd, int outFd,
	                        const FileOperation &operation) {
		efn.initExceptionInfo(operation, FileWriter::writeDataByType(
			                      payloadDevFd, outFd, operation));
	}

#else

	static void extractTask(ExtractFileNode &efn, const char *payloadDevName,
	                        const FileOperation &operation) {
		int ret = -1, payloadDevFd = -1, outFd = -1;
		if (operation.isUrl) {
			outFd = efn.openOutFileW();
			if (outFd > 0) {
				ret = FileWriter::writeDataByType(payloadDevFd, outFd, operation);
				close(outFd);
			}
		} else {
			payloadDevFd = open(payloadDevName, O_RDONLY | O_BINARY);
			if (payloadDevFd > 0) {
				outFd = efn.openOutFileW();
				if (outFd > 0) {
					ret = FileWriter::writeDataByType(payloadDevFd, outFd, operation);
					close(outFd);
				}
				close(payloadDevFd);
			}
		}
		efn.initExceptionInfo(operation, ret);
	}

#endif

	bool ExtractFileNode::writeFileWithMultiThread(uint32_t threadNum, bool isSilent) {
		int outFd = createOutFile();
		if (outFd > 0) {
			int totalSize = fileOperations.size();
			std::threadpool tp(threadNum);
#if !defined(_WIN32)
			int payloadDevFd = payloadInfo->payloadFd;
			for (const auto &operation: fileOperations) {
				tp.commit(extractTask, std::ref(*this), payloadDevFd, outFd, std::ref(operation));
			}
#else
			bool isUrl = payloadInfo->payloadType == PAYLOAD_TYPE_URL;
			const char *payloadBinDevName = isUrl ? "" : payloadInfo->path.c_str();
			for (const auto &operation: fileOperations) {
				tp.commit(extractTask, std::ref(*this), payloadBinDevName, std::ref(operation));
			}
#endif
			if (!isSilent) {
				char tag[64] = {};
				sprintf(tag, PART_INFO_FMT, name.c_str(), fileSize);
				int i = 0;
				std::atomic_int perIndex = 0;
				while (extractTaskRunCount < totalSize) {
					if (i != extractTaskRunCount) {
						printExtractProgress(tag, totalSize,
						                     extractTaskRunCount, perIndex, true);
						i = extractTaskRunCount;
					}
					sleep(0);
				}
				printExtractProgress(tag, 1, 1, perIndex, true);
			} else {
				LOGCI(PART_INFO_FMT, name.c_str(), fileSize);
			}
		} else {
			initExceptionByCreateOutFile(outFd);
		}
		if (outFd > 0) close(outFd);
		writeSuccess = extractTaskRunCount == fileOperations.size() &&
		               exceptionSize == 0;
		return writeSuccess;
	}
}
