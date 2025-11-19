#ifndef PAYLOAD_EXTRACT_REMOTEUPDATE_H
#define PAYLOAD_EXTRACT_REMOTEUPDATE_H

#include <future>
#include <mutex>

#include <payload/ExtractConfig.h>

namespace skkk {
	class RemoteContext {
		public:
			bool startUpdate = false;
			char url[1024] = {};
	};

	class RemoteUpdater {
			std::mutex mutex_;
			int mapFd = -1;
			RemoteContext *context = nullptr;
			static constexpr uint32_t contextSize = sizeof(RemoteContext);
			bool firstStage = false;
			const ExtractConfig &config;
			std::string contextFilePath;
			std::future<void> monitorFuture;
			std::atomic<bool> monitoring = false;

		public:
			explicit RemoteUpdater(const ExtractConfig &config);

			~RemoteUpdater();

			bool initRemoteUpdate(bool firstStage);

			void notifyRemoteUpdate() const;

			void handleRemoteUpdate();

			void monitor();

			void startMonitor();
	};
}

#endif //PAYLOAD_EXTRACT_REMOTEUPDATE_H
