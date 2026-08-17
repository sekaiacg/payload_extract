#include <format>
#include <print>
#include <ranges>

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
		for (auto &dp: dynamicPartitions) {
			dp.info = std::format("    name: {}\n    size: {}\n    items: {}",
			                      dp.name, dp.size, dp.partitionNames);
		}
		if (snapshotEnabled) {
			info += std::format("snapshot:\n    snapshot_enabled: {}\n", snapshotEnabled);
			if (hasCompressionFactor) {
				info += std::format("    compression_factor: {}\n", compressionFactor);
			}
			if (hasDisableUblk) {
				info += std::format("    disable_ublk: {}\n", disableUblk);
			}
		}
		if (vabcEnabled) {
			info += std::format("VABC:\n    vabc_enabled: {}\n    vabc_compression_param: {}\n    cow_version: {}\n",
			                    vabcEnabled, vabcCompressionParam, cowVersion);
			if (hasVabcFeatureSet) {
				info += std::format("    vabc_feature_set: {{ threaded: {}, batch_writes: {}}}\n",
				                    vabcFeatureSet.threaded, vabcFeatureSet.batchWrites);
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
				vabcCompressionParam = dpm.vabc_compression_param();
			}
			if (dpm.has_cow_version()) {
				cowVersion = dpm.cow_version();
			}
			if (dpm.has_vabc_feature_set()) {
				hasVabcFeatureSet = true;
				const auto &features = dpm.vabc_feature_set();
				if (features.has_threaded()) {
					vabcFeatureSet.threaded = features.threaded();
				}
				if (features.has_batch_writes()) {
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
