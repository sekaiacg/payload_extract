#include "payload/HttpDownload.h"
#include "payload/LogBase.h"
#include "payload/Utils.h"

namespace skkk {
	FileBuffer::FileBuffer(uint8_t *data, uint64_t offset)
		: data(data), offset(offset) {
	}

	HttpDownload::HttpDownload(const std::string &url, bool sslVerification) {
		LOGE("The HttpDownload class is not implemented.");
	}

	void HttpDownload::setUrl(const std::string &url) {
		std::string tmp{url};
		strTrim(tmp);
		this->url = tmp;
	}

	uint64_t HttpDownload::getFileSize() const {
		LOGE("The getFileSize() method is not implemented.");
		return 0;
	}

	std::tuple<bool, long> HttpDownload::download(std::string &data, uint64_t offset, uint64_t length) const {
		LOGE("The download(std::string, uint64_t offset, uint64_t length) is not implemented.");
		return {false, -1};
	}

	std::tuple<bool, long> HttpDownload::download(FileBuffer &fb, uint64_t offset, uint64_t length) const {
		LOGE("The download(FileBuffer &fb, uint64_t offset, uint64_t length) is not implemented.");
		return {false, -1};
	}

	std::tuple<bool, long> HttpDownload::download(FileBuffer &fb, uint64_t fbDataOffset, uint64_t offset,
	                                              uint64_t length) const {
		LOGE(
			"The download(FileBuffer &fb, uint64_t fbDataOffset, uint64_t offset, uint64_t length) is not implemented.");
		return {false, -1};
	}
}
