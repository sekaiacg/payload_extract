#include <string>

#include "payload/PartitionInfo.h"
#include "payload/Utils.h"
#include "payload/verify/VerifyInfo.h"

#include "ecc.h"

namespace skkk {
	Level::Level(uint64_t targetSize, uint32_t blockSize) {
		blockCount = targetSize / blockSize;
		realHashSize = blockCount * SHA256_DIGEST_SIZE;
		totalHashSize = alignUp(realHashSize, blockSize);
		paddingSize = totalHashSize - realHashSize;
		data.resize(totalHashSize, 0);
		hashData = data.data();
	}

	VerifyInfo::VerifyInfo(const PartitionInfo &partInfo) {
		blockSize = partInfo.blockSize;
		name = partInfo.name;
		outFilePath = partInfo.outFilePath;
		hasHashTreeDataExtent = partInfo.hasHashTreeDataExtent;
		hashTreeDataExtentOffset = partInfo.hashTreeDataExtent.dataOffset;
		hashTreeDataExtentSize = partInfo.hashTreeDataExtent.dataLength;
		hashTreeDataOffset = partInfo.hashTreeExtent.dataOffset;
		hashTreeDataSize = partInfo.hashTreeExtent.dataLength;
		hashTreeSalt = partInfo.hashTreeSalt;
		hasFecDataExtent = partInfo.hasFecDataExtent;
		fecDataExtentOffset = partInfo.fecDataExtent.dataOffset;
		fecDataExtentSize = partInfo.fecDataExtent.dataLength;
		fecDataOffset = partInfo.fecExtent.dataOffset;
		fecDataSize = partInfo.fecExtent.dataLength;
		fecRoots = partInfo.fecRoots;
		fecRsn = FEC_RSM - fecRoots;
		fecRounds = divRoundUp(fecDataExtentSize / blockSize, fecRsn);
		fecData.resize(fecDataSize, 0);
	}

	bool VerifyInfo::checkCalcHashTreeSuccessful() const {
		isCalcHashTreeSuccessful = *hashTreeExcSize == 0 &&
		                           *hashTreeProgress == hashTreeTotalProgress;
		return isCalcHashTreeSuccessful;
	}

	bool VerifyInfo::checkCalcFecSuccessful() const {
		isCalcFecSuccessful = *fecExcSize == 0 &&
		                      *fecProgress == fecRounds;
		return isCalcFecSuccessful;
	}

	void VerifyInfo::resetStatus() const {
		*hashTreeProgress = 0;
		*hashTreeExcSize = 0;
		isCalcHashTreeSuccessful = false;
		*fecProgress = 0;
		*fecExcSize = 0;
		isCalcFecSuccessful = false;
	}
}
