#include <memory>

#include <payload/LogBase.h>
#include "files/mman/mmap.hpp"

#include "RemoteUpdater.h"

namespace skkk {
	RemoteUpdater::RemoteUpdater(const ExtractConfig &config)
		: config(config) {
		contextFilePath = config.getOutDir() + "/remote_ctl";
	}

	RemoteUpdater::~RemoteUpdater() {
		monitoring = false;
		if (context) {
			munmap(context, contextSize);
		}
		closeFd(mapFd);
		if (firstStage && fileExists(contextFilePath)) {
			remove(contextFilePath.c_str());
		}
	}

	static void printRemoteResult(const std::string &text, bool success) {
		static auto *successFmt = BLUE_BOLD("Remote : ") GREEN2_BOLD("%s") "\n";
		static auto *failFmt = BLUE_BOLD("Remote : ") RED2("%s") "\n";
		printf(success ? successFmt : failFmt, text.c_str());
	}

	bool RemoteUpdater::initRemoteUpdate(bool firstStage) {
		uint64_t size = contextSize;
		int flag = O_RDWR | O_BINARY;
		this->firstStage = firstStage;

		if (firstStage) flag |= O_CREAT | O_TRUNC;
		mapFd = open(contextFilePath.c_str(), flag, 0644);
		if (mapFd > 0) {
			if (!payload_ftruncate(mapFd, contextSize)) {
				return mapByPath(mapFd, contextFilePath, context, size, false) == 0;
			}
			close(mapFd);
		}
		printRemoteResult("Failed to initialize!", false);
		return false;
	}

	void RemoteUpdater::notifyRemoteUpdate() const {
		if (config.isUrl && context) {
			const std::string url = config.getPayloadPath();
			strncpy(context->url, url.c_str(), url.size());
			context->url[url.size()] = '\0';
			context->startUpdate = true;
			printRemoteResult("Update successful!", true);
		}
	}

	void RemoteUpdater::handleRemoteUpdate() {
		std::lock_guard lock{_mutex};
		if (context->startUpdate) {
			printRemoteResult(context->url, true);
			context->startUpdate = false;
			config.httpDownload->setUrl(context->url);
		}
	}

	void RemoteUpdater::monitor() {
		while (monitoring) {
			handleRemoteUpdate();
			sleep(1);
		}
	}

	void RemoteUpdater::startMonitor() {
		if (!monitoring && context) {
			monitoring = true;
			monitorFuture = std::async(std::launch::async, &RemoteUpdater::monitor, std::ref(*this));
		}
	}
}
