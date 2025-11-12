#ifndef ZIP_PARSE_H
#define ZIP_PARSE_H

#include <cstdint>
#include <memory>
#include <vector>

#include "payload/HttpDownload.h"

namespace skkk {
#pragma pack(push, 1)
	struct ZipCentralDirFileHeader {
		uint32_t signature; // 0x02014b50
		uint16_t versionMadeBy;
		uint16_t versionNeeded;
		uint16_t flags;
		uint16_t compressionMethod;
		uint16_t lastModTime;
		uint16_t lastModDate;
		uint32_t crc32;
		uint32_t compressedSize32;
		uint32_t uncompressedSize32;
		uint16_t filenameLength;
		uint16_t extraFieldLength;
		uint16_t commentLength;
		uint16_t diskNumber;
		uint16_t internalAttr;
		uint32_t externalAttr;
		uint32_t localHeaderOffset32;
	};

	struct ZipEOCD {
		uint32_t signature; // 0x06054b50
		uint16_t diskNumber;
		uint16_t diskNumberStart;
		uint16_t numEntriesThisDisk;
		uint16_t totalEntries;
		uint32_t centralDirSize;
		uint32_t centralDirOffset;
		uint16_t commentLength;
	};

	struct Zip64EOCDLocator {
		uint32_t signature; // 0x07064b50
		uint32_t diskNumber;
		uint64_t eocd64Offset;
		uint32_t totalDisks;
	};

	struct Zip64EOCD {
		uint32_t signature; // 0x06064b50
		uint64_t recordSize;
		uint16_t versionMadeBy;
		uint16_t versionNeeded;
		uint32_t diskNumber;
		uint32_t diskNumberStart;
		uint64_t numEntriesThisDisk;
		uint64_t totalEntries;
		uint64_t centralDirSize;
		uint64_t centralDirOffset;
	};

	struct Zip64ExtendedInfo {
		uint16_t headerId; // 0x0001
		uint16_t dataSize;
		// uint64_t uncompressedSize;
		// uint64_t compressedSize;
		// uint64_t localHeaderOffset;
		// uint32_t diskNumber;
	};

	struct ZipLocalHeader {
		uint32_t signature; // 0x04034b50
		uint16_t versionNeeded;
		uint16_t flags;
		uint16_t compressionMethod;
		uint16_t lastModTime;
		uint16_t lastModDate;
		uint32_t crc32;
		uint32_t compressedSize;
		uint32_t uncompressedSize;
		uint16_t filenameLength;
		uint16_t extraFieldLength;
	};

#pragma pack(pop)

	class ZipFileItem {
		public:
			std::string name;
			uint64_t compressedSize;
			uint64_t uncompressedSize;
			uint64_t localHeaderOffset;
			uint64_t offset;
			uint32_t compression;
			uint32_t crc32;

		public:
			ZipFileItem(const std::string &name, const uint64_t compressedSize, const uint64_t uncompressedSize,
			            const uint64_t localHeaderOffset, const uint16_t compression, const uint32_t crc32)
				: name(name), compressedSize(compressedSize), uncompressedSize(uncompressedSize),
				  localHeaderOffset(localHeaderOffset), offset(0), compression(compression), crc32(crc32) {
			}
	};

	class ZipParser {
		public:
			std::shared_ptr<HttpDownload> httpDownload;
			std::string path;
			uint64_t fileDataSize = 0;
			const uint8_t *fileData = nullptr;
			std::vector<ZipFileItem> files;

		public:
			ZipParser(uint64_t fileSize, const uint8_t *fileData);

			explicit ZipParser(const std::shared_ptr<HttpDownload> &httpDownload);

			bool getFileData(uint8_t *data, uint64_t offset, uint64_t len) const;

			uint64_t getZipFileSize() const;

			bool parse();
	};
}

#endif //ZIP_PARSE_H
