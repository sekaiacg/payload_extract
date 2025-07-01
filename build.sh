VERSION="$(cat ./VERSION)"
OUT="./out"
BUILD_DIR="./"

cmake_build()
{
    local TARGET=$1
    local METHOD=$2
    local ABI=$3

    local tls_backend=""
    [[ "${TARGET}" == "Linux" || "${TARGET}" == "Android" ]] && tls_backend="mbedtls"

    if [[ $METHOD == "Ninja" ]]; then
        local BUILD_METHOD="-G Ninja"
        local MAKE_CMD="time -p cmake --build $OUT -j$(nproc) --target protobuf-cpp-full ${tls_backend} payload_extract"
    elif [[ $METHOD == "make" ]]; then
        local MAKE_CMD="time -p make -C $OUT -j$(nproc)"
    fi;

    if [[ $TARGET == "Android" ]]; then
        local ANDROID_PLATFORM=$4
        cmake -S ${BUILD_DIR} -B $OUT ${BUILD_METHOD} \
            -DPAYLOAD_EXTRACT_VERSION="${VERSION}" \
            -DNDK_CCACHE="ccache" \
            -DCMAKE_BUILD_TYPE="Release" \
            -DANDROID_PLATFORM="$ANDROID_PLATFORM" \
            -DANDROID_ABI="$ABI" \
            -DANDROID_STL="c++_static" \
            -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_LATEST_HOME/build/cmake/android.toolchain.cmake" \
            -DANDROID_USE_LEGACY_TOOLCHAIN_FILE="OFF" \
            -DPAYLOAD_EXTRACT_VERSION="${PAYLOAD_EXTRACT_VERSION}" \
            -DCMAKE_C_FLAGS="" \
            -DCMAKE_CXX_FLAGS="" \
            -DENABLE_FULL_LTO="ON"
    elif [[ $TARGET == "Linux" ]]; then
        local LINUX_PLATFORM=$4
        if [[ ${ABI} == "x86_64" ]]; then
            cmake -S ${BUILD_DIR} -B ${OUT} ${BUILD_METHOD} \
                -DPAYLOAD_EXTRACT_VERSION="${VERSION}" \
                -DCMAKE_SYSTEM_NAME="Linux" \
                -DCMAKE_SYSTEM_PROCESSOR="x86_64" \
                -DCMAKE_BUILD_TYPE="Release" \
                -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
                -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
                -DCMAKE_C_COMPILER="clang" \
                -DCMAKE_CXX_COMPILER="clang++" \
                -DCMAKE_C_FLAGS="" \
                -DCMAKE_CXX_FLAGS="" \
                -DENABLE_FULL_LTO="ON"
        elif [[ ${ABI} == "aarch64" ]]; then
            cmake -S ${BUILD_DIR} -B ${OUT} ${BUILD_METHOD} \
                -DPAYLOAD_EXTRACT_VERSION="${VERSION}" \
                -DCMAKE_SYSTEM_NAME="Linux" \
                -DCMAKE_SYSTEM_PROCESSOR="aarch64" \
                -DCMAKE_BUILD_TYPE="Release" \
                -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
                -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
                -DCMAKE_C_COMPILER="${CUSTOM_CLANG_PATH}/bin/clang" \
                -DCMAKE_CXX_COMPILER="${CUSTOM_CLANG_PATH}/bin/clang++" \
                -DCMAKE_SYSROOT="${GCC_AARCH64_TOOLS_PATH}/aarch64-linux-gnu/libc" \
                -DCMAKE_C_COMPILER_TARGET="aarch64-linux-gnu" \
                -DCMAKE_CXX_COMPILER_TARGET="aarch64-linux-gnu" \
                -DCMAKE_C_FLAGS="--gcc-toolchain=${GCC_AARCH64_TOOLS_PATH}" \
                -DCMAKE_CXX_FLAGS="--gcc-toolchain=${GCC_AARCH64_TOOLS_PATH}" \
                -DENABLE_FULL_LTO="ON"
        fi
    elif [[ $TARGET == "Windows" ]]; then
        cmake -S ${BUILD_DIR} -B ${OUT} ${BUILD_METHOD} \
            -DPAYLOAD_EXTRACT_VERSION="${VERSION}" \
            -DCMAKE_BUILD_TYPE="Release" \
            -DMINGW_ABI="$ABI" \
            -DMINGW_SYSROOT="${MINGW_LLVM_PATH}" \
            -DCMAKE_TOOLCHAIN_FILE="$(pwd)/cmake/mingw-llvm.cmake" \
            -DCMAKE_C_COMPILER_LAUNCHER="ccache" \
            -DCMAKE_CXX_COMPILER_LAUNCHER="ccache" \
            -DCMAKE_C_FLAGS="" \
            -DCMAKE_CXX_FLAGS="" \
            -DENABLE_FULL_LTO="ON"
    fi

	${MAKE_CMD}
}

build()
{
    local TARGET=$1
    local ABI=$2
    local PLATFORM=$3

    rm -rf $OUT > /dev/null 2>&1

    local NINJA=`which ninja`
    if [[ -f $NINJA ]]; then
        local METHOD="Ninja"
    else
        local METHOD="make"
    fi

    cmake_build "${TARGET}" "${METHOD}" "${ABI}" "${PLATFORM}"
    local BIN_SUFFIX=""
    [ "${TARGET}" == "Windows" ] && BIN_SUFFIX=".exe"

    local BUILD="$OUT/src/payload_extract"
    local PAYLOAD_EXTRACT_BIN="$BUILD/payload_extract${BIN_SUFFIX}"
    local TARGE_DIR_NAME="payload_extract-${VERSION}-${TARGET}_${ABI}-$(TZ=UTC-8 date +%y%m%d%H%M)"
    local TARGET_DIR_PATH="./target/${TARGET}_${ABI}/${TARGE_DIR_NAME}"

    if [[ -f "$PAYLOAD_EXTRACT_BIN" ]]; then
        echo "复制文件中..."
    [[ ! -d "$TARGET_DIR_PATH" ]] && mkdir -p ${TARGET_DIR_PATH}
        cp -af $PAYLOAD_EXTRACT_BIN ${TARGET_DIR_PATH}
        touch -c -d "2009-01-01 00:00:00" ${TARGET_DIR_PATH}/*
        echo "编译成功: ${TARGE_DIR_NAME}"
    else
        echo "error"
        exit 1
    fi
}

build_android()
{
    build "Android" "arm64-v8a" "android-31"
    build "Android" "armeabi-v7a" "android-31"
    build "Android" "x86_64" "android-31"
    build "Android" "x86" "android-31"
}

build_linux()
{
    build "Linux" "x86_64"
    build "Linux" "aarch64"
}

build_windows()
{
    build "Windows" "x86_64"
    build "Windows" "aarch64"
}

# build
if [[ "$1" == "Android" ]]; then
    build_android
elif [[ "$1" == "Linux" ]]; then
    build_linux
    build_windows
else
    exit 1
fi

exit 0
