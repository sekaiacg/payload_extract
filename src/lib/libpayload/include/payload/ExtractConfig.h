#ifndef PAYLOAD_EXTRACT_EXTRACTCONFIG_H
#define PAYLOAD_EXTRACT_EXTRACTCONFIG_H

#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "HttpDownload.h"
#if defined(ENABLE_HTTP_CPR)
#include "httpDownloadImpl/CprHttpDownload.h"
#endif
#include "PayloadDefs.h"

namespace skkk {
	enum ExtractResult {
		RET_EXTRACT_DONE = 0,
		RET_EXTRACT_CONFIG_DONE = 1,
		RET_EXTRACT_CONFIG_FAIL,
		RET_EXTRACT_INIT_FAIL,
		RET_EXTRACT_INIT_PART_FAIL,
		RET_EXTRACT_OUTDIR_ROOT,
		RET_EXTRACT_OPEN_FILE,
		RET_EXTRACT_CREATE_DIR_FAIL,
		RET_EXTRACT_CREATE_FILE_FAIL,
		RET_EXTRACT_THREAD_NUM_ERROR,
		RET_EXTRACT_FAIL_SKIP,
		RET_EXTRACT_FAIL_EXIT
	};

	class ExtractConfig {
		std::mutex _mutex;

		protected:
			std::string payloadPath;
			std::string oldDir;
			std::string outDir;
			std::string outFIlePath;
			std::string targetName;
			std::vector<std::string> targets;

		public:
			int payloadType = PAYLOAD_TYPE_ZIP;
#if !defined(_WIN32)
			static constexpr bool isWin = false;
#else
			static constexpr bool isWin = true;
#endif
			bool isIncremental = false;
			bool isExcludeMode = false;
			bool isVerifyUpdate = false;
			bool isSilent = false;
			bool isUrl = false;
			bool remoteUpdate = false;
			bool sslVerification = true;
			uint32_t threadNum = 0;
			uint32_t hardwareConcurrency = std::thread::hardware_concurrency();
			uint32_t limitHardwareConcurrency = hardwareConcurrency * 3;
			std::shared_ptr<HttpDownload> httpDownload;

		public:
			ExtractConfig() = default;

			ExtractConfig(int payloadType, const std::string &payloadPath, const std::string &oldDir,
			              const std::string &outDir, bool sslVerification);

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

			virtual const std::shared_ptr<HttpDownload> &getHttpDownloadImpl();
	};
}

#endif //PAYLOAD_EXTRACT_EXTRACTCONFIG_H
