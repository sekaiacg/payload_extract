#ifndef PAYLOAD_EXTRACT_BIN_UTILS_H
#define PAYLOAD_EXTRACT_BIN_UTILS_H

#include <cinttypes>

static uint64_t getShort(const unsigned char *buf, uint64_t offset) {
	return buf[offset] | (static_cast<uint64_t>(buf[offset + 1]) << 8);
}

static uint64_t getLong(const unsigned char *buf, uint64_t offset) {
	return buf[offset] | (static_cast<uint64_t>(buf[offset + 1]) << 8) | (
		       static_cast<uint64_t>(buf[offset + 2]) << 16) | (
		       static_cast<uint64_t>(buf[offset + 3]) << 24);
}

static uint64_t getLong64(const unsigned char *buf, uint64_t offset) {
	return buf[offset] | (static_cast<uint64_t>(buf[offset + 1]) << 8) |
	       (static_cast<uint64_t>(buf[offset + 2]) << 16) | (static_cast<uint64_t>(buf[offset + 3]) << 24) |
	       (static_cast<uint64_t>(buf[offset + 4]) << 32) | (static_cast<uint64_t>(buf[offset + 5]) << 40) |
	       (static_cast<uint64_t>(buf[offset + 6]) << 48) | (static_cast<uint64_t>(buf[offset + 7]) << 56);
}
#endif //PAYLOAD_EXTRACT_BIN_UTILS_H
