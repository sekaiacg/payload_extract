#ifndef PAYLOAD_EXTRACT_EXTRACTCONFIG_H
#define PAYLOAD_EXTRACT_EXTRACTCONFIG_H

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "HttpDownload.h"
#include "PayloadDefs.h"

namespace skkk {
	enum ExtractResult {
		RET_EXTRACT_DONE = 0,
		RET_EXTRACT_CONFIG_DONE = 1,
		RET_EXTRACT_CONFIG_FAIL,
		RET_EXTRACT_INIT_FAIL,
		RET_EXTRACT_INIT_NODE_FAIL,
		RET_EXTRACT_OUTDIR_ROOT,
		RET_EXTRACT_OPEN_FILE,
		RET_EXTRACT_CREATE_DIR_FAIL,
		RET_EXTRACT_CREATE_FILE_FAIL,
		RET_EXTRACT_THREAD_NUM_ERROR,
		RET_EXTRACT_FAIL_SKIP,
		RET_EXTRACT_FAIL_EXIT
	};

	class ExtractConfig {
		protected:
			std::string payloadPath;
			std::string oldDir;
			std::string outDir;
			std::string outFIlePath;
			std::string targetName;
			std::vector<std::string> targets;

		public:
			int payloadType = PAYLOAD_TYPE_ZIP;
			bool isIncremental = false;
			bool isExcludeMode = false;
			bool isSilent = false;
			bool isUrl = false;
			bool sslVerification = true;
			uint32_t threadNum = 0;
			uint32_t hardwareConcurrency = std::thread::hardware_concurrency();
			uint32_t limitHardwareConcurrency = hardwareConcurrency * 3;
			std::shared_ptr<HttpDownload> httpDownload;

		public:
			ExtractConfig() = default;

			ExtractConfig(int payloadType, const std::string &payloadPath, const std::string &oldDir,
			              const std::string &outDir, bool sslVerification)
				: payloadPath(payloadPath),
				  oldDir(oldDir),
				  outDir(outDir),
				  payloadType(payloadType),
				  sslVerification(sslVerification) {
			}

			ExtractConfig(const ExtractConfig &other)
				: payloadPath(other.payloadPath),
				  oldDir(other.oldDir),
				  outDir(other.outDir),
				  outFIlePath(other.outFIlePath),
				  targetName(other.targetName),
				  targets(other.targets),
				  payloadType(other.payloadType),
				  isIncremental(other.isIncremental),
				  isExcludeMode(other.isExcludeMode),
				  isSilent(other.isSilent),
				  isUrl(other.isUrl),
				  sslVerification(other.sslVerification),
				  threadNum(other.threadNum),
				  hardwareConcurrency(other.hardwareConcurrency),
				  limitHardwareConcurrency(other.limitHardwareConcurrency),
				  httpDownload(other.httpDownload) {
			}

			ExtractConfig &operator=(const ExtractConfig &other) {
				if (this == &other)
					return *this;
				payloadPath = other.payloadPath;
				oldDir = other.oldDir;
				outDir = other.outDir;
				outFIlePath = other.outFIlePath;
				targetName = other.targetName;
				targets = other.targets;
				payloadType = other.payloadType;
				isIncremental = other.isIncremental;
				isExcludeMode = other.isExcludeMode;
				isSilent = other.isSilent;
				isUrl = other.isUrl;
				sslVerification = other.sslVerification;
				threadNum = other.threadNum;
				hardwareConcurrency = other.hardwareConcurrency;
				limitHardwareConcurrency = other.limitHardwareConcurrency;
				httpDownload = other.httpDownload;
				return *this;
			}

			virtual ~ExtractConfig() = default;

			virtual const std::string &getOldDir() const;

			virtual void setOldDir(const std::string &path);

			virtual const std::string &getOutDir() const;

			virtual void setOutDir(const std::string &path);

			virtual const std::string &getPayloadPath() const;

			virtual void setPayloadPath(const std::string &path);

			virtual const std::string &getTargetName() const;

			virtual void setTargetName(const std::string &name);

			virtual const std::vector<std::string> &getTargets() const;

			virtual void setTargets(const std::vector<std::string> &target);
	};
}

#endif //PAYLOAD_EXTRACT_EXTRACTCONFIG_H
