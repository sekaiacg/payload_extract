#ifndef PAYLOAD_EXTRACT_PAYLOAD_MANIFEST_H
#define PAYLOAD_EXTRACT_PAYLOAD_MANIFEST_H

#include "PayloadFileInfo.h"
#include "update_metadata.pb.h"

namespace skkk {
	using namespace chromeos_update_engine;

	class PayloadManifest {
		public:
			uint64_t payloadOffset = 0;

			DeltaArchiveManifest manifest;

			PayloadFileInfo payloadFileInfo;
	};
}

#endif //PAYLOAD_EXTRACT_PAYLOAD_MANIFEST_H
