#include <string>

#include "payload/ExtractConfig.h"
#include "payload/Utils.h"

namespace skkk {
	ExtractConfig::ExtractConfig(int payloadType, const std::string &payloadPath,
	                             const std::string &oldDir, const std::string &outDir,
	                             bool sslVerification)
		: payloadPath(payloadPath),
		  oldDir(oldDir),
		  outDir(outDir),
		  payloadType(payloadType),
		  sslVerification(sslVerification) {
	}

	const std::string &ExtractConfig::getOldDir() const {
		return oldDir;
	}

	void ExtractConfig::setOldDir(const std::string &path) {
		strTrim(oldDir = path);
		handleWinPath(oldDir);
	}

	const std::string &ExtractConfig::getOutDir() const {
		return outDir;
	}

	void ExtractConfig::setOutDir(const std::string &path) {
		strTrim(outDir = path);
		handleWinPath(outDir);
	}

	const std::string &ExtractConfig::getPayloadPath() const {
		return payloadPath;
	}

	void ExtractConfig::setPayloadPath(const std::string &path) {
		strTrim(payloadPath = path);
		handleWinPath(payloadPath);
	}

	const std::string &ExtractConfig::getTargetName() const {
		return targetName;
	}

	void ExtractConfig::setTargetName(const std::string &name) {
		this->targetName = name;
	}

	const std::vector<std::string> &ExtractConfig::getTargets() const {
		return targets;
	}

	void ExtractConfig::setTargets(const std::vector<std::string> &target) {
		targets = target;
	}

	const std::shared_ptr<HttpDownload> &ExtractConfig::getHttpDownloadImpl() {
		std::unique_lock lock(_mutex);
		if (isUrl && !httpDownload) {
#if defined(ENABLE_HTTP_CPR)
			httpDownload = std::make_shared<CprHttpDownload>(payloadPath, sslVerification);
#else
			httpDownload = std::make_shared<HttpDownload>(payloadPath, sslVerification);
#endif
		}
		return httpDownload;
	}
}
