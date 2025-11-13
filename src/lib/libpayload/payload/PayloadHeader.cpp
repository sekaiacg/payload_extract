#include <cstring>

#include "common/endian.h"
#include "payload/PayloadHeader.h"

namespace skkk {
	template<typename T>
	void readValueFromBytes(T &value, const uint8_t *data, uint64_t &pos) {
		auto tmp = *reinterpret_cast<const T *>(data + pos);
		const int size = sizeof(T);
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
		pos += size;
	}

	bool parseMagic(PayloadHeader &header, const uint8_t *data, uint64_t &pos) {
		// magic size 4
		memcpy(header.magic, data, PAYLOAD_MAGIC_SIZE);
		pos += PAYLOAD_MAGIC_SIZE;
		return !strncmp(header.magic, PAYLOAD_MAGIC, PAYLOAD_MAGIC_SIZE);
	}

	bool parseFileFormatVersion(PayloadHeader &header, const uint8_t *data, uint64_t &pos) {
		// file_format_version size 8
		readValueFromBytes(header.fileFormatVersion, data, pos);
		return header.isFileFormatVersionValid();
	}

	bool parseManifestSize(PayloadHeader &header, const uint8_t *data, uint64_t &pos) {
		// manifest_size size 8
		readValueFromBytes(header.manifestSize, data, pos);
		return header.manifestSize > 0;
	}

	bool parseMetadataSignatureSize(PayloadHeader &header, const uint8_t *data, uint64_t &pos) {
		// metadata_signature_size size 8
		readValueFromBytes(header.metadataSignatureSize, data, pos);
		return header.metadataSignatureSize > 0;
	}

	bool PayloadHeader::parseHeader(const uint8_t *data) {
		PayloadHeader &header = *this;
		uint64_t &pos = header.inPayloadOffset;
		if (!parseMagic(header, data, pos)) goto out;
		if (!parseFileFormatVersion(header, data, pos)) goto out;
		if (!parseManifestSize(header, data, pos)) goto out;
		if (header.isVersion2()) {
			if (!parseMetadataSignatureSize(header, data, pos)) goto out;
		}

		// pHeader.printHeaderInfo();
		return true;
	out:
		LOGCE("Failed to parse header");
		return false;
	}
}
