#include <string>

#include "payload/ExtractConfig.h"
#include "payload/Utils.h"

namespace skkk {
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
}
