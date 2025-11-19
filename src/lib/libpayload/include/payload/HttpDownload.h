#ifndef PAYLOAD_EXTRACT_HTTP_DOWNLOAD_H
#define PAYLOAD_EXTRACT_HTTP_DOWNLOAD_H

#include <cinttypes>
#include <tuple>
#include <string>

namespace skkk {
	class FileBuffer {
		public:
			uint8_t *data = nullptr;
			uint32_t offset = 0;

		public:
			FileBuffer(uint8_t *data, uint64_t offset);

			~FileBuffer() { data = nullptr; }
	};

	class HttpDownload {
		public:
			std::string url;
			bool sslVerification = true;

		public:
			HttpDownload() = default;

			HttpDownload(const std::string &url, bool sslVerification);

			virtual ~HttpDownload() = default;

			virtual void setUrl(const std::string &url);

			virtual uint64_t getFileSize() const;

			virtual std::tuple<bool, long> download(std::string &data, uint64_t offset, uint64_t length) const;


			virtual std::tuple<bool, long> download(FileBuffer &fb, uint64_t offset, uint64_t length) const;

			virtual std::tuple<bool, long> download(FileBuffer &fb, uint64_t fbDataOffset, uint64_t offset,
			                                        uint64_t length) const;
	};
}

#endif //PAYLOAD_EXTRACT_HTTP_DOWNLOAD_H
