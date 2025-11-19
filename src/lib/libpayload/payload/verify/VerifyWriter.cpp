#include <cinttypes>
#include <iostream>
#include <ranges>

#include "common/LogProgress.h"
#include "common/threadpool.h"
#include "payload/ExtractConfig.h"
#include "payload/LogBase.h"
#include "payload/Utils.h"
#include "payload/common/io.h"
#include "payload/mman/mmap.hpp"
#include "payload/verify/VerifyWriter.h"

#include "ecc.h"
#include "sha256Utils.h"

extern "C" {
#include "fec.h"
}

namespace skkk {
	enum FMT_TYPE {
		HASH_TREE_FMT = 0,
		FEC_FMT
	};

	VerifyWriter::VerifyWriter(const std::vector<PartitionInfo> &partitions,
	                           const ExtractConfig &config)
		: partitions(partitions),
		  config(config) {
	}

#define PRINT_PROGRESS_HASH_FMT \
	BLUE_BOLD "HASH :   " COLOR_NONE "%s" \
	GREEN2_BOLD " [ " COLOR_NONE RED2 "%2d%%" LOG_RESET_COLOR GREEN2_BOLD " ]" COLOR_NONE \
	"\r"

#define PRINT_PROGRESS_FEC_FMT \
	BLUE_BOLD "FEC  :   " COLOR_NONE "%s" \
	GREEN2_BOLD " [ " COLOR_NONE RED2 "%2d%%" LOG_RESET_COLOR GREEN2_BOLD " ]" COLOR_NONE \
	"\r"

	static std::string getPrintMsg(const std::string &partName) {
		const std::string format = std::format("{:18}", partName);
		return format;
	}

	static void printProgressMT(bool isSilent, const std::string &partName, int fmtType, uint64_t totalSize,
	                            const std::atomic_int &progress, bool hasEnter) {
		if (!isSilent) {
			const char *fmt;
			switch (fmtType) {
				case HASH_TREE_FMT:
					fmt = PRINT_PROGRESS_HASH_FMT;
					break;
				case FEC_FMT:
					fmt = PRINT_PROGRESS_FEC_FMT;
					break;
				default:
					fmt = PRINT_PROGRESS_HASH_FMT;
			}
			std::string tag = getPrintMsg(partName);
			progressMT(fmt, tag,
			           totalSize, progress, hasEnter);
		}
	}

	void VerifyWriter::initHashTreeLevel() {
		for (const auto &partInfo: partitions) {
			if (partInfo.hasHashTreeDataExtent && partInfo.hasFecDataExtent) {
				auto &verifyInfo = verifyInfos.emplace_back(partInfo);
				auto &topLevel = verifyInfo.topHashLevel;
				topLevel = {verifyInfo.hashTreeDataExtentSize, partInfo.blockSize};
				uint64_t blockCount = topLevel.blockCount;
				uint64_t totalHashTotal = topLevel.totalHashSize;
				uint64_t totalCalcCount = blockCount;
				while (blockCount != 1) {
					const auto &level = verifyInfo.hashLevels.emplace_back(totalHashTotal, partInfo.blockSize);
					blockCount = level.blockCount;
					totalHashTotal = level.totalHashSize;
					totalCalcCount += blockCount;
				}
				verifyInfo.hashTreeTotalProgress = totalCalcCount;
			}
		}
	}

	static void sha256HashTreeTopLevelTask(const VerifyWriterHashTreeContext &ctx) {
		int ret = -1;
		const auto &info = ctx.verifyInfo;
		const auto &excSize = info.hashTreeExcSize;
		const auto &calcProgress = info.hashTreeProgress;
		const auto hashTreeSalt = info.hashTreeSalt.data();
		const auto readFilePos = ctx.readFilePos;
		const auto writeHashPos = ctx.writeHashPos;
		auto *hashData = ctx.hashData;
		const auto blockSize = info.blockSize;
		const auto SALT_VERIFY_SIZE = ctx.saltVerifySize;
		std::vector<uint8_t> origData(SALT_VERIFY_SIZE);
		auto *sha256Data = origData.data();
		auto *readData = sha256Data + SHA256_DIGEST_SIZE;

		memcpy(sha256Data, hashTreeSalt, SHA256_DIGEST_SIZE);
		ret = memcpy(readData, ctx.inData + readFilePos, blockSize) == readData ? 0 : -EIO;
		if (ret) goto exit;
		if (!sha256(sha256Data, SALT_VERIFY_SIZE, hashData + writeHashPos)) {
			ret = -EIO;
		}

	exit:
		if (ret) ++*excSize;
		++*calcProgress;
	}

	bool VerifyWriter::handleHashTreeDataByInfo(const VerifyInfo &info) const {
		int ret = 0, inFd = -1;
		uint64_t readPos = 0, writeHashPos = 0;
		std::future<void> progressThread;
		const auto &currentProgress = info.hashTreeProgress;
		const auto &hashTreeExcSize = info.hashTreeExcSize;
		auto &levels = info.hashLevels;
		const auto &topLevel = info.topHashLevel;
		const auto *preLevel = &topLevel;
		auto &rootLevel = info.rootHashLevel;
		auto *preHashData = preLevel->hashData;
		const auto blockSize = info.blockSize;
		const uint64_t SALT_VERIFY_SIZE = blockSize + SHA256_DIGEST_SIZE;
		std::vector<uint8_t> origData(SALT_VERIFY_SIZE);
		auto *sha256Data = origData.data();
		auto *readData = sha256Data + SHA256_DIGEST_SIZE;
		uint64_t inDataSize = 0;
		const uint8_t *inData = nullptr;
		memcpy(sha256Data, info.hashTreeSalt.data(), SHA256_DIGEST_SIZE);

		ret = mapRdByPath(inFd, info.outFilePath, inData, inDataSize);
		if (ret) {
			goto exit;
		}

		// wait
		{
			std::vector<VerifyWriterHashTreeContext> ctxs;
			ctxs.reserve(topLevel.blockCount);
			std::threadpool tp{config.threadNum};
			while (readPos < info.hashTreeDataExtentSize) {
				auto &ctx = ctxs.emplace_back(info, inData, readPos,
				                              writeHashPos, preHashData, SALT_VERIFY_SIZE);
				tp.commit(sha256HashTreeTopLevelTask, std::ref(ctx));
				readPos += blockSize;
				writeHashPos += SHA256_DIGEST_SIZE;
			}
			progressThread = std::async(std::launch::async, printProgressMT, config.isSilent, info.name,
			                            HASH_TREE_FMT, info.hashTreeTotalProgress, std::ref(*currentProgress),
			                            true);
		}

		readPos = writeHashPos = 0;
		for (const auto &level: levels) {
			auto *hashData = level.hashData;
			while (readPos < preLevel->totalHashSize) {
				memcpy(readData, preHashData + readPos, blockSize);
				if (!sha256(sha256Data, SALT_VERIFY_SIZE, hashData + writeHashPos)) {
					++*hashTreeExcSize;
				}
				readPos += blockSize;
				writeHashPos += SHA256_DIGEST_SIZE;
				++*currentProgress;
			}
			readPos = writeHashPos = 0;
			preLevel = &level;
			preHashData = level.hashData;
		}
		rootLevel = levels.back();
		levels.pop_back();
		std::ranges::reverse(levels);
		levels.emplace_back(topLevel);

	exit:
		if (progressThread.valid()) progressThread.wait();
		if (ret) ++*hashTreeExcSize;
		unmap(inData, inDataSize);
		closeFd(inFd);
		return info.checkCalcHashTreeSuccessful();
	}

	bool VerifyWriter::updateHashTreeByInfo(const VerifyInfo &info) {
		bool ret = -1;
		int outFd = openFileRW(info.outFilePath);
		if (outFd > 0) {
			const auto &levels = info.hashLevels;
			uint64_t hashPos = info.hashTreeDataOffset;
			for (const auto &level: levels) {
				ret = blobWrite(outFd, level.hashData, hashPos,
				                level.totalHashSize);
				if (ret) {
					goto exit;
				}
				hashPos += level.totalHashSize;
			}
		}
	exit:
		closeFd(outFd);
		return !ret;
	}

	/**
	 * Reference: https://android.googlesource.com/platform/system/update_engine/+/refs/heads/main/payload_consumer/verity_writer_android.cc#328
	 *
	 * @param ctx
	 */
	static void encodeFecTask(const VerifyWriterFecContext &ctx) {
		int ret = -1;
		const auto &info = ctx.verifyInfo;
		auto &currentProgress = info.fecProgress;
		auto &fecExcSize = info.fecExcSize;
		const auto fecRoots = info.fecRoots;
		const auto fecRsn = info.fecRsn;
		const auto dataSize = info.fecDataExtentSize;
		const auto blockSize = info.blockSize;
		const auto *inData = ctx.inData;
		const auto roundsIdx = ctx.roundsIdx;
		const auto rounds = info.fecRounds;
		const auto dataOffset = info.fecDataExtentOffset;
		const auto fecWriteOffset = roundsIdx * blockSize * fecRoots;
		auto *fecData = ctx.fecData;
		const uint32_t rsBlockSize = blockSize * fecRsn;

		std::unique_ptr<void, decltype(&free_rs_char)> rs_char{init_rs_char(FEC_PARAMS(fecRoots)), &free_rs_char};
		std::vector<uint8_t> rsBlocksData(rsBlockSize);
		auto *rsBlocks = rsBlocksData.data();
		std::vector<uint8_t> bufferData(blockSize);
		auto *buffer = bufferData.data();

		for (size_t j = 0; j < fecRsn; j++) {
			uint64_t offset =
					fec_ecc_interleave(roundsIdx * fecRsn * blockSize + j, fecRsn, rounds);
			if (offset < dataSize) {
				ret = memcpy(buffer, inData + dataOffset + offset, blockSize) == buffer ? 0 : -EIO;
				if (ret) {
					++*fecExcSize;
					goto exit;
				}
			}
			for (size_t k = 0; k < blockSize; k++) {
				rsBlocks[k * fecRsn + j] = buffer[k];
			}
			memset(buffer, 0, blockSize);
		}

		for (size_t j = 0; j < blockSize; j++) {
			encode_rs_char(rs_char.get(),
			               rsBlocks + j * fecRsn,
			               fecData + fecWriteOffset + j * fecRoots);
		}
		memset(rsBlocks, 0, rsBlockSize);

	exit:
		++*currentProgress;
	}


	bool VerifyWriter::handleFecDataByInfo(const VerifyInfo &info) const {
		int ret = 0, inFd = -1;
		const auto fecDataSize = info.fecDataSize;
		const auto fecRoots = info.fecRoots;
		const auto fecRounds = info.fecRounds;
		const auto &currentProgress = info.fecProgress;
		auto &fecExcSize = info.fecExcSize;
		const uint8_t *inData = nullptr;
		uint64_t inDataSize = 0;

		ret = mapRdByPath(inFd, info.outFilePath, inData, inDataSize);
		if (ret) {
			goto exit;
		}

		if (fecDataSize == fec_ecc_get_data_size(info.fecDataExtentSize, fecRoots)) {
			//wait
			{
				std::vector<VerifyWriterFecContext> ctxs;
				ctxs.reserve(fecRounds);
				std::threadpool tp{config.threadNum};
				for (int i = 0; i < fecRounds; i++) {
					auto &ctx = ctxs.emplace_back(info, const_cast<uint8_t *>(info.fecData.data()), inData, i);
					tp.commit(encodeFecTask, std::ref(ctx));
				}
				printProgressMT(config.isSilent, info.name, FEC_FMT,
				                fecRounds, std::ref(*currentProgress), true);
			}
		}

	exit:
		if (ret) ++*fecExcSize;
		unmap(inData, inDataSize);
		closeFd(inFd);
		return info.checkCalcFecSuccessful();
	}

	bool VerifyWriter::updateFecByInfo(const VerifyInfo &info) {
		int ret = -1, outFd = -1;
		outFd = openFileRW(info.outFilePath);
		if (outFd < 0) {
			goto exit;
		}
		ret = blobWrite(outFd, info.fecData.data(), info.fecDataOffset, info.fecDataSize);

	exit:
		closeFd(outFd);
		return !ret;
	}

	static void printVerifyResult(const std::string &name, int ret) {
		std::string message = std::format(
			BLUE_BOLD "Verify : " COLOR_NONE "{:18s}" BLUE_BOLD " result: " COLOR_NONE "{}",
			name, ret ? GREEN2_BOLD "success" COLOR_NONE : RED2 "fail" COLOR_NONE);
		std::cout << message << std::endl;
	}

	void VerifyWriter::updateVerifyData() const {
		for (const auto &info: verifyInfos) {
			bool hashTreeSuccessful = false, fecSuccessful = false, updateSuccessful = false;
			if (handleHashTreeDataByInfo(info)) {
				if (updateHashTreeByInfo(info)) {
					hashTreeSuccessful = true;
				}
			}
			if (hashTreeSuccessful) {
				if (handleFecDataByInfo(info)) {
					fecSuccessful = updateFecByInfo(info);
				} else {
				}
			}
			printVerifyResult(info.name, hashTreeSuccessful && fecSuccessful);
		}
	}
}
