#include <format>

#include "payload/print.hpp"
#include "payload/DynamicPartitionMetadata.h"

namespace skkk {
	DynamicPartition::DynamicPartition(const DynamicPartitionGroup &group) {
		name = group.name();
		if (group.has_size()) {
			size = group.size();
		}
		for (const auto &partitionName: group.partition_names()) {
			partitionNames.emplace_back(partitionName);
		}
	}

	void DynamicPartitionMetadata::initInfo() {
		constexpr uint32_t widthSecond = 4;
		constexpr uint32_t widthThird = widthSecond * 2;
		for (auto &dp: dynamicPartitions) {
			dp.info = std::format("{:{}s}name: {}\n    size: {}\n    items: {}", "", widthSecond,
			                      dp.name, dp.size, dp.partitionNames);
		}
		if (snapshotEnabled) {
			info += std::format("Snapshot:\n");
			info += std::format("{:{}s}snapshot_enabled: {}\n", "", widthSecond, snapshotEnabled);
			if (hasCompressionFactor) {
				info += std::format("{:{}s}compression_factor: {}\n", "", widthSecond, compressionFactor);
			}
			if (hasDisableUblk) {
				info += std::format("{:{}s}disable_ublk: {}\n", "", widthSecond, disableUblk);
			}
		}
		if (vabcEnabled) {
			info += std::format("VABC:\n");
			info += std::format("{:{}s}vabc_enabled: {}\n", "", widthSecond, vabcEnabled);
			if (hasVabcCompressionParam) {
				info += std::format("{:{}s}vabc_compression_param: {}\n", "", widthSecond, vabcCompressionParam);
			}
			if (hasCowVersion) {
				info += std::format("{:{}s}cow_version: {}\n", "", widthSecond, cowVersion);
			}
			if (hasVabcFeatureSet) {
				info += std::format("{:{}s}vabc_feature_set:\n", "", widthSecond);
				if (vabcFeatureSet.hasThreaded) {
					info += std::format("{:{}s}threaded: {}\n", "", widthThird, vabcFeatureSet.threaded);
				}
				if (vabcFeatureSet.hasBatchWrites) {
					info += std::format("{:{}s}batch_writes: {}\n", "", widthThird, vabcFeatureSet.batchWrites);
				}
			}
		}
	}

	bool DynamicPartitionMetadata::parseDynamicPartitionMetadata(const DeltaArchiveManifest &manifest) {
		if (manifest.has_dynamic_partition_metadata()) {
			const auto &dpm = manifest.dynamic_partition_metadata();
			dynamicPartitions.reserve(64);
			for (const auto &group: dpm.groups()) {
				dynamicPartitions.emplace_back(group);
			}
			if (dpm.has_snapshot_enabled()) {
				snapshotEnabled = dpm.snapshot_enabled();
			}
			if (dpm.has_vabc_enabled()) {
				vabcEnabled = dpm.has_vabc_enabled();
			}
			if (dpm.has_vabc_compression_param()) {
				hasVabcCompressionParam = true;
				vabcCompressionParam = dpm.vabc_compression_param();
			}
			if (dpm.has_cow_version()) {
				hasCowVersion = true;
				cowVersion = dpm.cow_version();
			}
			if (dpm.has_vabc_feature_set()) {
				hasVabcFeatureSet = true;
				const auto &features = dpm.vabc_feature_set();
				if (features.has_threaded()) {
					vabcFeatureSet.hasThreaded = true;
					vabcFeatureSet.threaded = features.threaded();
				}
				if (features.has_batch_writes()) {
					vabcFeatureSet.hasBatchWrites = true;
					vabcFeatureSet.batchWrites = features.batch_writes();
				}
			}
			if (dpm.has_compression_factor()) {
				hasCompressionFactor = true;
				compressionFactor = dpm.compression_factor();
			}
			if (dpm.has_disable_ublk()) {
				hasDisableUblk = true;
				disableUblk = dpm.disable_ublk();
			}
			initInfo();
		}
		return true;
	}

	void DynamicPartitionMetadata::printInfo() const {
		if (!dynamicPartitions.empty()) {
			std::println("DynamicPartition:");
			for (const auto &dp: dynamicPartitions) {
				std::println("{}", dp.info);
			}
		}
		if (!info.empty()) {
			std::println("{}", info);
		}
	}
}
