include_guard(GLOBAL)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

# 允许 CI 使用固定版本，也允许本地构建指定已下载的依赖目录。
set(ZCCHAT2_ONNXRUNTIME_VERSION "1.23.2" CACHE STRING
    "ONNX Runtime version used by Silero VAD")
set(ZCCHAT2_ONNXRUNTIME_ROOT "" CACHE PATH
    "Path to a prebuilt ONNX Runtime ${ZCCHAT2_ONNXRUNTIME_VERSION} package")
set(ZCCHAT2_SILERO_MODEL_PATH "" CACHE FILEPATH
    "Path to a local Silero VAD v6.2.1 ONNX model")

set(_zc_ort_version "${ZCCHAT2_ONNXRUNTIME_VERSION}")
if(NOT ZCCHAT2_ONNXRUNTIME_ROOT)
    # 根据目标平台选择官方预编译包，并在下载时校验 SHA-256。
    if(WIN32)
        set(_zc_ort_archive "onnxruntime-win-x64-${_zc_ort_version}.zip")
        set(_zc_ort_hash
            "0b38df9af21834e41e73d602d90db5cb06dbd1ca618948b8f1d66d607ac9f3cd")
    elseif(APPLE)
        # The universal2 package keeps the existing arm64+x86_64 application build.
        set(_zc_ort_archive "onnxruntime-osx-universal2-${_zc_ort_version}.tgz")
        set(_zc_ort_hash
            "49ae8e3a66ccb18d98ad3fe7f5906b6d7887df8a5edd40f49eb2b14e20885809")
    elseif(UNIX AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64|AMD64)$")
        set(_zc_ort_archive "onnxruntime-linux-x64-${_zc_ort_version}.tgz")
        set(_zc_ort_hash
            "1fa4dcaef22f6f7d5cd81b28c2800414350c10116f5fdd46a2160082551c5f9b")
    else()
        message(FATAL_ERROR
            "Silero VAD currently supports Windows x64, Linux x64, and macOS universal2 builds")
    endif()

    FetchContent_Declare(zc_onnxruntime
        URL "https://github.com/microsoft/onnxruntime/releases/download/v${_zc_ort_version}/${_zc_ort_archive}"
        URL_HASH "SHA256=${_zc_ort_hash}"
    )
    FetchContent_MakeAvailable(zc_onnxruntime)
    set(ZCCHAT2_ONNXRUNTIME_ROOT "${zc_onnxruntime_SOURCE_DIR}")
endif()

if(NOT EXISTS "${ZCCHAT2_ONNXRUNTIME_ROOT}/include/onnxruntime_cxx_api.h")
    message(FATAL_ERROR
        "Invalid ONNX Runtime package: ${ZCCHAT2_ONNXRUNTIME_ROOT}")
endif()

# 用统一的 imported target 隔离各平台的 ONNX Runtime 文件布局。
add_library(ZcOnnxRuntime SHARED IMPORTED GLOBAL)
set_target_properties(ZcOnnxRuntime PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${ZCCHAT2_ONNXRUNTIME_ROOT}/include"
)

if(WIN32)
    set(_zc_ort_runtime "${ZCCHAT2_ONNXRUNTIME_ROOT}/lib/onnxruntime.dll")
    set(ZCCHAT2_ONNXRUNTIME_RUNTIME_NAME "onnxruntime.dll")
    set_target_properties(ZcOnnxRuntime PROPERTIES
        IMPORTED_IMPLIB "${ZCCHAT2_ONNXRUNTIME_ROOT}/lib/onnxruntime.lib"
        IMPORTED_LOCATION "${_zc_ort_runtime}"
    )
elseif(APPLE)
    set(_zc_ort_runtime
        "${ZCCHAT2_ONNXRUNTIME_ROOT}/lib/libonnxruntime.${_zc_ort_version}.dylib")
    set(ZCCHAT2_ONNXRUNTIME_RUNTIME_NAME
        "libonnxruntime.${_zc_ort_version}.dylib")
    set_target_properties(ZcOnnxRuntime PROPERTIES
        IMPORTED_LOCATION "${_zc_ort_runtime}"
    )
else()
    set(_zc_ort_runtime
        "${ZCCHAT2_ONNXRUNTIME_ROOT}/lib/libonnxruntime.so.${_zc_ort_version}")
    set(ZCCHAT2_ONNXRUNTIME_RUNTIME_NAME
        "libonnxruntime.so.${_zc_ort_version}")
    string(REGEX MATCH "^[0-9]+" _zc_ort_major_version
        "${_zc_ort_version}")
    if(NOT _zc_ort_major_version)
        message(FATAL_ERROR
            "ONNX Runtime version must start with a numeric major version")
    endif()
    set(ZCCHAT2_ONNXRUNTIME_SONAME
        "libonnxruntime.so.${_zc_ort_major_version}")
    set_target_properties(ZcOnnxRuntime PROPERTIES
        IMPORTED_LOCATION "${_zc_ort_runtime}"
    )
endif()

if(NOT EXISTS "${_zc_ort_runtime}")
    message(FATAL_ERROR "ONNX Runtime library not found: ${_zc_ort_runtime}")
endif()
set(ZCCHAT2_ONNXRUNTIME_RUNTIME "${_zc_ort_runtime}")

# Silero 模型同样固定版本和哈希，避免构建时使用未知模型。
set(_zc_silero_hash
    "1a153a22f4509e292a94e67d6f9b85e8deb25b4988682b7e174c65279d8788e3")

if(ZCCHAT2_SILERO_MODEL_PATH)
    # 发布构建可以直接使用预下载模型，适合离线或受限网络环境。
    if(NOT EXISTS "${ZCCHAT2_SILERO_MODEL_PATH}")
        message(FATAL_ERROR
            "Silero VAD model not found: ${ZCCHAT2_SILERO_MODEL_PATH}")
    endif()
    file(SHA256 "${ZCCHAT2_SILERO_MODEL_PATH}" _zc_existing_model_hash)
    if(NOT _zc_existing_model_hash STREQUAL _zc_silero_hash)
        message(FATAL_ERROR
            "Silero VAD model has an unexpected SHA-256: ${ZCCHAT2_SILERO_MODEL_PATH}")
    endif()
    set(ZCCHAT2_SILERO_MODEL "${ZCCHAT2_SILERO_MODEL_PATH}")
    get_filename_component(ZCCHAT2_SILERO_MODEL_BASE
        "${ZCCHAT2_SILERO_MODEL}" DIRECTORY)
else()
    # 默认把模型下载到构建目录，随后由 Qt 资源系统嵌入应用。
    set(_zc_silero_dir "${CMAKE_CURRENT_BINARY_DIR}/_deps/silero-vad-v6.2.1")
    set(ZCCHAT2_SILERO_MODEL "${_zc_silero_dir}/silero_vad.onnx")
    set(ZCCHAT2_SILERO_MODEL_BASE "${_zc_silero_dir}")
    file(MAKE_DIRECTORY "${_zc_silero_dir}")

    set(_zc_download_model TRUE)
    if(EXISTS "${ZCCHAT2_SILERO_MODEL}")
        file(SHA256 "${ZCCHAT2_SILERO_MODEL}" _zc_existing_model_hash)
        if(_zc_existing_model_hash STREQUAL _zc_silero_hash)
            set(_zc_download_model FALSE)
        endif()
    endif()

    if(_zc_download_model)
        file(DOWNLOAD
            "https://raw.githubusercontent.com/snakers4/silero-vad/v6.2.1/src/silero_vad/data/silero_vad.onnx"
            "${ZCCHAT2_SILERO_MODEL}"
            EXPECTED_HASH "SHA256=${_zc_silero_hash}"
            STATUS _zc_model_status
            SHOW_PROGRESS
        )
        list(GET _zc_model_status 0 _zc_model_status_code)
        if(NOT _zc_model_status_code EQUAL 0)
            list(GET _zc_model_status 1 _zc_model_status_message)
            message(FATAL_ERROR "Failed to download Silero VAD model: ${_zc_model_status_message}")
        endif()
    endif()
endif()
