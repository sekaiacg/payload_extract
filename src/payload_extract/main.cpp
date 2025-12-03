#include <cstdio>
#include <getopt.h>
#include <iostream>
#include <string>
#include <sys/time.h>

#include <payload/ExtractConfig.h>
#include <payload/LogBase.h>
#include <payload/PartitionWriter.h>
#include <payload/PayloadParser.h>
#include <payload/Utils.h>
#include <payload/verify/VerifyWriter.h>

#include "ExtractOperation.h"
#include "RemoteUpdater.h"

using namespace skkk;

static void usage(const ExtractOperation &eo) {
	char buf[4096] = {};
	snprintf(buf, sizeof(buf) - 1,
			 BROWN "usage: [options]" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-h, --help" COLOR_NONE "           " BROWN "Display this help and exit" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-i, --input=[PATH]" COLOR_NONE "   " BROWN "File path or URL" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "--incremental=X" COLOR_NONE "      " BROWN "Old directory, Catalog requiring incremental patching" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "--verify-update" COLOR_NONE "      " BROWN "  In the incremental mode, The dm-verify verified file" COLOR_NONE "\n"
			 "  "             "               "            "      " BROWN "  does not contain HASH_TREE and FEC. Only files that" COLOR_NONE "\n"
			 "  "             "               "            "      " BROWN "  have successfully updated this information can undergo" COLOR_NONE "\n"
			 "  "             "               "            "      " BROWN "  SHA256 verification." COLOR_NONE "\n"
			 "  " GREEN2_BOLD "--verify-update=X" COLOR_NONE "    " BROWN "  Only Verify and update the specified targets: [boot,odm,...]" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-p" COLOR_NONE "                   " BROWN "Print all info" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-P, --print=X" COLOR_NONE "        " BROWN "Print the specified targets: [boot,odm,...]" COLOR_NONE "\n"
	         "  " GREEN2_BOLD "-x" COLOR_NONE "                   " BROWN "Extract all items" COLOR_NONE "\n"
	         "  " GREEN2_BOLD "-X, --extract=X" COLOR_NONE "      " BROWN "Extract the specified targets: [boot,odm,...]" COLOR_NONE "\n"
	         "  " GREEN2_BOLD "-e" COLOR_NONE "                   " BROWN "Exclude mode, exclude specific targets" COLOR_NONE "\n"
	         "  " GREEN2_BOLD "-s" COLOR_NONE "                   " BROWN "Silent mode, Don't show progress" COLOR_NONE "\n"
	         "  " GREEN2_BOLD "-T#" COLOR_NONE "                  " BROWN "[" GREEN2_BOLD "1-%u" COLOR_NONE BROWN "] Use # threads, default: -T0, is " GREEN2_BOLD "%u" COLOR_NONE COLOR_NONE "\n"
	         "  " GREEN2_BOLD "-k" COLOR_NONE "                   " BROWN "Skip SSL verification" COLOR_NONE "\n"
	         "  " GREEN2_BOLD "-o, --outdir=X" COLOR_NONE "       " BROWN "Output dir" COLOR_NONE "\n"
	         "  " GREEN2_BOLD "--out-config=X" COLOR_NONE "       " BROWN "Output config file, One config per line: [boot:/path/to/xxx]" COLOR_NONE "\n"
	         "  " GREEN2_BOLD "-R" COLOR_NONE "                   " BROWN "Modify the URL in the remote config" COLOR_NONE "\n"
	         "  "             "               "            "      " BROWN "  May need to specify the output directory" COLOR_NONE "\n"
	         "  " GREEN2_BOLD "-V, --version" COLOR_NONE "        " BROWN "Print the version info" COLOR_NONE "\n",
	         eo.limitHardwareConcurrency,
	         eo.hardwareConcurrency
	);
	std::cerr << buf << std::endl;
}

#ifndef PAYLOAD_EXTRACT_VERSION
#define PAYLOAD_EXTRACT_VERSION "v0.0.0"
#endif
#ifndef PAYLOAD_EXTRACT_BUILD_TIME
#define PAYLOAD_EXTRACT_BUILD_TIME "-0000000000"
#endif

static void printVersion() {
	printf("  " BROWN "payload_extract:" COLOR_NONE "     " RED2_BOLD PAYLOAD_EXTRACT_VERSION PAYLOAD_EXTRACT_BUILD_TIME COLOR_NONE "\n");
	printf("  " BROWN "author:" COLOR_NONE "              " RED2_BOLD "skkk" COLOR_NONE "\n");
}

static option argOptions[] = {
	{"help", no_argument, nullptr, 'h'},
	{"version", no_argument, nullptr, 'V'},
	{"input", required_argument, nullptr, 'i'},
	{"outdir", required_argument, nullptr, 'o'},
	{"print", required_argument, nullptr, 'P'},
	{"extract", required_argument, nullptr, 'X'},
	{"incremental", required_argument, nullptr, 200},
	{"verify-update", optional_argument, nullptr, 201},
	{"out-config",required_argument, nullptr, 202},
	{nullptr, no_argument, nullptr, 0},
};

static int parseExtractOperation(const int argc, char **argv, ExtractOperation &eo) {
	int opt, ret = RET_EXTRACT_CONFIG_FAIL;
	bool enterCheckOpt = false;
	while ((opt = getopt_long(argc, argv, "ehi:ko:pst:xP:T:VX:R", argOptions, nullptr)) != -1) {
		enterCheckOpt = true;
		switch (opt) {
			case 'h':
				usage(eo);
				goto exit;
			case 'V':
				printVersion();
				goto exit;
			case 'i':
				if (optarg) {
					eo.setPayloadPath(optarg);
				}
				LOGCD("path=%s", eo.getPayloadPath().c_str());
				break;
			case 'k':
				eo.sslVerification = false;
				LOGCD("Skip SSL verification=%d", eo.sslVerification);
				break;
			case 'o':
				if (optarg) {
					eo.setOutDir(optarg);
				}
				LOGCD("outDir=%s", eo.getOutDir().c_str());
				break;
			case 'p':
				eo.isPrintAll = true;
				LOGCD("isPrintAllNode=%d", eo.isPrintAll);
				break;
			case 'P':
				eo.isPrintTarget = true;
				if (optarg) eo.setTargetName(optarg);
				LOGCD("isPrintTarget=%d targetPath=%s", eo.isPrintTarget, eo.getTargetName().c_str());
				break;
			case 's':
				eo.isSilent = true;
				LOGCD("isSilent=%d", eo.isSilent);
				break;
			case 'x':
				eo.isExtractAll = true;
				LOGCD("isExtractAll=%d", eo.isExtractAll);
				break;
			case 'X':
				eo.isExtractTarget = true;
				if (optarg) eo.setTargetName(optarg);
				LOGCD("isExtractTarget=%d targetName=%s", eo.isExtractTarget, eo.getTargetName().c_str());
				break;
			case 'e':
				eo.isExcludeMode = true;
				LOGCD("isExcludeMode=%d", eo.isExcludeMode);
				break;
			case 'T':
				if (optarg) {
					char *endPtr;
					uint64_t n = strtoull(optarg, &endPtr, 0);
					if (*endPtr == '\0') {
						eo.threadNum = n;
					}
				}
				break;
			case 'R':
				eo.remoteUpdate = true;
				LOGCD("remoteUpdate=%d", eo.remoteUpdate);
				break;
			case 200:
				eo.isIncremental = true;
				if (optarg) {
					eo.isIncremental = true;
					eo.setOldDir(optarg);
				}
				LOGCD("isIncremental=%d oldDir=%s", eo.isIncremental, eo.getOldDir().c_str());
				break;
			case 201:
				eo.isVerifyUpdate = true;
				if (eo.getTargetName().empty()) {
					if (optarg) {
						eo.setTargetName(optarg);
					}
				}
				LOGCD("isVerifyUpdate=%d oldDir=%s", eo.isVerifyUpdate, eo.getOldDir().c_str());
				break;
			case 202:
				if (optarg) {
					eo.setOutConfigPath(optarg);
				}
				LOGCD("outConfigPath=%s", eo.getOutConfigPath().c_str());
				break;
			default:
				usage(eo);
				printVersion();
				goto exit;
		}
	}

	if (enterCheckOpt) {
		if (eo.getPayloadPath().empty()) {
			ret = RET_EXTRACT_OPEN_FILE;
			goto exit;
		}

		eo.handleUrl();
		LOGCD("isUrl=%d", eo.isUrl);

		eo.initHttpDownload();
		LOGCD("httpDownload=%d", eo.httpDownload != nullptr);

		if (eo.payloadType != PAYLOAD_TYPE_URL) {
			if (!fileExists(eo.getPayloadPath())) {
				LOGCE("payload file '%s' does not exist", eo.getPayloadPath().c_str());
				ret = RET_EXTRACT_OPEN_FILE;
				goto exit;
			}
		}

		if (eo.isIncremental) {
			ret = eo.initOldDir();
			if (ret) goto exit;
		}

		ret = eo.initOutDir();
		if (ret) goto exit;

		if (!eo.getOutConfigPath().empty()) {
			ret = eo.initOutConfig();
			if (ret) goto exit;
		}

		if (!eo.getTargetName().empty()) {
			ret = eo.initTargetNames();
			if (ret) goto exit;
		}

		if (eo.threadNum > eo.limitHardwareConcurrency) {
			ret = RET_EXTRACT_THREAD_NUM_ERROR;
			LOGCE("Threads min: 1 , max: %u", eo.limitHardwareConcurrency);
			goto exit;
		}
		if (eo.threadNum == 0) {
			eo.threadNum = eo.hardwareConcurrency;
		}
		LOGCD("Threads num=%u", eo.threadNum);
		ret = RET_EXTRACT_CONFIG_DONE;
	} else {
		usage(eo);
	}
exit:
	return ret;
}

static void printOperationTime(const timeval *start, const timeval *end) {
	LOGCI(GREEN2_BOLD "The operation took: " COLOR_NONE RED2 "%.3f" COLOR_NONE "%s",
	      (end->tv_sec - start->tv_sec) + static_cast<float>(end->tv_usec - start->tv_usec) / 1000000,
	      GREEN2_BOLD " second(s)." COLOR_NONE
	);
}

#if defined(_WIN32)
#include <windows.h>

static void handleWinTerminal() {
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
	if (hStdin != INVALID_HANDLE_VALUE) {
		DWORD mode;
		if (GetConsoleMode(hStdin, &mode)) {
			mode &= ~ENABLE_QUICK_EDIT_MODE;
			mode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			mode &= ~ENABLE_MOUSE_INPUT;
			SetConsoleMode(hStdin, mode);
		}
	}
}

static void enableWinTerminalColor(DWORD handle) {
	HANDLE nHandle = GetStdHandle(handle);
	if (nHandle != INVALID_HANDLE_VALUE) {
		DWORD mode;
		if (GetConsoleMode(nHandle, &mode)) {
			mode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			SetConsoleMode(nHandle, mode);
		}
	}
}
#endif

int main(const int argc, char *argv[]) {
	int ret = RET_EXTRACT_DONE;
	bool err = false;
	timeval start{}, end{};

#if defined(_WIN32)
	handleWinTerminal();
	enableWinTerminalColor(STD_OUTPUT_HANDLE);
	enableWinTerminalColor(STD_ERROR_HANDLE);
#endif

	setbuf(stdout, nullptr);
	setbuf(stderr, nullptr);

	// Start time
	gettimeofday(&start, nullptr);

	// Config
	ExtractOperation eo;
	PayloadParser payloadParser;
	std::shared_ptr<RemoteUpdater> ru;
	std::shared_ptr<PartitionWriter> pw;
	std::shared_ptr<VerifyWriter> vw;
	if (parseExtractOperation(argc, argv, eo) != RET_EXTRACT_CONFIG_DONE) {
		ret = RET_EXTRACT_INIT_FAIL;
		goto exit;
	}

	// RemoteUpdater
	ru = std::make_shared<RemoteUpdater>(eo);
	if (eo.remoteUpdate) {
		if (!ru->initRemoteUpdate(false)) {
			ret = RET_EXTRACT_INIT_FAIL;
			goto exit;
		}
		ru->notifyRemoteUpdate();
		goto exit;
	}

	// Parse payload.bin
	if (!payloadParser.parse(eo)) {
		ret = RET_EXTRACT_INIT_FAIL;
		goto exit;
	}

	// PartitionWriter
	pw = payloadParser.getPartitionWriter();

	if (!eo.getTargetName().empty()) {
		err = pw->initPartitionsByTarget();
	} else if (eo.isPrintAll || eo.isExtractAll) {
		err = pw->initPartitions();
	}
	if (!err) {
		ret = RET_EXTRACT_INIT_PART_FAIL;
		LOGCE("Cannot find the image file to be extracted!");
		goto exit;
	}

	// VerifyWriter
	vw = pw->getVerifyWriter();
	vw->initHashTreeLevel();

	if (eo.isPrintTarget || eo.isPrintAll) {
		pw->printPartitionsInfo();
		goto exit;
	}

	LOGCI(GREEN2_BOLD "Starting..." COLOR_NONE);

	if (eo.isExtractAll || eo.isExtractTarget) {
		err = eo.createExtractOutDir();
		if (err) {
			ret = RET_EXTRACT_CREATE_DIR_FAIL;
			goto exit;
		}

		if (eo.isUrl) {
			if (!ru->initRemoteUpdate(true)) {
				ret = RET_EXTRACT_INIT_FAIL;
				goto exit;
			}
			ru->startMonitor();
		}

		pw->extractPartitions();

		if (eo.isIncremental && eo.isVerifyUpdate) {
			vw->updateVerifyData();
		}
		goto end;
	}

	if (eo.isIncremental && eo.isVerifyUpdate) {
		vw->updateVerifyData();
		goto end;
	}

end:
	// End time
	gettimeofday(&end, nullptr);
	printOperationTime(&start, &end);

exit:
	return ret;
}
