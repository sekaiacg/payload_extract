#ifndef PAYLOAD_EXTRACT_HTTP_DOWNLOAD_H
#define PAYLOAD_EXTRACT_HTTP_DOWNLOAD_H

#include <cpr/cpr.h>

namespace skkk {
	class FileBuffer {
		public:
			uint8_t *buf = nullptr;
			size_t len = 0;

		public:
			FileBuffer(uint8_t *buf, const size_t len) : buf(buf), len(len) {
			}

			~FileBuffer() { buf = nullptr; }
	};

	class HttpDownload {
		public:
			static inline std::string CA_BUNDLE;
			static inline std::string CA_PATH;
			cpr::ConnectTimeout connectTimeout{std::chrono::seconds(5)};
			cpr::LowSpeed lowSpeed{1024 * 10, std::chrono::seconds(5)};
			cpr::Url url;
			cpr::Header cprHeader;
			bool sslVerification = true;

		public:
			explicit HttpDownload(const std::string &url, bool sslVerification);

			static void initCA();

			void initSession(cpr::Session &session) const;

			bool downloadData(std::string &data, uint64_t targetOffset, uint64_t len) const;

			bool downloadData(FileBuffer &fb, uint64_t dataOffset, uint64_t targetOffset, uint64_t len) const;

			bool downloadData(FileBuffer &fb, uint64_t targetOffset, uint64_t len) const;
	};
}

#endif //PAYLOAD_EXTRACT_HTTP_DOWNLOAD_H
