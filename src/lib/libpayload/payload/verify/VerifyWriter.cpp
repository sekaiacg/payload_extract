#include <cinttypes>
#include <iostream>
#include <ranges>

#include "common/io.h"
#include "common/LogProgress.h"
#include "common/threadpool.h"
#include "payload/ExtractConfig.h"
#include "payload/LogBase.h"
#include "payload/Utils.h"
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

	static bool hashTreeHandleWinFd(const VerifyWriterHashTreeContext &ctx, const VerifyInfo &info) {
		auto &inFd = ctx.inFd;
		inFd = openFileRD(info.outFilePath);
		if (inFd < 0) {
			return false;
		}
		return true;
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
		auto &inFd = ctx.inFd;
		const auto blockSize = info.blockSize;
		const auto SALT_VERIFY_SIZE = ctx.saltVerifySize;
		std::vector<uint8_t> origData(SALT_VERIFY_SIZE);
		auto *sha256Data = origData.data();
		auto *readData = sha256Data + SHA256_DIGEST_SIZE;
		std::string a;
		if constexpr (ExtractConfig::isWin) {
			if (!hashTreeHandleWinFd(ctx, info)) {
				ret = inFd;
				goto exit;
			}
		}
		memcpy(sha256Data, hashTreeSalt, SHA256_DIGEST_SIZE);
		ret = blobRead(inFd, readData, readFilePos, blockSize);
		if (ret) goto exit;
		if (!sha256(sha256Data, SALT_VERIFY_SIZE, hashData + writeHashPos)) {
			ret = -EIO;
			goto exit;
		}

	exit:
		if (ret) {
			++*excSize;
		}
		++*calcProgress;
		if constexpr (ExtractConfig::isWin) closeFd(inFd);
	}

	bool VerifyWriter::handleHashTreeDataByInfo(const VerifyInfo &info) const {
		int inFd = -1;
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
		memcpy(sha256Data, info.hashTreeSalt.data(), SHA256_DIGEST_SIZE);

		if constexpr (!ExtractConfig::isWin) {
			inFd = openFileRD(info.outFilePath);
			if (inFd < 0) {
				goto exit;
			}
		}

		// wait
		{
			std::vector<VerifyWriterHashTreeContext> ctxs;
			ctxs.reserve(topLevel.blockCount);
			std::threadpool tp{config.threadNum};
			while (readPos < info.hashTreeDataExtentSize) {
				auto &ctx = ctxs.emplace_back(info, inFd, readPos,
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
		if constexpr (!ExtractConfig::isWin) closeFd(inFd);
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

	static bool fecHandleWinFd(const VerifyWriterFecContext &ctx, const VerifyInfo &info) {
		auto &inFd = ctx.inFd;
		inFd = openFileRD(info.outFilePath);
		if (inFd < 0) {
			return false;
		}
		return true;
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
		auto &inFd = ctx.inFd;
		const auto roundsIdx = ctx.roundsIdx;
		const auto rounds = info.fecRounds;
		const auto dataOffset = info.fecDataExtentOffset;
		auto fecWriteOffset = roundsIdx * blockSize * fecRoots;
		auto *fecData = ctx.fecData;
		const uint32_t rsBlockSize = blockSize * fecRsn;

		std::unique_ptr<void, decltype(&free_rs_char)> rs_char{init_rs_char(FEC_PARAMS(fecRoots)), &free_rs_char};
		std::vector<uint8_t> rsBlocksData(rsBlockSize);
		auto *rsBlocks = rsBlocksData.data();
		std::vector<uint8_t> bufferData(blockSize);
		auto *buffer = bufferData.data();

		if constexpr (ExtractConfig::isWin) {
			if (!fecHandleWinFd(ctx, info)) {
				++*fecExcSize;
				goto exit;
			}
		}

		for (size_t j = 0; j < fecRsn; j++) {
			uint64_t offset =
					fec_ecc_interleave(roundsIdx * fecRsn * blockSize + j, fecRsn, rounds);
			if (offset < dataSize) {
				ret = blobRead(inFd, buffer,
				               dataOffset + offset, blockSize);
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
		if constexpr (ExtractConfig::isWin) closeFd(inFd);
	}


	bool VerifyWriter::handleFecDataByInfo(const VerifyInfo &info) const {
		int inFd = -1;
		const auto fecDataSize = info.fecDataSize;
		const auto fecRoots = info.fecRoots;
		const auto fecRounds = info.fecRounds;
		const auto &currentProgress = info.fecProgress;

		if constexpr (!ExtractConfig::isWin) {
			inFd = openFileRD(info.outFilePath);
			if (inFd < 0) {
				goto exit;
			}
		}
		if (fecDataSize == fec_ecc_get_data_size(info.fecDataExtentSize, fecRoots)) {
			//wait
			{
				std::vector<VerifyWriterFecContext> ctxs;
				ctxs.reserve(fecRounds);
				std::threadpool tp{config.threadNum};
				for (int i = 0; i < fecRounds; i++) {
					auto &ctx = ctxs.emplace_back(info, inFd, const_cast<uint8_t *>(info.fecData.data()), i);
					tp.commit(encodeFecTask, std::ref(ctx));
				}
				printProgressMT(config.isSilent, info.name, FEC_FMT,
				                fecRounds, std::ref(*currentProgress), true);
			}
		}

	exit:
		if constexpr (!ExtractConfig::isWin) closeFd(inFd);
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
