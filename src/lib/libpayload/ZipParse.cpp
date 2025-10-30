#include "payload/io.h"
#include "payload/Utils.h"
#include "payload/ZipParse.h"

namespace skkk {
	bool ZipParse::getFileData(uint8_t *data, uint64_t offset, uint64_t len) const {
		if (isUrl) {
			FileBuffer fb{data, 0};
			return httpDownload.downloadData(fb, offset, len);
		}
		return blobRead(inFileFd, data, offset, len) == 0;
	}

	uint64_t ZipParse::getZipFileSize() const {
		if (isUrl) {
			return httpDownload.getFileSize();
		}
		return getFileSize(path);
	}

	bool ZipParse::parse() {
		constexpr uint64_t eocdSize = sizeof(ZipEOCD);
		const uint64_t fileSize = getZipFileSize();
		if (fileSize < eocdSize) return false;

		constexpr uint64_t maxEOCDSize = 4096;
		const uint64_t searchSize = std::min(fileSize - eocdSize, maxEOCDSize);
		uint64_t eocdStartOffset = 0;
		bool foundEOCD = false;
		ZipEOCD eocd = {};

		std::string buffer;
		buffer.reserve(maxEOCDSize * 2);
		const auto buf = reinterpret_cast<uint8_t *>(buffer.data());

		if (!getFileData(buf, fileSize - searchSize - eocdSize, searchSize)) return false;
		for (uint64_t offset = 0; offset < searchSize; ++offset) {
			eocdStartOffset = fileSize - eocdSize - offset;
			const auto *tmpEOCD = reinterpret_cast<ZipEOCD *>(buf + searchSize - offset);
			if (tmpEOCD->signature == 0x06054b50 && tmpEOCD->commentLength == offset) {
				memcpy(&eocd, tmpEOCD, eocdSize);
				foundEOCD = true;
				break;
			}
		}

		if (!foundEOCD) {
			return false;
		}

		const bool isZip64 = eocd.totalEntries == 0xFFFF ||
		                     eocd.centralDirOffset == 0xFFFFFFFF ||
		                     eocd.numEntriesThisDisk == 0xFFFF;

		Zip64EOCDLocator eocd64Locator = {};
		if (isZip64) {
			const uint64_t locatorPos = eocdStartOffset - sizeof(Zip64EOCDLocator);
			if (!getFileData(reinterpret_cast<uint8_t *>(&eocd64Locator),
			                 locatorPos, sizeof(Zip64EOCDLocator)))
				return false;
			if (eocd64Locator.signature != 0x07064b50) {
				return false;
			}
		}

		uint64_t centralDirOffset = isZip64 ? eocd64Locator.eocd64Offset : eocd.centralDirOffset;
		Zip64EOCD eocd64 = {};
		if (isZip64) {
			if (!getFileData(reinterpret_cast<uint8_t *>(&eocd64),
			                 centralDirOffset, sizeof(Zip64EOCD)))
				return false;
			if (eocd64.signature != 0x06064b50) {
				return false;
			}

			centralDirOffset = eocd64.centralDirOffset;
		}

		const uint64_t totalEntries = isZip64 ? eocd64.totalEntries : eocd.totalEntries;
		const uint64_t centralDirSize = isZip64 ? eocd64.centralDirSize : eocd.centralDirSize;

		if (!getFileData(buf, centralDirOffset, centralDirSize)) return false;
		files.reserve(32);
		uint64_t entryOffset = 0;
		for (uint64_t i = 0; i < totalEntries; ++i) {
			const auto *header = reinterpret_cast<ZipCentralDirFileHeader *>(buf + entryOffset);
			if (header->signature != 0x02014b50) return false;
			entryOffset += sizeof(ZipCentralDirFileHeader);

			const std::string filename{
				reinterpret_cast<char *>(buf) + entryOffset,
				0, header->filenameLength
			};
			entryOffset += header->filenameLength;

			uint64_t uncompressedSize = header->uncompressedSize32;
			uint64_t compressedSize = header->compressedSize32;
			uint64_t localHeaderOffset = header->localHeaderOffset32;

			if (header->extraFieldLength > 0) {
				const auto extra = reinterpret_cast<Zip64ExtendedInfo *>(buf + entryOffset);
				if (extra->headerId == 0x0001) {
					uint32_t dataConsumed = 0;
					auto *data = buf + entryOffset + sizeof(Zip64ExtendedInfo);

					if (uncompressedSize == 0xFFFFFFFF && dataConsumed + 8 <= extra->dataSize) {
						uncompressedSize = *reinterpret_cast<uint64_t *>(data + dataConsumed);
						dataConsumed += 8;
					}

					if (compressedSize == 0xFFFFFFFF && dataConsumed + 8 <= extra->dataSize) {
						compressedSize = *reinterpret_cast<uint64_t *>(data + dataConsumed);
						dataConsumed += 8;
					}

					if (localHeaderOffset == 0xFFFFFFFF && dataConsumed + 8 <= extra->dataSize) {
						localHeaderOffset = *reinterpret_cast<uint64_t *>(data + dataConsumed);
						dataConsumed += 8;
					}
				}
				entryOffset += header->extraFieldLength;
			}
			entryOffset += header->commentLength;

			files.emplace_back(filename, uncompressedSize, compressedSize,
			                   localHeaderOffset, header->compressionMethod, header->crc32);
		}

		return true;
	}
}
