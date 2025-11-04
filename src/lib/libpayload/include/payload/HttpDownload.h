#ifndef PAYLOAD_EXTRACT_HTTP_DOWNLOAD_H
#define PAYLOAD_EXTRACT_HTTP_DOWNLOAD_H

#include <cinttypes>
#include <string>

namespace skkk {
	class FileBuffer {
		public:
			uint8_t *data = nullptr;
			uint32_t length = 0;

		public:
			FileBuffer(uint8_t *data, const uint32_t length) : data(data), length(length) {
			}

			~FileBuffer() { data = nullptr; }
	};

	class DefaultHttpDownload {
	};

	class HttpDownload {
		public:
			std::string url;
			bool sslVerification = true;

		public:
			HttpDownload() = default;

			HttpDownload(const std::string &url, bool sslVerification);

			virtual ~HttpDownload() = default;

			virtual uint64_t getDlFileSize() const;

			virtual bool download(std::string &data, uint64_t offset, uint64_t length) const;


			virtual bool download(FileBuffer &fb, uint64_t offset, uint64_t length) const;

			virtual bool download(FileBuffer &fb, uint64_t fbDataOffset, uint64_t offset,
			                      uint64_t length) const;
	};
}

#endif //PAYLOAD_EXTRACT_HTTP_DOWNLOAD_H
