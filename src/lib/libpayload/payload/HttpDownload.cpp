#include "payload/HttpDownload.h"
#include "payload/LogBase.h"

namespace skkk {
	HttpDownload::HttpDownload(const std::string &url, bool sslVerification) {
		LOGE("The HttpDownload class is not implemented.");
	}

	uint64_t HttpDownload::getDlFileSize() const {
		LOGE("The getDlFileSize() method is not implemented.");
		return 0;
	}

	bool HttpDownload::download(std::string &data, uint64_t offset, uint64_t length) const {
		LOGE("The download(std::string, uint64_t offset, uint64_t length) is not implemented.");
		return false;
	}

	bool HttpDownload::download(FileBuffer &fb, uint64_t offset, uint64_t length) const {
		LOGE("The download(FileBuffer &fb, uint64_t offset, uint64_t length) is not implemented.");
		return false;
	}

	bool HttpDownload::download(FileBuffer &fb, uint64_t fbDataOffset, uint64_t offset, uint64_t length) const {
		LOGE(
			"The download(FileBuffer &fb, uint64_t fbDataOffset, uint64_t offset, uint64_t length) is not implemented.");
		return false;
	}
}
