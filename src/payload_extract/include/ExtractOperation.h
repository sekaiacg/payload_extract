#ifndef PAYLOAD_EXTRACT_EXTRACTOPERATION_H
#define PAYLOAD_EXTRACT_EXTRACTOPERATION_H

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#include "payload/PayloadInfo.h"
#include "ExtractFileNode.h"

namespace skkk {
	enum DumpResult {
		RET_EXTRACT_DONE = 0,
		RET_EXTRACT_CONFIG_DONE = 1,
		RET_EXTRACT_CONFIG_FAIL,
		RET_EXTRACT_INIT_FAIL,
		RET_EXTRACT_INIT_NODE_FAIL,
		RET_EXTRACT_OUTDIR_ROOT,
		RET_EXTRACT_CREATE_DIR_FAIL,
		RET_EXTRACT_CREATE_FILE_FAIL,
		RET_EXTRACT_THREAD_NUM_ERROR,
		RET_EXTRACT_FAIL_SKIP,
		RET_EXTRACT_FAIL_EXIT
	};

	class ExtractOperation {
		private:
			std::string filePath;
			std::string outDir;
			//		string configDir;
			std::vector<ExtractFileNode> fileNodes;

			int32_t partitionSize = 0;
			uint32_t minorVersion = 0;
			std::string securityPatchLevel;

		public:
			bool isPrintAll = false;
			bool isPrintTarget = false;
			bool isExtractAll = false;
			bool isExtractTarget = false;
			bool excludeTargets = false;
			bool isExtractTargetConfig = false;
			bool isSilent = false;
			int payloadType = PAYLOAD_TYPE_BIN;
			bool sslVerification = true;
			std::string targetNames;
			std::string targetConfigPath;
			uint32_t threadNum = 0;
			uint32_t hardwareConcurrency = std::thread::hardware_concurrency();
			uint32_t limitHardwareConcurrency = hardwareConcurrency * 3;

		public:
			void setFilePath(const char *path);

			const std::string &getFilePath() const;

			void setOutDir(const char *path);

			const std::string &getOutDir() const;

			int initOutDir();

			int createExtractOutDir() const;

			void initPayloadInfo(const PayloadInfo &pbi);

			int initAllFileNode(const PayloadInfo &pbi);

			int initFileNodeByTarget(const PayloadInfo &pbi);

			void printFiles() const;

			void extractFiles();
	};
}

#endif //PAYLOAD_EXTRACT_EXTRACTOPERATION_H
