#include <cstdio>
#include <getopt.h>
#include <iostream>
#include <string>
#include <sys/time.h>

#include <payload/io.h>
#include <payload/LogBase.h>
#include "payload/PayloadParse.h"
#include "payload/Utils.h"
#include "ExtractOperation.h"
#include "payload/ZipParse.h"

using namespace skkk;

static void usage(const ExtractOperation &eo) {
	char buf[1536] = {};
	snprintf(buf, 1536,
			 BROWN "usage: [options]" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-h, --help" COLOR_NONE "           " BROWN "Display this help and exit" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-i, --input=[FILE]" COLOR_NONE "   " BROWN "Input file" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-t" COLOR_NONE "                   " BROWN "Input path type: [bin,url], default: bin(zip)" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-p" COLOR_NONE "                   " BROWN "Print all info" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-P, --print=X" COLOR_NONE "        " BROWN "Print the specified targets: [boot,odm,...]" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-x" COLOR_NONE "                   " BROWN "Extract all items" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-X, --extract=X" COLOR_NONE "      " BROWN "Extract the specified targets: [boot,odm,...]" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-e" COLOR_NONE "                   " BROWN "Exclude mode, exclude specific targets" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-s" COLOR_NONE "                   " BROWN "Silent mode, Don't show progress" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-T#" COLOR_NONE "                  " BROWN "[" GREEN2_BOLD "1-%u" COLOR_NONE BROWN "] Use # threads, default: -T0, is " GREEN2_BOLD "%u" COLOR_NONE COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-k" COLOR_NONE "                   " BROWN "Skip SSL verification" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-o, --outdir=X" COLOR_NONE "       " BROWN "Output dir" COLOR_NONE "\n"
			 "  " GREEN2_BOLD "-V, --version" COLOR_NONE "        " BROWN "Print the version info" COLOR_NONE "\n",
			 eo.limitHardwareConcurrency,
			 eo.hardwareConcurrency
	);
	std::cerr << buf << std::endl;
}

#ifndef PAYLOAD_EXTRACT_VERSION
#define PAYLOAD_EXTRACT_VERSION "v0.0.0"
#endif

static inline void print_version() {
	printf("  " BROWN "payload_extract:" COLOR_NONE "     " RED2_BOLD PAYLOAD_EXTRACT_VERSION COLOR_NONE "\n");
	printf("  " BROWN "author:" COLOR_NONE "              " RED2_BOLD "skkk" COLOR_NONE "\n");
}

static struct option argOptions[] = {
	{"help", no_argument, nullptr, 'h'},
	{"version", no_argument, nullptr, 'V'},
	{"input", required_argument, nullptr, 'i'},
	{"outdir", required_argument, nullptr, 'o'},
	{"print", required_argument, nullptr, 'P'},
	{"extract", required_argument, nullptr, 'X'},
	{nullptr, no_argument, nullptr, 0},
};

static int parseAndCheckExtractCfg(const int argc, char **argv, ExtractOperation &eo) {
	int opt;
	int ret = RET_EXTRACT_CONFIG_FAIL;
	bool enterParseOpt = false;
	while ((opt = getopt_long(argc, argv, "ehi:ko:pst:xP:T:VX:", argOptions, nullptr)) != -1) {
		enterParseOpt = true;
		switch (opt) {
			case 'h':
				usage(eo);
				goto exit;
			case 'V':
				print_version();
				goto exit;
				if (optarg) {
					eo.setFilePath(optarg);
				}
				LOGCD("path=%s", eo.getFilePath().c_str());
				break;
			case 'i':
				if (optarg) {
					eo.setFilePath(optarg);
				}
				LOGCD("path=%s", eo.getFilePath().c_str());
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
				if (optarg) eo.targetNames = optarg;
				LOGCD("isPrintTarget=%d targetPath=%s", eo.isPrintTarget, eo.targetNames.c_str());
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
				if (optarg) eo.targetNames = optarg;
				LOGCD("isExtractTarget=%d targetName=%s", eo.isExtractTarget, eo.targetNames.c_str());
				break;
			case 'c':
				eo.isExtractTargetConfig = true;
				if (optarg) eo.targetConfigPath = optarg;
				LOGCD("targetConfig=%s", eo.targetConfigPath.c_str());
				break;
			case 'e':
				eo.excludeTargets = true;
				LOGCD("exclude targets=%d", eo.excludeTargets);
				break;
			case 't':
				if (optarg) {
					std::string type(optarg);
					if (startsWithIgnoreCase(type, "BIN") || startsWithIgnoreCase(type, "ZIP")) {
						eo.payloadType = PAYLOAD_TYPE_BIN;
					} else if (startsWithIgnoreCase(type, "URL")) {
						eo.payloadType = PAYLOAD_TYPE_URL;
					}
				}
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
			default:
				usage(eo);
				print_version();
				goto exit;
		}
	}

	if (enterParseOpt) {
		if (eo.payloadType != PAYLOAD_TYPE_URL) {
			bool err = !eo.getFilePath().empty() && fileExists(eo.getFilePath());
			if (!err) {
				LOGCE("payload file '%s' does not exist", eo.getFilePath().c_str());
				goto exit;
			}
		}
		ret = !eo.initOutDir();
		if (!ret) {
			goto exit;
		}
		LOGCD("outDir=%s", eo.getOutDir().c_str());

		if (eo.threadNum > eo.limitHardwareConcurrency) {
			ret = RET_EXTRACT_THREAD_NUM_ERROR;
			LOGCE("Threads min: 1 , max: %u", eo.limitHardwareConcurrency);
			goto exit;
		} else if (eo.threadNum == 0) {
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


static void printOperationTime(struct timeval *start, struct timeval *end) {
	LOGCI(GREEN2_BOLD "The operation took: " COLOR_NONE RED2 "%.3f" COLOR_NONE "%s",
	      (end->tv_sec - start->tv_sec) + (float) (end->tv_usec - start->tv_usec) / 1000000,
	      GREEN2_BOLD " second(s)." COLOR_NONE
	);
}

#if defined(_WIN32)
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
	struct timeval start = {}, end = {};

#if defined(_WIN32)
	handleWinTerminal();
	enableWinTerminalColor(STD_OUTPUT_HANDLE);
	enableWinTerminalColor(STD_ERROR_HANDLE);
#endif

	setbuf(stdout, nullptr);
	setbuf(stderr, nullptr);

	// Start time
	gettimeofday(&start, nullptr);

	PayloadParse pp;
	PayloadInfo *pi = nullptr;
	// Initialize extract coPnfig
	ExtractOperation eo = {};
	int err = parseAndCheckExtractCfg(argc, argv, eo);
	if (err != RET_EXTRACT_CONFIG_DONE) {
		ret = err;
		goto exit;
	}

	pi = pp.parsePayloadInfo(eo.getFilePath(), eo.payloadType,
	                         eo.sslVerification);
	if (pi == nullptr) {
		ret = RET_EXTRACT_INIT_FAIL;
		goto exit;
	} else {
		eo.initPayloadInfo(*pi);
	}

	if (eo.isPrintTarget || eo.isExtractTarget || eo.isExtractTargetConfig) {
		err = eo.initFileNodeByTarget(*pi);
	} else if (eo.isPrintAll || eo.isExtractAll) {
		err = eo.initAllFileNode(*pi);
	}

	if (err) {
		ret = RET_EXTRACT_INIT_NODE_FAIL;
		goto exit_dev_close;
	}

	if (eo.isPrintTarget || eo.isPrintAll) {
		eo.printFiles();
		goto exit_dev_close;
	}

	LOGCI(GREEN2_BOLD "Starting..." COLOR_NONE);

	if (eo.isExtractAll || eo.isExtractTarget) {
		err = eo.createExtractOutDir();
		if (err) {
			ret = RET_EXTRACT_CREATE_DIR_FAIL;
			goto exit_dev_close;
		}
		eo.extractFiles();
		goto end;
	}

end:
	// End time
	gettimeofday(&end, nullptr);
	printOperationTime(&start, &end);

exit_dev_close:
	LOGCD("main exit ret=%d", ret);

exit:
	return ret;
}
