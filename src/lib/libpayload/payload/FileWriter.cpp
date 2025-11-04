#include <random>
#include <thread>

#include <bsdiff/bspatch.h>

#include "common/io.h"
#include "decompress/Decompress.h"
#include "payload/FileWriter.h"
#include "payload/HttpDownload.h"
#include "payload/update_metadata.pb.h"

using namespace chromeos_update_engine;

namespace skkk {
	static std::random_device rd;
	static std::mt19937 mt{rd()};
	static std::uniform_int_distribution<uint32_t> randomWaitTime(1200, 3000);

	static uint32_t getRdWaitTime() {
		return randomWaitTime(mt);
	}

	int FileWriter::urlRead(uint8_t *buf, const FileOperation &operation) const {
		FileBuffer fb{buf, 0};

	retry:
		if (httpDownload->download(fb, operation.dataOffset, operation.dataLength)) {
			return 0;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(getRdWaitTime()));
		goto retry;
	}

	int FileWriter::commonWrite(const decompressPtr &decompress, int payloadBinFd, int outFd,
	                            const FileOperation &operation) const {
		int ret = -1;
		auto *srcBuf = static_cast<uint8_t *>(malloc(operation.dataLength));
		if (srcBuf) {
			if (httpDownload) {
				ret = urlRead(srcBuf, operation);
			} else {
				ret = blobRead(payloadBinFd, srcBuf, operation.dataOffset, operation.dataLength);
			}
			if (!ret) {
				auto &dst = operation.dstExtents[0];
				auto *destBuf = static_cast<uint8_t *>(malloc(dst.dataLength));
				if (destBuf) {
					uint64_t outDestLength = dst.dataLength;
					ret = decompress(srcBuf, operation.dataLength, destBuf, outDestLength);
					if (!ret) {
						ret = blobWrite(outFd, destBuf, dst.dataOffset, dst.dataLength);
					}
					free(destBuf);
				}
			}
			free(srcBuf);
		}
		return ret;
	}

	int FileWriter::directWrite(int payloadFd, int outFd, const FileOperation &operation) const {
		int ret = -1;
		auto *srcBuf = static_cast<uint8_t *>(malloc(operation.dataLength));
		if (srcBuf) {
			if (httpDownload) {
				ret = urlRead(srcBuf, operation);
			} else {
				ret = blobRead(payloadFd, srcBuf, operation.dataOffset, operation.dataLength);
			}
			if (!ret) {
				auto &dst = operation.dstExtents[0];
				ret = blobWrite(outFd, srcBuf, dst.dataOffset, dst.dataLength);
			}
			free(srcBuf);
		}
		return ret;
	}

	int FileWriter::bzipWrite(int payloadFd, int outFd, const FileOperation &operation) const {
		int ret = commonWrite(Decompress::bzipDecompress,
		                      payloadFd, outFd, operation);
		return ret;
	}

	int FileWriter::zeroWrite(int payloadFd, int outFd, const FileOperation &operation) {
		// static constexpr char buf[1] = {};
		int ret = -1;
		auto &dst = operation.dstExtents[0];
		auto *srcBuf = static_cast<uint8_t *>(calloc(1, dst.dataLength));
		if (srcBuf) {
			ret = blobWrite(outFd, srcBuf, dst.dataOffset, dst.dataLength);
			free(srcBuf);
		}
		return ret;
	}

	int FileWriter::xzWrite(int payloadFd, int outFd, const FileOperation &operation) const {
		int ret = commonWrite(Decompress::xzDecompress,
		                      payloadFd, outFd, operation);
		return ret;
	}

	int FileWriter::zstdWrite(int payloadFd, int outFd, const FileOperation &operation) const {
		int ret = commonWrite(Decompress::zstdDecompress,
		                      payloadFd, outFd, operation);
		return ret;
	}

	int FileWriter::extentsRead(int fd, uint8_t *data, const std::vector<Extent> &extents) {
		int ret = -1;
		for (const auto &e: extents) {
			ret = blobRead(fd, data, e.dataOffset, e.dataLength);
			if (ret) return ret;
			data += e.dataLength;
		}
		return ret;
	}

	int FileWriter::extentsWrite(int fd, uint8_t *data, const std::vector<Extent> &extents) {
		int ret = -1;
		for (const auto &e: extents) {
			ret = blobWrite(fd, data, e.dataOffset, e.dataLength);
			if (ret) return ret;
			data += e.dataLength;
		}
		return ret;
	}

	// TODO copy_file_range
	int FileWriter::sourceCopy(int inFd, int outFd, const FileOperation &operation) {
		int ret = -1;
		auto &dst = operation.dstExtents[0];
		auto *srcBuf = static_cast<uint8_t *>(malloc(operation.srcTotalLength));
		if (srcBuf) {
			ret = extentsRead(inFd, srcBuf, operation.srcExtents);
			if (!ret) {
				ret = blobWrite(outFd, srcBuf, dst.dataOffset, dst.dataLength);
			}
			free(srcBuf);
		}
		return ret;
	}

	int FileWriter::brotliBSDiff(int payloadBinFd, int inFd, int outFd, const FileOperation &operation) const {
		int ret = -1;
		auto &srcs = operation.srcExtents;
		auto &dsts = operation.dstExtents;

		uint64_t patchDataLength = operation.dataLength;
		auto *patchData = static_cast<uint8_t *>(malloc(operation.dataLength));
		if (patchData) {
			if (httpDownload) {
				ret = urlRead(patchData, operation);
			} else {
				ret = blobRead(payloadBinFd, patchData, operation.dataOffset, patchDataLength);
			}
			if (!ret) {
				uint64_t srcTotalLength = operation.srcTotalLength;
				auto *srcBuf = static_cast<uint8_t *>(malloc(srcTotalLength));
				if (srcBuf) {
					ret = extentsRead(inFd, srcBuf, operation.srcExtents);
					if (!ret) {
						std::vector<uint8_t> dstBuf;
						dstBuf.reserve(operation.dstTotalLength * 2);
						auto sink = [&dstBuf](const uint8_t *data, size_t len) {
							dstBuf.insert(dstBuf.end(), data, data + len);
							return len;
						};
						ret = bsdiff::bspatch(srcBuf, srcTotalLength,
						                      patchData, patchDataLength, sink);
						if (!ret) {
							ret = extentsWrite(outFd, dstBuf.data(), dsts);
						}
					}
					free(srcBuf);
				}
			}
			free(patchData);
		}

		return ret;
	}

	int FileWriter::writeDataByType(int payloadBinFd, int inFd, int outFd, const FileOperation &operation) const {
		int ret = -1;
		switch (operation.type) {
			case InstallOperation_Type_REPLACE:
				ret = directWrite(payloadBinFd, outFd, operation);
				break;
			case InstallOperation_Type_REPLACE_BZ:
				ret = bzipWrite(payloadBinFd, outFd, operation);
				break;
			case InstallOperation_Type_SOURCE_COPY:
				ret = sourceCopy(inFd, outFd, operation);
				break;
			case InstallOperation_Type_ZERO:
				ret = zeroWrite(payloadBinFd, outFd, operation);
				break;
			case InstallOperation_Type_REPLACE_XZ:
				ret = xzWrite(payloadBinFd, outFd, operation);
				break;
			case InstallOperation_Type_BROTLI_BSDIFF:
				ret = brotliBSDiff(payloadBinFd, inFd, outFd, operation);
				break;
			case InstallOperation_Type_REPLACE_ZSTD:
				ret = zstdWrite(payloadBinFd, outFd, operation);
				break;
			default:
				ret = -1;
		}
		return ret;
	}
}
