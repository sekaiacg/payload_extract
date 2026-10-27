#include <random>
#include <thread>
#include <bsdiff/bspatch.h>

#include "decompress/Decompress.h"
#include "files/io.h"
#include "payload/FileWriter.h"
#include "payload/HttpDownload.h"
#include "payload/update_metadata.pb.h"
#include "payload/common/Buffer.hpp"

namespace skkk {
	using namespace chromeos_update_engine;
	static std::random_device rd;
	static std::mt19937 mt{rd()};
	static std::uniform_int_distribution<uint32_t> randomWaitTime(1200, 3000);

	FileWriter::FileWriter(const std::shared_ptr<HttpDownload> &httpDownload)
		: httpDownload(httpDownload) {
	}

	uint32_t FileWriter::getRdWaitTime() {
		return randomWaitTime(mt);
	}

	int FileWriter::urlRead(uint8_t *data, const FileOperation &operation) const {
		FileBuffer fb{data, 0};

	retry:
		if (std::get<0>(httpDownload->download(fb, operation.dataOffset, operation.dataLength))) {
			return 0;
		}
		fb.offset = 0;
		std::this_thread::sleep_for(std::chrono::milliseconds(getRdWaitTime()));
		goto retry;
	}

	int FileWriter::extentsRead(const int inFd, uint8_t *outData, const std::vector<Extent> &extents) {
		int ret = -1;
		for (const auto &e: extents) {
			ret = IO::blobRead(inFd, outData, e.dataOffset, e.dataLength);
			if (ret) return ret;
			outData += e.dataLength;
		}
		return ret;
	}

	int FileWriter::extentsWrite(const uint8_t *inData, const int outFd, const std::vector<Extent> &extents) {
		int ret = -1;
		for (const auto &e: extents) {
			ret = IO::blobWrite(outFd, inData, e.dataOffset, e.dataLength);
			if (ret) return ret;
			inData += e.dataLength;
		}
		return ret;
	}

	int FileWriter::directWrite(const int payloadFd, const int outFd, const FileOperation &operation) const {
		int ret = -1;
		uint64_t srcDataLength = operation.dataLength;
		Buffer<uint8_t> srcBuffer{srcDataLength};
		if (auto *srcData = srcBuffer.get()) {
			if (httpDownload) {
				ret = urlRead(srcData, operation);
			} else {
				ret = IO::blobRead(payloadFd, srcData, operation.dataOffset, srcDataLength);
			}
			if (!ret) {
				auto &dst = operation.dstExtents[0];
				ret = IO::blobWrite(outFd, srcData, dst.dataOffset, dst.dataLength);
			}
		}
		return ret;
	}

	int FileWriter::decompressWrite(Dec::Decompressor auto &&decompress, const int payloadFd, const int outFd,
	                                const FileOperation &operation) const {
		int ret = -1;
		uint64_t srcDataLength = operation.dataLength;
		Buffer<uint8_t> srcBuffer{srcDataLength};
		if (auto *srcData = srcBuffer.get()) {
			if (httpDownload) {
				ret = urlRead(srcData, operation);
			} else {
				ret = IO::blobRead(payloadFd, srcData, operation.dataOffset, srcDataLength);
			}
			if (!ret) {
				auto &dst = operation.dstExtents[0];
				uint64_t destLength = dst.dataLength;
				Buffer<uint8_t> destBuffer{destLength};
				if (auto *destData = destBuffer.get()) {
					ret = decompress(srcData, srcDataLength, destData, destLength);
					if (!ret) {
						ret = IO::blobWrite(outFd, destData, dst.dataOffset, destLength);
					}
				}
			}
		}
		return ret;
	}

	int FileWriter::bzipWrite(const int payloadFd, const int outFd, const FileOperation &operation) const {
		int ret = decompressWrite(Decompress::bzipDecompress, payloadFd, outFd, operation);
		return ret;
	}

	int FileWriter::zeroWrite(int outFd, const FileOperation &operation) {
		int ret = -1;
		Buffer<uint8_t> srcBuffer{0, operation.dstTotalLength};
		if (auto *srcData = srcBuffer.get()) {
			ret = extentsWrite(srcData, outFd, operation.dstExtents);
		}
		return ret;
	}

	int FileWriter::xzWrite(const int payloadFd, const int outFd, const FileOperation &operation) const {
		int ret = decompressWrite(Decompress::xzDecompress, payloadFd, outFd, operation);
		return ret;
	}

	int FileWriter::zstdWrite(const int payloadFd, const int outFd, const FileOperation &operation) const {
		int ret = decompressWrite(Decompress::zstdDecompress, payloadFd, outFd, operation);
		return ret;
	}

	int FileWriter::sourceCopy(const int payloadFd, const int outFd, const FileOperation &operation) {
		int ret = -1;
		Buffer<uint8_t> srcBuffer{operation.srcTotalLength};
		if (auto *srcData = srcBuffer.get()) {
			ret = extentsRead(payloadFd, srcData, operation.srcExtents);
			if (!ret) {
				auto &dst = operation.dstExtents[0];
				ret = IO::blobWrite(outFd, srcData, dst.dataOffset, dst.dataLength);
			}
		}
		return ret;
	}

	int FileWriter::brotliBSDiff(const int payloadFd, const int oldFd, const int outFd,
	                             const FileOperation &operation) const {
		int ret = -1;
		uint64_t patchDataLength = operation.dataLength;
		Buffer<uint8_t> patchBuffer{patchDataLength};
		if (auto *patchData = patchBuffer.get()) {
			if (httpDownload) {
				ret = urlRead(patchData, operation);
			} else {
				ret = IO::blobRead(payloadFd, patchData, operation.dataOffset, patchDataLength);
			}
			uint64_t srcTotalLength = operation.srcTotalLength;
			Buffer<uint8_t> oldTmpBuffer{srcTotalLength};
			if (auto *oldTmpData = oldTmpBuffer.get()) {
				ret = extentsRead(oldFd, oldTmpData, operation.srcExtents);
				if (!ret) {
					std::vector<uint8_t> patchedData;
					patchedData.reserve(operation.dstTotalLength);
					auto sink = [&patchedData](const uint8_t *data, size_t len) {
						patchedData.insert(patchedData.end(), data, data + len);
						return len;
					};
					ret = bsdiff::bspatch(oldTmpData, srcTotalLength,
					                      patchData, patchDataLength, sink);
					if (!ret) {
						ret = extentsWrite(patchedData.data(), outFd, operation.dstExtents);
					}
				}
			}
		}
		return ret;
	}

	int FileWriter::writeDataByType(const WriteParam &writeParam, const FileOperation &operation) const {
		int ret = -1;
		switch (operation.type) {
			case InstallOperation_Type_REPLACE:
				ret = directWrite(writeParam.payloadFd, writeParam.outFd, operation);
				break;
			case InstallOperation_Type_REPLACE_BZ:
				ret = bzipWrite(writeParam.payloadFd, writeParam.outFd, operation);
				break;
			case InstallOperation_Type_SOURCE_COPY:
				ret = sourceCopy(writeParam.oldFd, writeParam.outFd, operation);
				break;
			case InstallOperation_Type_ZERO:
				ret = zeroWrite(writeParam.outFd, operation);
				break;
			case InstallOperation_Type_REPLACE_XZ:
				ret = xzWrite(writeParam.payloadFd, writeParam.outFd, operation);
				break;
			case InstallOperation_Type_BROTLI_BSDIFF:
				ret = brotliBSDiff(writeParam.payloadFd, writeParam.oldFd, writeParam.outFd, operation);
				break;
			case InstallOperation_Type_REPLACE_ZSTD:
				ret = zstdWrite(writeParam.payloadFd, writeParam.outFd, operation);
				break;
			default:
				ret = -1;
		}
		return ret;
	}
}
