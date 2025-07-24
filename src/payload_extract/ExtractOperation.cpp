#include <cinttypes>
#include <iostream>

#include "ExtractOperation.h"
#include "payload/LogBase.h"
#include "payload/threadpool.h"
#include "payload/Utils.h"

namespace skkk {
	void ExtractOperation::setFilePath(const char *path) {
		filePath = path;
		strTrim(filePath);
#if defined(_WIN32)
		handleWinFilePath(filePath);
#endif
		LOGCD("config: filePath=%s", filePath.c_str());
	}

	const std::string &ExtractOperation::getFilePath() const { return filePath; }

	void ExtractOperation::setOutDir(const char *path) { outDir = path; }

	const std::string &ExtractOperation::getOutDir() const { return outDir; }

	int ExtractOperation::initOutDir() {
		int rc = RET_EXTRACT_DONE;
		strTrim(outDir);
		if (outDir.empty()) {
			outDir = "./dump";
		} else {
			if (outDir.size() > 1 &&
			    (outDir.at(outDir.size() - 1) == '/' ||
			     outDir.at(outDir.size() - 1) == '\\'))
				outDir.pop_back();
			if (outDir.size() >= PATH_MAX) {
				LOGE("outDir directory name too long!");
				return RET_EXTRACT_OUTDIR_ROOT;
			}
		}
#if defined( _WIN32)
		handleWinFilePath(filePath);
#endif
		return rc;
	}

	int ExtractOperation::createExtractOutDir() const {
		int rc = RET_EXTRACT_DONE;
		if (!dirExists(outDir)) {
			if (mkdirs(outDir.c_str(), 0755)) {
				rc = RET_EXTRACT_CREATE_DIR_FAIL;
				LOGCE("create out dir fail: '%s'", outDir.c_str());
			}
		}
		return rc;
	}

	void ExtractOperation::initPayloadInfo(const PayloadInfo &pbi) {
		const PayloadHeader &pHeader = pbi.payloadHeader;
		partitionSize = pHeader.partitionSize;
		minorVersion = pHeader.minorVersion;
		securityPatchLevel = pHeader.securityPatchLevel;
	}

	int ExtractOperation::initAllFileNode(const PayloadInfo &pbi) {
		const FileInfoMap &fileMap = pbi.payloadManifest.payloadFileInfo.payloadFileMap;
		fileNodes.reserve(fileMap.size());
		for (const auto &[name, info]: fileMap) {
			fileNodes.emplace_back(pbi, info, outDir);
		}
		return !fileNodes.empty() ? RET_EXTRACT_DONE : RET_EXTRACT_INIT_NODE_FAIL;
	}

	int ExtractOperation::initFileNodeByTarget(const PayloadInfo &pbi) {
#if defined(_WIN32)
		handleWinFilePath(targetNames);
		handleWinFilePath(targetConfigPath);
#endif
		std::vector<std::string> targets;
		targets.reserve(partitionSize);
		fileNodes.reserve(partitionSize);
		splitString(targets, targetNames, ",", true);

		const FileInfoMap &fileMap = pbi.payloadManifest.payloadFileInfo.payloadFileMap;
		if (excludeTargets) {
			for (const auto &[name, file]: fileMap) {
				if (std::ranges::find(targets, name) == targets.end())
					fileNodes.emplace_back(pbi, file, outDir);
			}
		} else {
			for (const auto &target: targets) {
				const auto &file = fileMap.find(target);
				if (file != fileMap.end()) {
					fileNodes.emplace_back(pbi, file->second, outDir);
				}
			}
		}
		return !fileNodes.empty() ? RET_EXTRACT_DONE : RET_EXTRACT_INIT_NODE_FAIL;
	}

	void ExtractOperation::printFiles() const {
		printf("partitionSize: %-3" PRId32 " minorVersion: %-2" PRIu32 " securityPatchLevel: %s\n",
		       partitionSize, minorVersion, securityPatchLevel.c_str());
		for (const auto &fileNode: fileNodes) {
			fileNode.printInfo();
		}
	}

	void ExtractOperation::extractFiles() {
		if (!fileNodes.empty()) {
			LOGCI(GREEN2_BOLD "Using " COLOR_NONE RED2 "%" PRIu32 COLOR_NONE GREEN2_BOLD " threads" COLOR_NONE,
			      threadNum);
			if (threadNum == 1) {
				for (auto &fileNode: fileNodes) {
					fileNode.writeFile(isSilent);
					fileNode.writeExceptionFileIfExists();
				}
			} else {
				for (auto &fileNode: fileNodes) {
					fileNode.writeFileWithMultiThread(threadNum, isSilent);
					fileNode.writeExceptionFileIfExists();
				}
			}
		}
	}
}
