#include <random>
#include <thread>
#include <bsdiff/bspatch.h>

#include "decompress/Decompress.h"
#include "payload/FileWriter.h"
#include "payload/HttpDownload.h"
#include "payload/update_metadata.pb.h"
#include "payload/Utils.h"
#include "payload/common/Buffer.hpp"
#include "payload/common/io.h"

using namespace chromeos_update_engine;

namespace skkk {
	static std::random_device rd;
	static std::mt19937 mt{rd()};
	static std::uniform_int_distribution<uint32_t> randomWaitTime(1200, 3000);

	static uint32_t getRdWaitTime() {
		return randomWaitTime(mt);
	}

	FileWriter::FileWriter(const std::shared_ptr<HttpDownload> &httpDownload)
		: httpDownload(httpDownload) {
	}

	int FileWriter::urlRead(uint8_t *buf, const FileOperation &operation) const {
		FileBuffer fb{buf, 0};

	retry:
		if (std::get<0>(httpDownload->download(fb, operation.dataOffset, operation.dataLength))) {
			return 0;
		}
		fb.offset = 0;
		std::this_thread::sleep_for(std::chrono::milliseconds(getRdWaitTime()));
		goto retry;
	}

	int FileWriter::commonWrite(const decompressPtr &decompress, const uint8_t *payloadData, uint8_t *outData,
	                            const FileOperation &operation) const {
		int ret = -1;
		uint8_t *srcData = nullptr;
		Buffer<uint8_t> srcBuffer;
		if (httpDownload) {
			srcBuffer.reserve(operation.dataLength);
			srcData = srcBuffer.get();
			urlRead(srcData, operation);
		} else {
			srcData = const_cast<uint8_t *>(payloadData + operation.dataOffset);
		}
		if (srcData) {
			auto &dst = operation.dstExtents[0];
			Buffer<uint8_t> destBuffer{dst.dataLength};
			auto *destBuf = destBuffer.get();
			if (destBuf) {
				uint64_t outDestLength = dst.dataLength;
				ret = decompress(srcData, operation.dataLength, destBuf, outDestLength);
				if (!ret) {
					ret = memcpy(outData + dst.dataOffset, destBuf, dst.dataLength) ? 0 : -EIO;
				}
			}
		}
		return ret;
	}

	int FileWriter::directWrite(const uint8_t *payloadData, uint8_t *outData, const FileOperation &operation) const {
		int ret = -1;
		uint8_t *srcData = nullptr;
		Buffer<uint8_t> srcBuffer;
		if (httpDownload) {
			srcBuffer.reserve(operation.dataLength);
			srcData = srcBuffer.get();
			urlRead(srcData, operation);
		} else {
			srcData = const_cast<uint8_t *>(payloadData + operation.dataOffset);
		}
		if (srcData) {
			auto &dst = operation.dstExtents[0];
			ret = memcpy(outData + dst.dataOffset, srcData, dst.dataLength) ? 0 : -EIO;
		}
		return ret;
	}

	int FileWriter::bzipWrite(const uint8_t *payloadData, uint8_t *outData, const FileOperation &operation) const {
		int ret = commonWrite(Decompress::bzipDecompress,
		                      payloadData, outData, operation);
		return ret;
	}

	int FileWriter::zeroWrite(const uint8_t *payloadData, uint8_t *outData, const FileOperation &operation) {
		int ret = -1;
		auto &dst = operation.dstExtents[0];
		Buffer<uint8_t> srcBuffer{0, dst.dataLength};
		auto *srcData = srcBuffer.get();
		if (srcData) {
			ret = memcpy(outData + dst.dataOffset, srcData, dst.dataLength) ? 0 : -EIO;
		}
		return ret;
	}

	int FileWriter::xzWrite(const uint8_t *payloadData, uint8_t *outData, const FileOperation &operation) const {
		int ret = commonWrite(Decompress::xzDecompress,
		                      payloadData, outData, operation);
		return ret;
	}

	int FileWriter::zstdWrite(const uint8_t *payloadData, uint8_t *outData, const FileOperation &operation) const {
		int ret = commonWrite(Decompress::zstdDecompress,
		                      payloadData, outData, operation);
		return ret;
	}

	int FileWriter::extentsRead(const uint8_t *inData, uint8_t *data, const std::vector<Extent> &extents) {
		int ret = -1;
		for (const auto &e: extents) {
			ret = memcpy(data, inData + e.dataOffset, e.dataLength) == data ? 0 : -EIO;
			if (ret) return ret;
			data += e.dataLength;
		}
		return ret;
	}

	int FileWriter::extentsWrite(uint8_t *outData, const uint8_t *srcData, const std::vector<Extent> &extents) {
		int ret = -1;
		for (const auto &e: extents) {
			ret = memcpy(outData + e.dataOffset, srcData, e.dataLength) ? 0 : -EIO;
			if (ret) return ret;
			srcData += e.dataLength;
		}
		return ret;
	}

	int FileWriter::sourceCopy(const uint8_t *inData, uint8_t *outData, const FileOperation &operation) {
		int ret = -1;
		auto &dst = operation.dstExtents[0];
		Buffer<uint8_t> srcBuffer{operation.srcTotalLength};
		auto *srcData = srcBuffer.get();
		if (srcData) {
			ret = extentsRead(inData, srcData, operation.srcExtents);
			if (!ret) {
				ret = memcpy(outData + dst.dataOffset, srcData, dst.dataLength) ? 0 : -EIO;
			}
		}
		return ret;
	}

	int FileWriter::brotliBSDiff(const uint8_t *payloadData, const uint8_t *inData, uint8_t *outData,
	                             const FileOperation &operation) const {
		int ret = -1;
		auto &srcs = operation.srcExtents;
		auto &dsts = operation.dstExtents;
		uint64_t patchDataLength = operation.dataLength;
		uint8_t *patchData = nullptr;
		Buffer<uint8_t> patchBuffer;
		if (httpDownload) {
			patchBuffer.reserve(patchDataLength);
			patchData = patchBuffer.get();
			urlRead(patchData, operation);
		} else {
			patchData = const_cast<uint8_t *>(payloadData + operation.dataOffset);
		}
		if (patchData) {
			uint64_t srcTotalLength = operation.srcTotalLength;
			Buffer<uint8_t> srcBuffer{srcTotalLength};
			auto *srcData = srcBuffer.get();
			if (srcData) {
				ret = extentsRead(inData, srcData, operation.srcExtents);
				if (!ret) {
					std::vector<uint8_t> patchedData;
					patchedData.reserve(operation.dstTotalLength);
					auto sink = [&patchedData](const uint8_t *data, size_t len) {
						patchedData.insert(patchedData.end(), data, data + len);
						return len;
					};
					ret = bsdiff::bspatch(srcData, srcTotalLength,
					                      patchData, patchDataLength, sink);
					if (!ret) {
						ret = extentsWrite(outData, patchedData.data(), dsts);
					}
				}
			}
		}

		return ret;
	}

	int FileWriter::writeDataByType(const uint8_t *payloadData, const uint8_t *inData, uint8_t *outData,
	                                const FileOperation &operation) const {
		int ret = -1;
		switch (operation.type) {
			case InstallOperation_Type_REPLACE:
				ret = directWrite(payloadData, outData, operation);
				break;
			case InstallOperation_Type_REPLACE_BZ:
				ret = bzipWrite(payloadData, outData, operation);
				break;
			case InstallOperation_Type_SOURCE_COPY:
				ret = sourceCopy(inData, outData, operation);
				break;
			case InstallOperation_Type_ZERO:
				ret = zeroWrite(nullptr, outData, operation);
				break;
			case InstallOperation_Type_REPLACE_XZ:
				ret = xzWrite(payloadData, outData, operation);
				break;
			case InstallOperation_Type_BROTLI_BSDIFF:
				ret = brotliBSDiff(payloadData, inData, outData, operation);
				break;
			case InstallOperation_Type_REPLACE_ZSTD:
				ret = zstdWrite(payloadData, outData, operation);
				break;
			default:
				ret = -1;
		}
		return ret;
	}
}
