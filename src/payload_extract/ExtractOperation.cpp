#include <cerrno>

#include <payload/LogBase.h>
#include <payload/Utils.h>
#include <payload/common/io.h>

#include "ExtractOperation.h"

namespace skkk {
	int ExtractOperation::initOldDir() const {
		if (!dirExists(oldDir)) {
			LOGCE("oldDir does not exist: '%s'", oldDir.c_str());
			return RET_EXTRACT_INIT_FAIL;
		}
		return RET_EXTRACT_DONE;
	}

	int ExtractOperation::initOutDir() {
		if (outDir.empty()) {
			if (isIncremental) {
				outDir = oldDir + "/patched";
			} else {
				outDir = "./dump";
			}
		} else {
			if (oldDir == outDir) {
				LOGE("oldDir and outDir cannot be the same!");
			}
			if (outDir.size() > 1 &&
			    (outDir.at(outDir.size() - 1) == '/' ||
			     outDir.at(outDir.size() - 1) == '\\'))
				outDir.pop_back();
		}
		return RET_EXTRACT_DONE;
	}

	int ExtractOperation::initOutConfig() {
		if (!fileExists(outConfigPath)) {
			LOGCE("outConfigPath does not exist: '%s'", outConfigPath.c_str());
			return RET_EXTRACT_INIT_FAIL;
		}
		std::vector<std::string> lines;
		if (readAllLines(outConfigPath, lines)) {
			std::vector<std::string> split;
			for (const auto &line: lines) {
				splitString(split, line, ":", true);
				if (split.size() == 2) {
					outConfig[split[0]] = split[1];
					split.clear();
				}
			}
		}
		return outConfig.empty() ? RET_EXTRACT_INIT_FAIL : RET_EXTRACT_DONE;
	}

	int ExtractOperation::initTargetNames() {
		targets.reserve(32);
		splitString(targets, targetName, ",", true);
		return targets.empty() ? RET_EXTRACT_INIT_FAIL : RET_EXTRACT_DONE;
	}

	void ExtractOperation::handleUrl() {
		isUrl = startsWithIgnoreCase(payloadPath, "https://") ||
		        startsWithIgnoreCase(payloadPath, "http://");
		payloadType = isUrl ? PAYLOAD_TYPE_URL : PAYLOAD_TYPE_BIN;
	}

	void ExtractOperation::initHttpDownload() {
		// Http download implement
		httpDownload = getHttpDownloadImpl();
	}

	int ExtractOperation::createExtractOutDir() const {
		int rc = RET_EXTRACT_DONE;
		if (!dirExists(outDir)) {
			if (mkdirs(outDir.c_str(), 0755)) {
				rc = RET_EXTRACT_CREATE_DIR_FAIL;
				LOGCE("create out dir fail: '%s'(%s)", outDir.c_str(), strerror(errno));
			}
		}
		return rc;
	}
}
