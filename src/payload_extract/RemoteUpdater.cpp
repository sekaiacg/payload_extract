#include <unistd.h>

#include <files/io.h>
#include <files/mmap.hpp>
#include <payload/LogBase.h>

#include "RemoteUpdater.h"

namespace skkk {
	RemoteUpdater::RemoteUpdater(const ExtractConfig &config)
		: config(config) {
		contextFilePath = config.getOutDir() + "/remote_ctl";
	}

	RemoteUpdater::~RemoteUpdater() {
		monitoring = false;
		std::lock_guard lock{_mutex};
		if (context) {
			munmap(context, contextSize);
		}
		IO::closeFd(mapFd);
		if (firstStage && IO::fileExists(contextFilePath)) {
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
		this->firstStage = firstStage;
		mapFd = IO::createFile(contextFilePath, firstStage);
		if (mapFd > 0) {
			if (!IO::blobFtruncate(mapFd, size)) {
				return mapByPath(mapFd, contextFilePath, context, size, false) == 0;
			}
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

	void RemoteUpdater::handleRemoteUpdate() const {
		if (context && context->startUpdate) {
			printRemoteResult(context->url, true);
			context->startUpdate = false;
			config.httpDownload->setUrl(context->url);
		}
	}

	void RemoteUpdater::monitor() const {
		while (monitoring) {
			std::lock_guard lock{_mutex};
			handleRemoteUpdate();
			sleep(1);
		}
	}

	void RemoteUpdater::startMonitor() {
		std::lock_guard lock{_mutex};
		if (!monitoring && context) {
			monitoring = true;
			monitorFuture = std::async(std::launch::async, &RemoteUpdater::monitor, std::ref(*this));
		}
	}
}
