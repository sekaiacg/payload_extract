#ifndef PAYLOAD_EXTRACT_PAYLOAD_HEADER_H
#define PAYLOAD_EXTRACT_PAYLOAD_HEADER_H

#include <cstdint>
#include <string>

#include "LogBase.h"

namespace skkk {
	class PayloadHeader {
#define kMaxPayloadHeaderSize 24
#define PAYLOAD_MAGIC "CrAU"
#define PAYLOAD_MAGIC_SIZE 4

		enum file_format_version {
			VERSION_0,
			VERSION_1,
			VERSION_2,
		};

		public:
			uint64_t payloadBinOffset = 0;
			char magic[4] = {0};
			uint64_t fileFormatVersion = VERSION_0; // payload major version
			uint64_t manifestSize = 0; // Size of protobuf DeltaArchiveManifest
			uint32_t metadataSignatureSize = 0; // Only present if format_version >= 2:
			uint8_t *manifest = nullptr; // char manifest[manifest_size];  protobuf
			uint8_t *metadata_signature_message = nullptr; // char[metadata_signature_size]; protobuf

			uint32_t blockSize = 0; // block_size

			// partition info
			int32_t partitionSize = 0;
			uint32_t minorVersion = 0;
			std::string securityPatchLevel;

		public:
			bool isFileFormatVersionValid() const {
				switch (fileFormatVersion) {
					case VERSION_0:
					case VERSION_1:
					case VERSION_2:
						return true;
					default:
						return false;
				}
			}

			bool isVersion2() const {
				return fileFormatVersion >= VERSION_2;
			}


			bool parseHeader(const uint8_t *buf);

			void printHeaderInfo() {
				LOGCI("magic: %.4s", magic);
				LOGCI("file_format_version: %ld", fileFormatVersion);
				LOGCI("manifest_size: %ld", manifestSize);
				LOGCI("metadata_signature_size: %u", metadataSignatureSize);
			}
	};
}

#endif //PAYLOAD_EXTRACT_PAYLOAD_HEADER_H
