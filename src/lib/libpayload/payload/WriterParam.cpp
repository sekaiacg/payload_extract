#include "payload/FileWriter.h"

namespace skkk {
	WriteParam::WriteParam(const uint8_t *payloadData, const uint8_t *oldData, uint8_t *outData)
		: type(WriteType::BUFFER), payloadData(payloadData), oldData(oldData), outData(outData) {
	}

	WriteParam::WriteParam(const int payloadFd, const int oldFd, const int outFd)
		: type(WriteType::FD), payloadFd(payloadFd), oldFd(oldFd), outFd(outFd) {
	}

	WriteParam::WriteParam(const std::string *payloadPath, const std::string *oldPath, const std::string *outPath)
		: type(WriteType::PATH), payloadPath(payloadPath), oldPath(oldPath), outPath(outPath) {
	}
}
