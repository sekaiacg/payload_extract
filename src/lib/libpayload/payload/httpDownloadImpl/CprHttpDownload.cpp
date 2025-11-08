#include "payload/LogBase.h"
#include "payload/httpDownloadImpl/CprHttpDownload.h"
#include "payload/httpDownloadImpl/HttpUtils.h"

namespace skkk {
	CprHttpDownload::CprHttpDownload(const std::string &url, bool sslVerification) {
		this->cprUrl = url;
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

	void CprHttpDownload::initCA() {
#if defined(__linux__)
		if (CA_BUNDLE.empty()) {
			CA_BUNDLE = findCaBundlePath();
		}
		if (CA_PATH.empty()) {
			CA_PATH = findCaPath();
		}
#endif
	}

	void CprHttpDownload::initSession(cpr::Session &session) const {
		session.SetUrl(cprUrl);
		session.SetHeader(cprHeader);
		session.SetConnectTimeout(connectTimeout);
		session.SetLowSpeed(lowSpeed);
		session.SetAcceptEncoding(cpr::AcceptEncoding{"disabled"});
		if (startsWithIgnoreCase(cprUrl.c_str(), "https")) {
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

	uint64_t CprHttpDownload::getFileSize() const {
		cpr::Session session;
		initSession(session);
		uint64_t fileSize = session.GetDownloadFileLength();
		return fileSize > 0 ? fileSize : 0;
	}

	static bool writeDataStr(const std::string_view &data, intptr_t userdata) {
		auto *dst = reinterpret_cast<std::string *>(userdata);
		*dst += data;
		return true;
	}

	bool CprHttpDownload::download(std::string &data, uint64_t offset, uint64_t length) const {
		cpr::Session session;
		initSession(session);
		session.SetRange(cpr::Range{offset, offset + length - 1});
		const auto &r = session.Download(cpr::WriteCallback{
			writeDataStr,
			reinterpret_cast<intptr_t>(&data)
		});
		if (r.status_code == 206 && r.error.code == cpr::ErrorCode::OK &&
		    r.downloaded_bytes == length) {
			return true;
		}
		LOGCD("download failed hc=%d msg=%s", r.status_code, r.error.message.c_str());
		return false;
	}

	static bool writeDataFb(const std::string_view &data, intptr_t userdata) {
		auto *f = reinterpret_cast<FileBuffer *>(userdata);
		memcpy(f->data + f->offset, data.data(), data.size());
		f->offset += data.size();
		return true;
	}

	bool CprHttpDownload::download(FileBuffer &fb, uint64_t offset, uint64_t length) const {
		cpr::Session session;
		initSession(session);
		session.SetRange(cpr::Range{offset, offset + length - 1});
		const auto &r = session.Download(cpr::WriteCallback{
			writeDataFb,
			reinterpret_cast<intptr_t>(&fb)
		});
		if (r.status_code == 206 && r.error.code == cpr::ErrorCode::OK &&
		    r.downloaded_bytes == length) {
			return true;
		}
		LOGCD("download failed hc=%d msg=%s", r.status_code, r.error.message.c_str());
		return false;
	}

	bool CprHttpDownload::download(FileBuffer &fb, uint64_t fbDataOffset, uint64_t offset, uint64_t length) const {
		cpr::Session session;
		initSession(session);
		session.SetRange(cpr::Range{offset, offset + length - 1});

		uint8_t *backDataPtr = fb.data;
		fb.data += fbDataOffset;
		const auto &r = session.Download(cpr::WriteCallback{
			writeDataFb,
			reinterpret_cast<intptr_t>(&fb)
		});
		fb.data = backDataPtr;

		if (r.status_code == 206 && r.error.code == cpr::ErrorCode::OK &&
		    r.downloaded_bytes == length) {
			return true;
		}
		LOGCD("download failed hc=%d msg=%s", r.status_code, r.error.message.c_str());
		return false;
	}
}
