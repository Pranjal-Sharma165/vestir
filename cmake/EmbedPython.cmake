include_guard(GLOBAL)

# Python version
set(PYTHON_VERSION 3.14.10)
# Python major and minor version; used in filenames and such
set(PYTHON_MAJOR_MINOR 3.14)
# Where the build artifacts of things to do with Python will go
set(EMBEDPYTHON_BUILD_DIR ${CMAKE_BINARY_DIR}/EmbedPython_build)
# Paths to requirements lists
set(RUNTIME_REQUIREMENTS ${CMAKE_SOURCE_DIR}/python/requirements_runtime.txt)
set(FASHIONCLIP_ONNX_BUILD_REQUIREMENTS ${CMAKE_SOURCE_DIR}/python/requirements_dev.txt)
set(ANDROID_PYTHON_TARGET cpython_android)
# The URL used to get the Python source code for desktop
set(PYTHON_SRC_ZIP https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz)
# SHA256 checksum of PYTHON_SRC_ZIP
set(PYTHON_SRC_HASH 9e4996c19ae5a3df31f10c37723661f71d3455d73bd005f79df7098e5cc13192)
set(ANDROID_PY_ASSETS ${CMAKE_BINARY_DIR}/android/python)
set(FASHIONCLIP_TARGET fashionclip_assets)

# Create a virtual environment to install dev dependencies
set(VENV_DIR ${EMBEDPYTHON_BUILD_DIR}/.venv_build)
if(WIN32)
    set(VENV_PYTHON "${VENV_DIR}/Scripts/python.exe")
else()
    set(VENV_PYTHON "${VENV_DIR}/bin/python")
endif()

if(NOT EXISTS "${VENV_PYTHON}")
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    message(STATUS "Creating virtual environment in ${VENV_DIR}...")
    execute_process(
      COMMAND "${Python3_EXECUTABLE}" -m venv "${VENV_DIR}"
      RESULT_VARIABLE VENV_RESULT
    )
    if(NOT VENV_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to create Python virtual environment.")
    endif()

    unset(Python3_EXECUTABLE CACHE)
    unset(Python3_INCLUDE_DIRS CACHE)
    unset(Python3_LIBRARIES CACHE)
endif()

get_filename_component(req_file "${RUNTIME_REQUIREMENTS}" ABSOLUTE)

set(ENV{VIRTUAL_ENV} "${VENV_DIR}")
set(Python3_FIND_VIRTUALENV ONLY)
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

# Export the FashionCLIP weights to ONNX
function(export_fashionclip)
  cmake_parse_arguments(ARG "" "OUTPUT_DIR" "" ${ARGN})
  set(FASHIONCLIP_ASSET_DIR ${ARG_OUTPUT_DIR})
  set(FASHIONCLIP_EXPORT_SCRIPT ${CMAKE_SOURCE_DIR}/python/export_fashionclip.py)
  get_filename_component(EXPORT_REQ "${FASHIONCLIP_ONNX_BUILD_REQUIREMENTS}" ABSOLUTE)
  set(OUTPUTS
    ${FASHIONCLIP_ASSET_DIR}/image.onnx
    ${FASHIONCLIP_ASSET_DIR}/text.onnx
    ${FASHIONCLIP_ASSET_DIR}/fashionclip.json
    ${FASHIONCLIP_ASSET_DIR}/tokenizer.json)

  add_custom_command(OUTPUT ${OUTPUTS}
    COMMAND ${VENV_PYTHON} -m pip install
            --no-warn-script-location
            -r ${EXPORT_REQ}
    COMMAND ${VENV_PYTHON} ${FASHIONCLIP_EXPORT_SCRIPT} ${FASHIONCLIP_ASSET_DIR}
    DEPENDS ${FASHIONCLIP_EXPORT_SCRIPT} ${EXPORT_REQ}
    USES_TERMINAL)
  add_custom_target(${FASHIONCLIP_TARGET} ALL DEPENDS ${OUTPUTS})
endfunction()

# Set up the project for desktop.
# This assumes that desktop is for development builds only (this is a mobile
# app after all), and as such borrows the same virtual environment as that which
# is used in export_fashionclip.
function(setup_python_desktop)
  export_fashionclip(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/python/ai/classifier/fashionclip)

  set(RUNTIME_PIP_STAMP ${EMBEDPYTHON_BUILD_DIR}/runtime-pip.stamp)
  add_custom_command(OUTPUT ${RUNTIME_PIP_STAMP}
    COMMAND ${VENV_PYTHON} -m pip install --no-warn-script-location -r ${req_file}
    COMMAND ${CMAKE_COMMAND} -E touch ${RUNTIME_PIP_STAMP}
    DEPENDS ${req_file}
    USES_TERMINAL)
  add_custom_target(runtime_python_packages DEPENDS ${RUNTIME_PIP_STAMP})
  add_dependencies(appVestir runtime_python_packages)
  add_dependencies(${FASHIONCLIP_TARGET} runtime_python_packages)

  set(Python3_FIND_VIRTUALENV FIRST)
  find_package(Python3 COMPONENTS Interpreter Development.Embed REQUIRED)
  target_link_libraries(appVestir PRIVATE Python3::Python)
  target_compile_definitions(appVestir PRIVATE
      PYTHON_HOME_DIR=""
      PYTHON_EXECUTABLE="${VENV_PYTHON}"
      APP_PYTHON_DIR="${CMAKE_SOURCE_DIR}/python")
endfunction()

function(setup_python_android)
  if(NOT ANDROID OR NOT ANDROID_ABI OR NOT ANDROID_PLATFORM_LEVEL)
    message(FATAL_ERROR "setup_python_android() requires the NDK toolchain file")
  endif()

  if(NOT ANDROID_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "Unsupported ANDROID_ABI '${ANDROID_ABI}', expected 'arm64-v8a'")
  endif()

  export_fashionclip(OUTPUT_DIR ${ANDROID_PY_ASSETS}/ai/classifier/fashionclip)

  set(ANDROID_PYTHON_EXTRACT_DIR ${EMBEDPYTHON_BUILD_DIR}/android/${ANDROID_ABI})
  set(ANDROID_PYTHON_ARCHIVE_NAME python-${PYTHON_VERSION}-aarch64-linux-android.tar.gz)
  set(ANDROID_PYTHON_BUILD_ZIP https://www.python.org/ftp/python/${PYTHON_VERSION}/${ANDROID_PYTHON_ARCHIVE_NAME})
  set(ANDROID_PYTHON_HASH 423c80b957330cf0ebb92b9b20f2294c292144a441bdb0ceb8c69d14928e569b)
  set(ANDROID_PIP_PACKAGES ${ANDROID_PYTHON_EXTRACT_DIR}/prefix PARENT_SCOPE)
  set(ANDROID_PIP_LINK_DIR ${ANDROID_PIP_PACKAGES}/lib PARENT_SCOPE)
  set(ANDROID_LIBPYTHON ${ANDROID_PIP_LINK_DIR}/libpython${PYTHON_MAJOR_MINOR}.so)
  set(ANDROID_PIP_INCLUDE_DIR ${ANDROID_PIP_PACKAGES}/include/python${PYTHON_MAJOR_MINOR})

  if(NOT EXISTS ${ANDROID_LIBPYTHON})
    file(DOWNLOAD
      ${ANDROID_PYTHON_BUILD_ZIP}
      ${EMBEDPYTHON_BUILD_DIR}/downloads/${ANDROID_PYTHON_ARCHIVE_NAME}
      EXPECTED_HASH SHA256=${ANDROID_PYTHON_HASH}
      STATUS dl_status SHOW_PROGRESS)
    list(GET dl_status 0 dl_code)
    if(dl_code)
      message(FATAL_ERROR "Download failed: ${dl_status}")
    endif()
    file(ARCHIVE_EXTRACT
      INPUT ${EMBEDPYTHON_BUILD_DIR}/downloads/${ANDROID_PYTHON_ARCHIVE_NAME}
      DESTINATION ${ANDROID_PYTHON_EXTRACT_DIR})
  endif()

  if(NOT EXISTS ${ANDROID_LIBPYTHON})
    message(FATAL_ERROR "${ANDROID_LIBPYTHON} not found; unexpected archive layout")
  endif()

  get_filename_component(req_file "${RUNTIME_REQUIREMENTS}" ABSOLUTE)
  set(ANDROID_PIP_STAMP ${EMBEDPYTHON_BUILD_DIR}/android/${ANDROID_ABI}-pip.stamp)
  add_custom_command(OUTPUT ${ANDROID_PIP_STAMP}
    COMMAND ${VENV_PYTHON} -m pip install
            --no-warn-script-location
            --only-binary=:all:
            --python-version ${PYTHON_MAJOR_MINOR}
            --platform android_24_arm64_v8a
            --extra-index-url https://pypi.flet.dev
            --target ${ANDROID_PIP_LINK_DIR}
            -r ${req_file}
            COMMAND ${CMAKE_COMMAND} -E touch ${ANDROID_PIP_STAMP}
            DEPENDS ${req_file}
    USES_TERMINAL)

  add_custom_target(${ANDROID_PYTHON_TARGET} ALL DEPENDS ${ANDROID_PIP_STAMP})
  add_library(Python::Embed SHARED IMPORTED GLOBAL)
  set_target_properties(Python::Embed PROPERTIES
    IMPORTED_LOCATION ${ANDROID_LIBPYTHON}
    IMPORTED_SONAME libpython${PYTHON_MAJOR_MINOR}.so
    INTERFACE_INCLUDE_DIRECTORIES ${ANDROID_PIP_INCLUDE_DIR}
    INTERFACE_LINK_DIRECTORIES ${ANDROID_PIP_LINK_DIR})
  add_dependencies(Python::Embed ${ANDROID_PYTHON_TARGET})
endfunction()
