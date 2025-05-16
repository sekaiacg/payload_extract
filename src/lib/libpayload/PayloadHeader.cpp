#include <cstring>
#include <inttypes.h>

#include "payload/endian.h"
#include "payload/PayloadHeader.h"

namespace skkk {
	template<typename T>
	void readValueFromBytes(T &value, const uint8_t *buf, uint64_t &offset) {
		T tmp = 0;
		int size = sizeof(T);
		memcpy(reinterpret_cast<uint8_t *>(&tmp), buf + offset, size);
		offset += size;
		switch (size) {
			case 2:
				value = be16toh(tmp);
				break;
			case 4:
				value = be32toh(tmp);
				break;
			case 8:
				value = be64toh(tmp);
				break;
			default:
				value = 0;
		}
	}

	bool parseMagic(PayloadHeader &header, const uint8_t *buf, uint64_t &offset) {
		// magic size 4
		memcpy(header.magic, buf, PAYLOAD_MAGIC_SIZE);
		offset += PAYLOAD_MAGIC_SIZE;
		return !strncmp(header.magic, PAYLOAD_MAGIC, PAYLOAD_MAGIC_SIZE);
	}

	bool parseFileFormatVersion(PayloadHeader &header, const uint8_t *buf, uint64_t &offset) {
		// file_format_version size 8
		readValueFromBytes(header.fileFormatVersion, buf, offset);
		return header.isFileFormatVersionValid();
	}

	bool parseManifestSize(PayloadHeader &header, const uint8_t *buf, uint64_t &offset) {
		// manifest_size size 8
		readValueFromBytes(header.manifestSize, buf, offset);
		return header.manifestSize > 0;
	}

	bool parseMetadataSignatureSize(PayloadHeader &header, const uint8_t *buf, uint64_t &offset) {
		// metadata_signature_size size 8
		readValueFromBytes(header.metadataSignatureSize, buf, offset);
		return header.metadataSignatureSize > 0;
	}

	bool PayloadHeader::parseHeader(const uint8_t *buf) {
		PayloadHeader &pHeader = *this;
		uint64_t &offset = pHeader.payloadBinOffset;
		if (!parseMagic(pHeader, buf, offset)) goto out;
		if (!parseFileFormatVersion(pHeader, buf, offset)) goto out;
		if (!parseManifestSize(pHeader, buf, offset)) goto out;
		if (pHeader.isVersion2()) {
			if (!parseMetadataSignatureSize(pHeader, buf, offset)) goto out;
		}

		// pHeader.printHeaderInfo();
		return true;
	out:
		LOGCE("Failed to parse header");
		return false;
	}
}
