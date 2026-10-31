#ifndef PAYLOAD_EXTRACT_CPRHTTPDOWNLOAD_H
#define PAYLOAD_EXTRACT_CPRHTTPDOWNLOAD_H

#include <chrono>
#include <cpr/cpr.h>

#include "payload/HttpDownload.h"

using namespace std::chrono_literals;

namespace skkk {
	class CprHttpDownload : public HttpDownload {
		public:
			cpr::ConnectionPool connectionPool{};
			cpr::ConnectTimeout connectTimeout{5s};
			cpr::LowSpeed lowSpeed{1024 * 10, 5s};
			cpr::Url cprUrl;
			cpr::Header cprHeader;

		public:
			CprHttpDownload(const std::string &url, bool sslVerification);

			void initSession(cpr::Session &session) const;

			void setUrl(const std::string &url) override;

			uint64_t getFileSize() const override;

			std::tuple<bool, long> download(std::string &data, uint64_t offset, uint64_t length) const override;

			std::tuple<bool, long> download(FileBuffer &fb, uint64_t offset, uint64_t length) const override;

			std::tuple<bool, long> download(FileBuffer &fb, uint64_t fbDataOffset, uint64_t offset,
			                                uint64_t length) const override;
	};
}

#endif //PAYLOAD_EXTRACT_CPRHTTPDOWNLOAD_H
