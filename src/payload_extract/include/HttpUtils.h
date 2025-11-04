#ifndef PAYLOAD_EXTRACT_HTTPUTILS_H
#define PAYLOAD_EXTRACT_HTTPUTILS_H

#include <string>
#include <vector>

#include <payload/Utils.h>

static std::string findCaBundlePath() {
	static std::vector files{
		"/etc/ssl/certs/ca-certificates.crt",
		"/etc/pki/tls/certs/ca-bundle.crt",
		"/usr/share/ssl/certs/ca-bundle.crt",
		"/usr/local/share/certs/ca-root-nss.crt",
		"/etc/ssl/cert.pem",
	};
	for (auto file: files) {
		if (fileExists(file)) {
			return file;
		}
	}
	return "NONE";
}

static std::string findCaPath() {
	static std::vector dirs{
		"/etc/ssl/certs",
		"/etc/pki/tls/certs",
		"/usr/share/ssl/certs",
		"/usr/local/share/certs",
		"/etc/ssl",
	};
	for (auto dir: dirs) {
		if (dirExists(dir)) {
			return dir;
		}
	}
	return "NONE";
}

#endif //PAYLOAD_EXTRACT_HTTPUTILS_H
