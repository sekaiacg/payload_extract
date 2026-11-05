#include <charconv>
#include <cinttypes>
#include <string>

#include "files/io.h"
#include "payload/PayloadInfo.h"
#include "payload/Utils.h"

namespace skkk {
	PayloadInfo::PayloadInfo(const ExtractConfig &config)
		: config(config) {
		this->path = config.getPayloadPath();
	}

	PayloadInfo::~PayloadInfo() {
		IO::closeFd(payloadFd);
	}

	const ExtractConfig &PayloadInfo::getConfig() const {
		return config;
	}

	const std::string &PayloadInfo::getPath() const {
		return path;
	}

	int PayloadInfo::getPayloadFd() const {
		return payloadFd;
	}

	uint64_t PayloadInfo::getPayloadOffset() const {
		return payloadOffset;
	}

	bool PayloadInfo::initPayloadFile() {
		payloadFd = IO::openFileRD(path);
		if (payloadFd < 0) {
			LOGCE("Failed to open({}).\n", path);
			return false;
		}
		inputFileSize = IO::getFileSize(path);
		if (inputFileSize <= 0) {
			LOGCE("The payload file cannot be empty.\n");
			return false;
		}
		return true;
	}

	bool PayloadInfo::getPayloadData(uint8_t *data, uint64_t pos, uint64_t len) const {
		return IO::blobRead(payloadFd, data, pos, len) == 0;
	}

	static bool parseInfo(const std::string &tmp, uint64_t &payloadOffset, uint64_t &payloadSize) {
		size_t first_colon = tmp.find(':');
		if (first_colon == std::string_view::npos) return false;
		const char *start_ptr = tmp.data() + first_colon + 1;
		const char *global_end = tmp.data() + tmp.size();
		auto [ptr1, ec1] = std::from_chars(start_ptr, global_end, payloadOffset);
		if (ec1 != std::errc{} || ptr1 == global_end || *ptr1 != ':') {
			return false;
		}
		const char *size_start_ptr = ptr1 + 1;
		auto [ptr2, ec2] = std::from_chars(size_start_ptr, global_end, payloadSize);
		if (ec2 != std::errc{}) {
			return false;
		}
		return ptr2 > size_start_ptr;
	}

	bool PayloadInfo::parsePayloadMetadataFile(const std::string &fileContent) {
		size_t startPos = fileContent.find(OTA_PROP_FILES_PREFIX);
		if (startPos != std::string::npos) {
			size_t endPos = fileContent.find_first_of('\n', startPos + 1);
			if (endPos != std::string::npos) {
				std::string findStr = {
					fileContent, startPos + OTA_PROP_FILES_PREFIX.size(),
					endPos - OTA_PROP_FILES_PREFIX.size()
				};
				strTrim(findStr);
				std::vector<std::string> list;
				splitString(list, findStr, ",", true);
				for (const auto &tmp: list) {
					if (tmp.starts_with(PAYLOAD_FILENAME)) {
						return parseInfo(tmp, payloadOffset, payloadSize) && payloadOffset > 0 && payloadSize > 0;
					}
				}
			}
		}
		return false;
	}

	bool PayloadInfo::initPayloadOffsetByFastParseZip(const uint8_t *data, uint64_t dataSize) {
		const auto *zlh = reinterpret_cast<const ZipLocalHeader *>(data);
		const auto filenameSize = zlh->filenameLength;
		const auto extraFieldSize = zlh->extraFieldLength;
		if (zlh->compressionMethod == 0) {
			const std::string &filename = {reinterpret_cast<const char *>(data) + ZLH_SIZE, 0, filenameSize};
			if (zlh->compressedSize == 0xFFFFFFFFL || zlh->uncompressedSize == 0xFFFFFFFFL) {
				uint64_t extraFieldOffset = ZLH_SIZE + filenameSize;
				auto eInfo = reinterpret_cast<const Zip64ExtendedInfo *>(data + extraFieldOffset);
				if (filename == PAYLOAD_FILENAME) {
					payloadOffset = extraFieldOffset + zlh->extraFieldLength;
					payloadSize = eInfo->uncompressedSize;
				}
				return payloadOffset > 0 && payloadSize > 0;
			}
			if (filename == METADATA_FILENAME) {
				const std::string fileContent = {
					reinterpret_cast<const char *>(data) + ZLH_SIZE + filenameSize + extraFieldSize,
					dataSize
				};
				return parsePayloadMetadataFile(fileContent);
			}
		}
		return false;
	}

	bool PayloadInfo::initPayloadOffsetByParseZip() {
		if (ZipParser zip{payloadFd, inputFileSize}; zip.parse()) {
			if (const auto it = std::ranges::find(zip.files, METADATA_FILENAME, &ZipFileItem::name);
				it != zipFiles.end()) {
				uint64_t fileSize = it->uncompressedSize;
				Buffer<uint8_t> dataBuffer{fileSize};
				auto *data = dataBuffer.get();
				if (IO::blobRead(payloadFd, data, it->localHeaderOffset, fileSize) == 0) {
					return initPayloadOffsetByFastParseZip(data, fileSize);
				}
			}
		}
		return false;
	}

	bool PayloadInfo::handleOffset() {
		if (inputFileSize >= HEADER_DATA_SIZE) {
			uint8_t data[HEADER_DATA_SIZE] = {};
			if (getPayloadData(data, 0, HEADER_DATA_SIZE)) {
				if (memcmp(data, ZIP_LOCAL_FILE_HEADER_MAGIC,
				           ZIP_LOCAL_FILE_HEADER_SIZE) == 0) {
					if (initPayloadOffsetByFastParseZip(data, HEADER_DATA_SIZE)) {
						return true;
					}
					if (initPayloadOffsetByParseZip()) {
						return true;
					}
				} else if (memcmp(data, PAYLOAD_MAGIC, PAYLOAD_MAGIC_SIZE) == 0) {
					return true;
				}
			}
		}
		LOGCE("ERROR: payload.bin not found!");
		return false;
	}

	bool PayloadInfo::parseHeader() {
		uint8_t data[kMaxPayloadHeaderSize] = {};
		if (getPayloadData(data, payloadOffset, kMaxPayloadHeaderSize)) {
			return pHeader.parseHeader(data);
		}
		return false;
	}


	bool PayloadInfo::readManifestData() {
		auto &inPayloadOffset = pHeader.inPayloadOffset;
		auto &manifest = pHeader.manifest;
		const auto manifestSize = pHeader.manifestSize;
		manifest.reserve(manifestSize);
		if (getPayloadData(manifest.get(), payloadOffset + inPayloadOffset, manifestSize)) {
			inPayloadOffset += manifestSize;
			return true;
		}
		return false;
	}


	bool PayloadInfo::readMetadataSignatureMessage() {
		// Skip manifest signature message
		pHeader.inPayloadOffset += pHeader.metadataSignatureSize;
		return true;
	}

	bool PayloadInfo::readHeaderData() {
		if (!readManifestData()) goto out;

		if (pHeader.metadataSignatureSize > 0) {
			if (!readMetadataSignatureMessage()) goto out;
		}
		return true;
	out:
		LOGCE("Failed to read header data");
		return false;
	}

	bool PayloadInfo::parseManifestData() {
		if (manifest->ParseFromArray(pHeader.manifest.get(), pHeader.manifestSize)) {
			pHeader.blockSize = manifest->block_size();
			return true;
		}
		LOGCE("failed to parse manifest");
		return false;
	}

	bool PayloadInfo::parsePartitionInfo() {
		const auto partitionsSize = manifest->partitions_size();
		const auto minorVersion = manifest->minor_version();
		const uint64_t offset = payloadOffset + pHeader.inPayloadOffset;
		const auto blockSize = pHeader.blockSize;
		const auto &outConfig = config.getOutConfig();

		pHeader.partitionSize = partitionsSize;
		pHeader.minorVersion = minorVersion;
		pHeader.securityPatchLevel = manifest->security_patch_level();

		for (const PartitionUpdate &pu: manifest->partitions()) {
			const auto &opi = pu.old_partition_info();
			const auto &npi = pu.new_partition_info();
			const auto &partName = pu.partition_name();

			const auto &name = outConfig.find(partName);
			std::string outFilePath{
				name != outConfig.end() ? name->second : config.getOutDir() + "/" + partName + ".img"
			};

			auto &partInfo = partitionInfoMap.emplace(std::piecewise_construct, std::forward_as_tuple(partName),
			                                          std::forward_as_tuple(partName, npi.size(),
			                                                                outFilePath, blockSize,
			                                                                opi.hash(), opi.size(),
			                                                                npi.hash(), npi.size())).first->second;
			if (config.isIncremental) {
				partInfo.oldFilePath = config.getOldDir() + "/" + partName + ".img";
			}
			partInfo.outErrorPath = config.getOutDir() + "/" + partName + "_err.txt";

			if (pu.has_hash_tree_data_extent()) {
				partInfo.hasHashTreeDataExtent = true;
				auto &htde = pu.hash_tree_data_extent();
				partInfo.hashTreeDataExtent = {blockSize, htde.start_block(), htde.num_blocks()};
				auto &hte = pu.hash_tree_extent();
				partInfo.hashTreeExtent = {blockSize, hte.start_block(), hte.num_blocks()};
				partInfo.hashTreeAlgorithm = pu.hash_tree_algorithm();
				partInfo.hashTreeSalt = pu.hash_tree_salt();
				partInfo.hashTreeSaltSize = partInfo.hashTreeSalt.size();
			}

			if (pu.has_fec_data_extent()) {
				partInfo.hasFecDataExtent = true;
				auto &fde = pu.fec_data_extent();
				partInfo.fecDataExtent = {blockSize, fde.start_block(), fde.num_blocks()};
				auto &fe = pu.fec_extent();
				partInfo.fecExtent = {blockSize, fe.start_block(), fe.num_blocks()};
				partInfo.fecRoots = pu.fec_roots();
			}

			auto &operations = partInfo.operations;
			operations.reserve(static_cast<uint32_t>(pu.operations_size() * 1.5));
			for (const auto &iop: pu.operations()) {
				const auto dataOffset = iop.data_offset() + offset;
				auto &fop = operations.emplace_back(partName, iop.type(), blockSize,
				                                    dataOffset, iop.data_length(), iop.src_length(),
				                                    iop.dst_length(), iop.src_sha256_hash(), iop.data_sha256_hash());
				auto &srcs = fop.srcExtents;
				auto &dsts = fop.dstExtents;
				for (auto &src: iop.src_extents()) {
					auto &s = srcs.emplace_back(blockSize, src.start_block(), src.num_blocks());
					fop.srcTotalLength += s.dataLength;
				}
				for (auto &dst: iop.dst_extents()) {
					auto &d = dsts.emplace_back(blockSize, dst.start_block(), dst.num_blocks());
					fop.dstTotalLength += d.dataLength;
				}
			}
		}

		LOGCD("Partition size: {}", partitionsSize);
		LOGCD("Minor version: {}", manifest->minor_version());
		LOGCD("Security patch level: {}", manifest->security_patch_level());
		return partitionsSize == partitionInfoMap.size();
	}

	void PayloadInfo::freeManifest() {
		manifest = nullptr;
	}

	void PayloadInfo::printInfo() {
		constexpr uint32_t widthSecond = 4;
		std::println("PayloadInfo:");
		std::println("{:{}}PartitionSize: {}", "", widthSecond, partitionInfoMap.size());
		std::println("{:{}}MinorVersion: {}", "", widthSecond, pHeader.minorVersion);
		std::println("{:{}}SecurityPatchLevel: {}", "", widthSecond, pHeader.securityPatchLevel);
	}

	bool PayloadInfo::initPayloadInfo() {
		if (!initPayloadFile()) goto out;
		if (!handleOffset()) goto out;
		if (!parseHeader()) goto out;
		if (!readHeaderData()) goto out;
		if (!parseManifestData()) goto out;
		if (!dynamicPartitionMetadata.parseDynamicPartitionMetadata(*manifest)) goto out;
		if (!parsePartitionInfo()) goto out;
		freeManifest();
		return true;
	out:
		LOGCE("Failed to initialize payload info");
		return false;
	}
}
