#include "payload/Decompress.h"
#include "payload/FileWriter.h"
#include "payload/HttpDownload.h"
#include "payload/io.h"
#include "payload/Utils.h"

namespace skkk {
	int FileWriter::urlRead(uint8_t *buf, const FileOperation &operation) {
		HttpDownload hd{operation.url, operation.sslVerification};
		FileBuffer fb{buf, 0};

	retry:
		if (hd.downloadData(fb, operation.dataOffset, operation.dataLength)) {
			return 0;
		}
		sleep(2);
		goto retry;
		return -1;
	}

	int FileWriter::commonWrite(const decompress_func_t decompress_func, int payloadBinFd, int outFd,
	                            const FileOperation &operation) {
		int ret = -1;
		uint8_t *srcBuf = static_cast<uint8_t *>(malloc(operation.dataLength));
		if (srcBuf) {
			if (operation.isUrl) {
				ret = urlRead(srcBuf, operation);
			} else {
				ret = blobRead(payloadBinFd, srcBuf, operation.dataOffset, operation.dataLength);
			}
			if (!ret) {
				char *destBuf = static_cast<char *>(malloc(operation.destLength));
				if (destBuf) {
					uint64_t outDestLength = operation.destLength;
					ret = decompress_func(srcBuf, operation.dataLength, destBuf, outDestLength);
					if (!ret) {
						ret = blobWrite(outFd, destBuf, operation.fileOffset, operation.destLength);
					}
					free(destBuf);
				}
			}
			free(srcBuf);
		}
		return ret;
	}

	int FileWriter::directWrite(int payloadFd, int outFd, const FileOperation &operation) {
		int ret = -1;
		uint8_t *srcBuf = static_cast<uint8_t *>(malloc(operation.dataLength));
		if (srcBuf) {
			if (operation.isUrl) {
				ret = urlRead(srcBuf, operation);
			} else {
				ret = blobRead(payloadFd, srcBuf, operation.dataOffset, operation.dataLength);
			}
			if (!ret) {
				ret = blobWrite(outFd, srcBuf, operation.fileOffset, operation.destLength);
			}
			free(srcBuf);
		}
		return ret;
	}

	int FileWriter::zeroWrite(int payloadFd, int outFd, const FileOperation &operation) {
		static constexpr char buf[1] = {0};
		return blobWrite(outFd, buf, operation.fileOffset, 1);
	}

	int FileWriter::bzipWrite(int payloadFd, int outFd, const FileOperation &operation) {
		int ret = commonWrite(Decompress::bzipDecompress,
		                      payloadFd, outFd, operation);
		return ret;
	}


	int FileWriter::xzWrite(int payloadFd, int outFd, const FileOperation &operation) {
		int ret = commonWrite(Decompress::xzDecompress,
		                      payloadFd, outFd, operation);
		return ret;
	}

	int FileWriter::zstdWrite(int payloadFd, int outFd, const FileOperation &operation) {
		int ret = commonWrite(Decompress::zstdDecompress,
		                      payloadFd, outFd, operation);
		return ret;
	}

	int FileWriter::writeDataByType(int payloadBinFd, int outFd, const FileOperation &operation) {
		int ret = 0;
		switch (operation.type) {
			case InstallOperation_Type_REPLACE:
				ret = directWrite(payloadBinFd, outFd, operation);
				break;
			case InstallOperation_Type_REPLACE_BZ:
				ret = bzipWrite(payloadBinFd, outFd, operation);
				break;
			case InstallOperation_Type_ZERO:
				ret = zeroWrite(payloadBinFd, outFd, operation);
				break;
			case InstallOperation_Type_REPLACE_XZ:
				ret = xzWrite(payloadBinFd, outFd, operation);
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
