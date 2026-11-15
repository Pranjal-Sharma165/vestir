include_guard(GLOBAL)
include(ExternalProject)
# Used later to specify level of parallelism when building Python
include(ProcessorCount)

# Python version
set(PYTHON_VERSION 3.14.10)
# Python major and minor version; used in filenames and such
set(PYTHON_MAJOR_MINOR 3.14)
# Where the build artifacts will go
set(ROOT_DIR ${CMAKE_BINARY_DIR}/python)
# Location of the libpython shared object
set(LIBPYTHON ${ROOT_DIR}/lib/libpython${PYTHON_MAJOR_MINOR}.so)
# Location of the include directory
set(INCLUDE_DIR ${ROOT_DIR}/include/python${PYTHON_MAJOR_MINOR})
# Torch version
set(TORCH_VERSION 2.14.1)
# Path to requirements list
set(REQUIREMENTS_TXT_PATH ${CMAKE_SOURCE_DIR}/python/requirements_ml_only.txt)
set(DESKTOP_PYTHON_TARGET cpython_desktop)
set(ANDROID_PYTHON_TARGET cpython_android)
# Location of the Python executable
set(DESKTOP_PYTHON_EXECUTABLE ${ROOT_DIR}/bin/python3)
# Used in top level to set include path
set(EMBEDDED_PYTHON_PREFIX ${ROOT_DIR})
# The URL used to get the Python source code for desktop
set(PYTHON_SRC_ZIP https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz)
# SHA256 checksum of PYTHON_SRC_ZIP
set(PYTHON_SRC_HASH 9e4996c19ae5a3df31f10c37723661f71d3455d73bd005f79df7098e5cc13192)

function(build_host_python)
  find_program(GNU_MAKE NAMES gmake make REQUIRED)
  ProcessorCount(nproc)
  if(nproc EQUAL 0)
    set(nproc 1)
  endif()

  ExternalProject_Add(${DESKTOP_PYTHON_TARGET}
    URL ${PYTHON_SRC_ZIP}
    URL_HASH SHA256=${PYTHON_SRC_HASH}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --prefix=${ROOT_DIR}
      --enable-shared
      --config-cache
      --with-ensurepip=install
      --disable-test-modules
      "LDFLAGS=-Wl,-rpath,${ROOT_DIR}/lib"
    BUILD_COMMAND   ${GNU_MAKE} -j${nproc}
    INSTALL_COMMAND ${GNU_MAKE} install
    BUILD_BYPRODUCTS ${LIBPYTHON}
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD ON
    USES_TERMINAL_INSTALL ON
  )
  ExternalProject_Add_StepTargets(${DESKTOP_PYTHON_TARGET} install)
endfunction()

function(setup_python_desktop)
  build_host_python()

  # Preinstall the CPU-only version of torch to avoid downloading the extra 
  # stuff needed for GPU acceleration
  ExternalProject_Add_Step(${DESKTOP_PYTHON_TARGET} preinstall_torch_cpu
      COMMAND ${DESKTOP_PYTHON_EXECUTABLE} -m pip install torch==${TORCH_VERSION}
            --index-url https://download.pytorch.org/whl/cpu
    DEPENDEES install
    USES_TERMINAL 1
  )
 
  # Install the requirements listed in REQUIREMENTS_TXT_PATH
  get_filename_component(req_file "${REQUIREMENTS_TXT_PATH}" ABSOLUTE)
  ExternalProject_Add_Step(${DESKTOP_PYTHON_TARGET} pip_requirements
      COMMAND ${DESKTOP_PYTHON_EXECUTABLE} -m pip install --no-warn-script-location -r ${req_file}
    DEPENDEES preinstall_torch_cpu
    DEPENDS   ${req_file}
    USES_TERMINAL 1
  )

  file(MAKE_DIRECTORY ${INCLUDE_DIR})
 
  add_library(Python::Embed SHARED IMPORTED GLOBAL)
  set_target_properties(Python::Embed PROPERTIES
    IMPORTED_LOCATION ${LIBPYTHON}
    IMPORTED_SONAME libpython${PYTHON_MAJOR_MINOR}.so.1.0
    INTERFACE_INCLUDE_DIRECTORIES ${INCLUDE_DIR}
    INTERFACE_LINK_DIRECTORIES ${ROOT_DIR}/lib
  )
  add_dependencies(Python::Embed ${DESKTOP_PYTHON_TARGET})
endfunction()

function(setup_python_android)
  if(NOT ANDROID OR NOT ANDROID_ABI OR NOT ANDROID_PLATFORM_LEVEL)
    message(FATAL_ERROR "setup_python_android() requires the NDK toolchain file")
  endif()

  if(NOT ANDROID_ABI STREQUAL "arm64-v8a")
    message(FATAL_ERROR "Unsupported ANDROID_ABI '${ANDROID_ABI}', expected 'arm64-v8a'")
  endif()

  build_host_python()

  set(ANDROID_ARCH_PIP arm64_v8a)
  set(ANDROID_TRIPLE aarch64-linux-android)
  set(ANDROID_PYTHON_ARCHIVE_NAME python-${PYTHON_VERSION}-${ANDROID_TRIPLE}.tar.gz)
  set(ANDROID_PYTHON_BUILD_ZIP https://www.python.org/ftp/python/${PYTHON_VERSION}/${ANDROID_PYTHON_ARCHIVE_NAME})
  set(ANDROID_PYTHON_HASH 423c80b957330cf0ebb92b9b20f2294c292144a441bdb0ceb8c69d14928e569b)
  set(ANDROID_PYTHON_EXTRACT_DIR ${ROOT_DIR}/android/${ANDROID_ABI})
  set(ANDROID_LIBPYTHON ${ANDROID_PYTHON_EXTRACT_DIR}/prefix/lib/libpython${PYTHON_MAJOR_MINOR}.so)
  set(ANDROID_PYTHON_INCLUDE_DIR ${ANDROID_PYTHON_EXTRACT_DIR}/prefix/include/python${PYTHON_MAJOR_MINOR})
  
  if(NOT EXISTS ${ANDROID_LIBPYTHON})
    file(DOWNLOAD
      ${ANDROID_PYTHON_BUILD_ZIP}
      ${ROOT_DIR}/downloads/${ANDROID_PYTHON_ARCHIVE_NAME}
      EXPECTED_HASH SHA256=${ANDROID_PYTHON_HASH}
      STATUS dl_status SHOW_PROGRESS)
    list(GET dl_status 0 dl_code)
    if(dl_code)
      message(FATAL_ERROR "Download failed: ${dl_status}")
    endif()
    file(ARCHIVE_EXTRACT
      INPUT ${ROOT_DIR}/downloads/${ANDROID_PYTHON_ARCHIVE_NAME}
      DESTINATION ${ANDROID_PYTHON_EXTRACT_DIR})
  endif()

  if(NOT EXISTS ${ANDROID_LIBPYTHON})
    message(FATAL_ERROR "${ANDROID_LIBPYTHON} not found; unexpected archive layout")
  endif()

  get_filename_component(req_file "${REQUIREMENTS_TXT_PATH}" ABSOLUTE)
  set(ANDROID_PIP_STAMP ${ROOT_DIR}/android/${ANDROID_ABI}-pip.stamp)
  add_custom_command(OUTPUT ${ANDROID_PIP_STAMP}
    COMMAND ${DESKTOP_PYTHON_EXECUTABLE} -m pip install
            --no-warn-script-location
            --only-binary=:all:
            --implementation cp
            --python-version ${PYTHON_MAJOR_MINOR}
            --platform android_${ANDROID_PLATFORM_LEVEL}_${ANDROID_ARCH_PIP}
            --target ${ANDROID_PYTHON_EXTRACT_DIR}/prefix/lib/python${PYTHON_MAJOR_MINOR}/site-packages
            -r ${req_file}
            COMMAND ${CMAKE_COMMAND} -E touch ${ANDROID_PIP_STAMP}
    DEPENDS ${req_file}
    USES_TERMINAL)

  add_custom_target(${ANDROID_PYTHON_TARGET} ALL DEPENDS ${ANDROID_PIP_STAMP})
  add_dependencies(${ANDROID_PYTHON_TARGET} ${DESKTOP_PYTHON_TARGET}-install)
  add_library(Python::Embed SHARED IMPORTED GLOBAL)
  set_target_properties(Python::Embed PROPERTIES
    IMPORTED_LOCATION ${ANDROID_LIBPYTHON}
    IMPORTED_SONAME libpython${PYTHON_MAJOR_MINOR}.so
    INTERFACE_INCLUDE_DIRECTORIES ${ANDROID_PYTHON_INCLUDE_DIR}
    INTERFACE_LINK_DIRECTORIES ${ANDROID_PYTHON_EXTRACT_DIR}/prefix/lib)
  add_dependencies(Python::Embed ${ANDROID_PYTHON_TARGET})
endfunction()
