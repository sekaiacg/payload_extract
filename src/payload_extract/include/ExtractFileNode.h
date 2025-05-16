#ifndef PAYLOAD_EXTRACT_EXTRACTFILENODE_H
#define PAYLOAD_EXTRACT_EXTRACTFILENODE_H

#include <string>
#include <vector>

#include "payload/PayloadFileInfo.h"
#include "payload/PayloadInfo.h"

namespace skkk {
	class ExtractFileNode {
		private:
			std::string name;
			std::string outFilePath;
			std::string outErrorPath;
			uint64_t fileSize = 0;
			std::vector<FileOperation> fileOperations;

			std::atomic_int extractTaskRunCount = 0;
			bool writeSuccess = false;

			//exceptionInfo
			std::atomic_int exceptionSize = 0;
			std::vector<std::string> exceptionInfos;

		public:
			const PayloadInfo *payloadInfo = nullptr;

		public:
			ExtractFileNode() = default;

			ExtractFileNode(const ExtractFileNode &extractFileNode);

			ExtractFileNode(const PayloadInfo &payloadInfo, const FileInfo &fileInfo, const std::string &outDir);

		public:
			void writeExceptionFileIfExists() const;

			int createOutFile() const;

			int openOutFileW() const;

			void printInfo() const;

			bool initExceptionInfo(const FileOperation &fileOperation, int errCode);

			void initExceptionByCreateOutFile(int errCode);

			void printfExceptionIfExists() const;

			bool writeFile(bool isSilent);

			bool writeFileWithMultiThread(uint32_t threadNum, bool isSilent);
	};
}

#endif //PAYLOAD_EXTRACT_EXTRACTFILENODE_H
