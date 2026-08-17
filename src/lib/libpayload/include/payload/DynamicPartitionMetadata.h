#pragma once

#include "update_metadata.pb.h"

namespace skkk {
	using namespace chromeos_update_engine;

	class DynamicPartition {
		public:
			/**
			 * name of the group.
			 */
			std::string name;

			/**
			 * maximum size of the group. The sum of sizes of all partitions in the group
			 * must not exceed the maximum size of the group.
			 */
			uint64_t size = 0;

			/**
			 * a list of partitions that belong to the group
			 */
			std::vector<std::string> partitionNames;

			std::string info;

		public:
			DynamicPartition(const DynamicPartitionGroup &group);
	};

	class VabcFeatureSet {
		public:
			bool threaded = false;
			bool batchWrites = false;
	};

	class DynamicPartitionMetadata {
		public:
			std::vector<DynamicPartition> dynamicPartitions;
			bool snapshotEnabled = false;
			bool vabcEnabled = false;
			std::string vabcCompressionParam;
			uint32_t cowVersion = false;
			bool hasVabcFeatureSet = false;
			VabcFeatureSet vabcFeatureSet;
			bool hasCompressionFactor = false;
			uint64_t compressionFactor = 0;
			bool hasDisableUblk = false;
			bool disableUblk = false;
			std::string info;

		public:
			void initInfo();

			bool parseDynamicPartitionMetadata(const DeltaArchiveManifest &manifest);

			void printInfo() const;
	};
}
