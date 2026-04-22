#include <cinttypes>
#include <string>

#include "payload/PayloadInfo.h"
#include "payload/Utils.h"
#include "payload/mman/mmap.hpp"

namespace skkk {
	PayloadInfo::PayloadInfo(const ExtractConfig &config)
		: config(config) {
		this->path = config.getPayloadPath();
	}

	PayloadInfo::~PayloadInfo() {
		closePayloadFile();
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
		int ret = mapRdByPath(payloadFd, path, fileData, fileDataSize);
		if (!ret) {
			if (fileDataSize > 0 && fileData) {
				return true;
			}
			LOGCE("failed to mmap({}).\n", path);
			return false;
		}
		LOGCE("failed to open({}).\n", path);
		return false;
	}

	bool PayloadInfo::parsePayloadMetadataFile(const std::string &fileContext) {
		size_t startPos = fileContext.find(OTA_PROP_FILES_PREFIX);
		if (startPos != std::string::npos) {
			size_t endPos = fileContext.find_first_of('\n', startPos + 1);
			if (endPos != std::string::npos) {
				std::string findStr = {
					fileContext, startPos + OTA_PROP_FILES_PREFIX.size(),
					endPos - OTA_PROP_FILES_PREFIX.size()
				};
				strTrim(findStr);
				std::vector<std::string> list;
				splitString(list, findStr, ",", true);
				for (const auto &tmp: list) {
					if (tmp.starts_with("payload_metadata.bin")) {
						std::stringstream ss(tmp);
						std::string offsetStr;
						std::string sizeStr;
						std::getline(ss, offsetStr, ':');
						std::getline(ss, offsetStr, ':');
						std::getline(ss, sizeStr);
						payloadOffset = std::stoll(offsetStr);
						payloadMetadataSize = std::stoll(sizeStr);
						return payloadOffset > 0 && payloadMetadataSize > 0;
					}
				}
			}
		}
		return false;
	}

	bool PayloadInfo::initPayloadOffsetByFastParseZip(const uint8_t *data, uint64_t dataSize) {
		const auto *zlh = reinterpret_cast<const ZipLocalHeader *>(data);
		const auto filenameSize = zlh->filenameLength;
		const auto fileSize = zlh->uncompressedSize;
		const auto extraFieldSize = zlh->extraFieldLength;
		if (zlh->compressionMethod == 0) {
			constexpr uint64_t headerSize = sizeof(ZipLocalHeader);
			const uint64_t totalSize = headerSize + filenameSize + extraFieldSize + fileSize;
			const std::string &filename = {reinterpret_cast<const char *>(data) + headerSize, 0, filenameSize};
			if (filename == METADATA_FILENAME && totalSize <= dataSize) {
				const std::string fileContext = {
					reinterpret_cast<const char *>(data) + headerSize + filenameSize + extraFieldSize,
					fileSize
				};
				return parsePayloadMetadataFile(fileContext);
			}
		}
		return false;
	}

	bool PayloadInfo::initPayloadOffsetByParseZip() {
		if (ZipParser zip{fileData, fileDataSize}; zip.parse()) {
			if (const auto it = std::ranges::find(zip.files, METADATA_FILENAME, &ZipFileItem::name);
				it != zipFiles.end()) {
				return initPayloadOffsetByFastParseZip(fileData + it->localHeaderOffset, fileDataSize);
			}
		}
		return false;
	}

	bool PayloadInfo::handleOffset() {
		if (fileDataSize >= HEADER_DATA_SIZE) {
			uint8_t data[HEADER_DATA_SIZE] = {};
			if (memcpy(data, fileData, HEADER_DATA_SIZE) == data) {
				if (memcmp(fileData, ZIP_LOCAL_FILE_HEADER_MAGIC, ZIP_LOCAL_FILE_HEADER_SIZE) == 0) {
					if (initPayloadOffsetByFastParseZip(data, HEADER_DATA_SIZE)) {
						return true;
					}
					if (initPayloadOffsetByParseZip()) {
						return true;
					}
				} else if (memcmp(fileData, PAYLOAD_MAGIC, PAYLOAD_MAGIC_SIZE) == 0) {
					return true;
				}
			}
		}
		LOGCE("ZIP: payload.bin not found!");
		return true;
	}

	bool PayloadInfo::parseHeader() {
		return pHeader.parseHeader(payloadMetadata ? payloadMetadata.get() : fileData + payloadOffset);
	}

	bool PayloadInfo::readManifestData() {
		auto &inPayloadOffset = pHeader.inPayloadOffset;
		const auto manifestSize = pHeader.manifestSize;
		auto &mf = pHeader.manifest;
		mf.reserve(manifestSize);
		const uint8_t *data = payloadMetadata
			                      ? payloadMetadata.get() + inPayloadOffset
			                      : fileData + payloadOffset + inPayloadOffset;
		if (memcpy(mf.get(), data, manifestSize)) {
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

		if (pHeader.isVersion2()) {
			if (!readMetadataSignatureMessage()) goto out;
		}
		return true;
	out:
		LOGCE("Failed to read header data");
		return false;
	}

	bool PayloadInfo::parseManifestData() {
		if (manifest.ParseFromArray(pHeader.manifest.get(), pHeader.manifestSize)) {
			pHeader.blockSize = manifest.block_size();
			return true;
		}
		LOGCE("failed to parse manifest");
		return false;
	}

	const uint8_t *PayloadInfo::getPayloadData() const {
		if (fileData) {
			return fileData;
		}
		return nullptr;
	}

	bool PayloadInfo::parsePartitionInfo() {
		const auto partitionsSize = manifest.partitions_size();
		const auto minorVersion = manifest.minor_version();
		const uint64_t offset = payloadOffset + pHeader.inPayloadOffset;
		const auto blockSize = pHeader.blockSize;
		const auto &outConfig = config.getOutConfig();

		pHeader.partitionSize = partitionsSize;
		pHeader.minorVersion = minorVersion;
		pHeader.securityPatchLevel = manifest.security_patch_level();

		for (const PartitionUpdate &pu: manifest.partitions()) {
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
		LOGCD("Minor version: {}", manifest.minor_version());
		LOGCD("Security patch level: {}", manifest.security_patch_level());
		return partitionsSize == partitionInfoMap.size();
	}

	bool PayloadInfo::initPayloadInfo() {
		if (!initPayloadFile()) goto out;
		if (!handleOffset()) goto out;
		if (!parseHeader()) goto out;
		if (!readHeaderData()) goto out;
		if (!parseManifestData()) goto out;
		if (!parsePartitionInfo()) goto out;
		return true;
	out:
		LOGCE("Failed to initialize payload info");
		return false;
	}

	void PayloadInfo::closePayloadFile() {
		if (!unmap(fileData, fileDataSize)) {
			closeFd(payloadFd);
		}
	}
}
