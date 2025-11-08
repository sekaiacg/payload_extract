#ifndef PAYLOAD_EXTRACT_LOGPROGRESS_H
#define PAYLOAD_EXTRACT_LOGPROGRESS_H

#include <cmath>
#include <string>
#include <atomic>

namespace skkk {
	static void progress(const char *tagPrefixFmt, const std::string &tag,
	                     uint64_t totalSize, uint64_t index, int perPrint,
	                     bool hasEnter) {
		const char *msg = tag.c_str();

		if (index % perPrint == 0 || index == totalSize) {
			uint32_t percentage = 0;
			percentage = std::floor(static_cast<float>(index) / static_cast<float>(totalSize) * 100.0F);
			printf(tagPrefixFmt, msg, percentage);
			if (hasEnter && percentage == 100) [[unlikely]] {
				printf("\n");
			}
		}
	}

	static void progressMT(const char *tagPrefix, const std::string &tag,
	                       uint64_t totalSize, const std::atomic_int &progress,
	                       bool hasEnter) {
		auto *msg = tag.c_str();
		uint32_t currProgress = 0, previousPercentage = 0, percentage = 0;
		do {
			if (currProgress != progress) {
				percentage = std::floor(static_cast<float>(progress) / static_cast<float>(totalSize) * 100.0F);
				if (percentage > previousPercentage) {
					printf(tagPrefix, msg, percentage);
					if (hasEnter && percentage == 100) [[unlikely]] {
						printf("\n");
					}
					previousPercentage = percentage;
				}
				currProgress = progress;
				sleep(0);
			}
			sleep(0);
		} while (currProgress != totalSize);
	}
}

#endif //PAYLOAD_EXTRACT_LOGPROGRESS_H
