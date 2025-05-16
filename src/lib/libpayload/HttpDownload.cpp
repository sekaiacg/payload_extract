#include <cstring>
#include <curl/curl.h>

#include "payload/HttpDownload.h"
#include "payload/HttpUtils.h"
#include "payload/LogBase.h"

namespace skkk {
	HttpDownload::HttpDownload(const std::string &url, bool sslVerification) {
		this->url = url;
		this->sslVerification = sslVerification;
		cprHeader = cpr::Header{
			{"Connection", "close"},
			{
				"User-Agent",
				"Dalvik/2.1.0 (Linux; U; Android 15; RMX5010 Build/AP3A.240617.008)"
			}
		};
		initCA();
	}

	void HttpDownload::initCA() {
#if defined(__linux__)
		if (CA_BUNDLE.empty()) {
			CA_BUNDLE = findCaBundlePath();
		}
		if (CA_PATH.empty()) {
			CA_PATH = findCaPath();
		}
#endif
	}

	void HttpDownload::initSession(cpr::Session &session) const {
		session.SetUrl(url);
		session.SetHeader(cprHeader);
		session.SetConnectTimeout(connectTimeout);
		session.SetLowSpeed(lowSpeed);
		session.SetAcceptEncoding(cpr::AcceptEncoding{"disabled"});
		if (startsWithIgnoreCase(url.c_str(), "https")) {
			session.SetVerifySsl(sslVerification);
			if (sslVerification) {
				CURL *curl = session.GetCurlHolder()->handle;
#if defined(__linux__)
				if (CA_BUNDLE != "NONE") {
					curl_easy_setopt(curl, CURLOPT_CAINFO, CA_BUNDLE.c_str());
					curl_easy_setopt(curl, CURLOPT_PROXY_CAINFO, CA_BUNDLE.c_str());
				}
				if (CA_PATH != "NONE") {
					curl_easy_setopt(curl, CURLOPT_CAPATH, CA_PATH.c_str());
					curl_easy_setopt(curl, CURLOPT_PROXY_CAPATH, CA_PATH.c_str());
				}

				LOGCD("CA_BUNDLE=%s CA_PATH=%s", CA_BUNDLE.c_str(), CA_PATH.c_str());
#elif !defined(__ANDROID__)
				curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif
			}
			LOGCD("Url: SSL verification=%d", sslVerification);
		}
	}

	bool writeDataStr(const std::string_view &data, intptr_t userdata) {
		// NOLINTNEXTLINE (cppcoreguidelines-pro-type-reinterpret-cast)
		auto *dst = reinterpret_cast<std::string *>(userdata);
		*dst += data;
		return true;
	}


	bool HttpDownload::downloadData(std::string &data, uint64_t targetOffset, uint64_t len) const {
		cpr::Session session;
		initSession(session);
		session.SetRange(cpr::Range{targetOffset, targetOffset + len - 1});

		cpr::Response r = session.Download(cpr::WriteCallback{
			writeDataStr,
			reinterpret_cast<intptr_t>(&data)
		});
		if (r.status_code == 206 && r.error.code == cpr::ErrorCode::OK &&
		    r.downloaded_bytes == len) {
			return true;
		}
		LOGCD("download failed hc=%d msg=%s", r.status_code, r.error.message.c_str());
		return false;
	}

	static bool writeDataFb(const std::string_view &data, intptr_t userdata) {
		auto *f = reinterpret_cast<FileBuffer *>(userdata);
		memcpy(f->buf + f->len, data.data(), data.size());
		f->len += data.size();
		return true;
	}

	bool HttpDownload::downloadData(FileBuffer &fb, uint64_t dataOffset, uint64_t targetOffset,
	                                uint64_t len) const {
		cpr::Session session;
		initSession(session);
		session.SetRange(cpr::Range{targetOffset, targetOffset + len - 1});

		fb.buf += dataOffset;
		cpr::Response r = session.Download(cpr::WriteCallback{
			writeDataFb,
			reinterpret_cast<intptr_t>(&fb)
		});
		fb.buf -= dataOffset;
		fb.len = 0;
		if (r.status_code == 206 && r.error.code == cpr::ErrorCode::OK &&
		    r.downloaded_bytes == len) {
			return true;
		}
		LOGCD("download failed hc=%d msg=%s", r.status_code, r.error.message.c_str());
		return false;
	}

	bool HttpDownload::downloadData(FileBuffer &fb, uint64_t targetOffset, uint64_t len) const {
		cpr::Session session;
		initSession(session);
		session.SetRange(cpr::Range{targetOffset, targetOffset + len - 1});
		cpr::Response r = session.Download(cpr::WriteCallback{
			writeDataFb,
			reinterpret_cast<intptr_t>(&fb)

		});
		fb.len = 0;
		if (r.status_code == 206 && r.error.code == cpr::ErrorCode::OK &&
		    r.downloaded_bytes == len) {
			return true;
		}
		LOGCD("download failed hc=%d msg=%s", r.status_code, r.error.message.c_str());
		return false;
	}
}
