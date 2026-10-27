#include <algorithm>
#include <cinttypes>
#include <print>
#include <ranges>

#include "common/LogProgress.h"
#include "common/threadpool.h"
#include "files/io.h"
#include "payload/ExtractConfig.h"
#include "payload/LogBase.h"
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
	BLUE_BOLD("HASH :   ") "%s" \
	GREEN2_BOLD(" [ ") RED2("%2d%%") GREEN2_BOLD(" ]") \
	"\r"

#define PRINT_PROGRESS_FEC_FMT \
	BLUE_BOLD("FEC  :   ") "%s" \
	GREEN2_BOLD(" [ ") RED2("%2d%%") GREEN2_BOLD(" ]") \
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
			if (partInfo.hasHashTreeDataExtent) {
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
		auto &inFd = ctx.inFd;
		auto *hashData = ctx.hashData;
		const auto blockSize = info.blockSize;
		const auto hashTreeSaltSize = info.hashTreeSalt.size();
		const auto SALT_VERIFY_SIZE = blockSize + hashTreeSaltSize;
		std::vector<uint8_t> origData(SALT_VERIFY_SIZE);
		auto *sha256Data = origData.data();
		auto *readData = sha256Data + hashTreeSaltSize;
		memcpy(sha256Data, hashTreeSalt, hashTreeSaltSize);

		ret = IO::blobRead(inFd, readData, readFilePos, blockSize);
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
		const auto hashTreeSaltSize = info.hashTreeSalt.size();
		const uint64_t SALT_VERIFY_SIZE = blockSize + hashTreeSaltSize;
		std::vector<uint8_t> origData(SALT_VERIFY_SIZE);
		auto *sha256Data = origData.data();
		auto *readData = sha256Data + hashTreeSaltSize;
		uint64_t inDataSize = 0;
		const uint8_t *inData = nullptr;
		memcpy(sha256Data, info.hashTreeSalt.data(), hashTreeSaltSize);

		inFd = IO::openFileRD(info.outFilePath);
		if (inFd < 0) {
			ret = inFd;
			goto exit;
		}

		// wait
		{
			progressThread = std::async(std::launch::async, printProgressMT, config.isSilent, info.name,
			                            HASH_TREE_FMT, info.hashTreeTotalProgress, std::ref(*currentProgress),
			                            true);
			std::vector<VerifyWriterHashTreeContext> ctxs;
			ctxs.reserve(topLevel.blockCount);
			std::threadpool tp{config.threadNum};
			while (readPos < info.hashTreeDataExtentSize) {
				auto &ctx = ctxs.emplace_back(info, inFd, readPos,
				                              writeHashPos, preHashData);
				tp.commit(sha256HashTreeTopLevelTask, std::ref(ctx));
				readPos += blockSize;
				writeHashPos += SHA256_DIGEST_SIZE;
			}
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
		return info.checkCalcHashTreeSuccessful();
	}

	bool VerifyWriter::updateHashTreeByInfo(const VerifyInfo &info) {
		bool ret = -1;
		int outFd = IO::openFileRW(info.outFilePath);
		if (outFd > 0) {
			auto &levels = info.hashLevels;
			uint64_t hashPos = info.hashTreeDataOffset;
			for (const auto &level: levels) {
				ret = IO::blobWrite(outFd, level.hashData, hashPos, level.totalHashSize);
				if (ret) {
					goto exit;
				}
				hashPos += level.totalHashSize;
			}
			levels.clear();
		}
	exit:
		IO::closeFd(outFd);
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
		auto &inFd = ctx.inFd;
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
				ret = IO::blobRead(inFd, buffer, dataOffset + offset, blockSize);
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

		inFd = IO::openFileRD(info.outFilePath);
		if (inFd < 0) {
			ret = inFd;
			goto exit;
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
		if (ret) ++*fecExcSize;
		return info.checkCalcFecSuccessful();
	}

	bool VerifyWriter::updateFecByInfo(const VerifyInfo &info) {
		int ret = -1, outFd = -1;
		outFd = IO::openFileRW(info.outFilePath);
		if (outFd < 0) {
			goto exit;
		}
		ret = IO::blobWrite(outFd, info.fecData.data(), info.fecDataOffset, info.fecDataSize);

	exit:
		IO::closeFd(outFd);
		return !ret;
	}

	static void printVerifyResult(const std::string &name, int ret) {
		std::println(BLUE_BOLD("Verify : ") "{:18s}" BLUE_BOLD(" result: ") "{}",
		             name, ret ? GREEN2_BOLD("success") : RED2("fail"));
	}

	void VerifyWriter::updateVerifyData() const {
		for (const auto &info: verifyInfos) {
			bool hashTreeSuccessful = false, fecSuccessful = false;
			if (handleHashTreeDataByInfo(info)) {
				if (updateHashTreeByInfo(info)) {
					hashTreeSuccessful = true;
				}
			}
			if (hashTreeSuccessful && info.hasFecDataExtent) {
				if (handleFecDataByInfo(info)) {
					fecSuccessful = updateFecByInfo(info);
				}
			}
			printVerifyResult(info.name, info.hasFecDataExtent
				                             ? hashTreeSuccessful && fecSuccessful
				                             : hashTreeSuccessful);
		}
	}
}
