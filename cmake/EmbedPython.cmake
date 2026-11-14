include_guard(GLOBAL)
 
include(ExternalProject)

# Used later to specify level of parallelism when building Python
include(ProcessorCount)

# Python builds with GNU make
find_program(GNU_MAKE NAMES gmake make REQUIRED)
 
function(add_python)
  # Python version
  set(PYTHON_VERSION 3.14.10)
  # Python major and minor version; used in filenames and such
  set(MAJOR_MINOR 3.14)
  # Torch version (set here so that only the CPU version is installed)
  set(TORCH_VERSION 2.14.1)
  # Path to requirements list
  set(REQUIREMENTS_TXT_PATH ${CMAKE_SOURCE_DIR}/python/requirements_ml_only.txt)
  # Where the build artifacts will go
  set(BUILD_ARTIFACT_DIR ${CMAKE_BINARY_DIR}/python)
  # Name of the target
  set(TARGET_NAME cpython)
  # Update SHA256 checksum when changing version
  set(HASH 9e4996c19ae5a3df31f10c37723661f71d3455d73bd005f79df7098e5cc13192)
 # Location of the Python executable
  set(EXECUTABLE ${BUILD_ARTIFACT_DIR}/bin/python3)
  # Location of the libpython shared object
  set(LIBPYTHON ${BUILD_ARTIFACT_DIR}/lib/libpython${MAJOR_MINOR}.so)
  # Location of the include directory
  set(INCLUDE_DIR ${BUILD_ARTIFACT_DIR}/include/python${MAJOR_MINOR})
 
  # Set number of processors for building Python
  ProcessorCount(nproc)
  if(nproc EQUAL 0)
    set(nproc 1)
  endif()
 
  # Add CPython as external project
  ExternalProject_Add(${TARGET_NAME}
    URL https://www.python.org/ftp/python/${PYTHON_VERSION}/Python-${PYTHON_VERSION}.tgz
    URL_HASH SHA256=${HASH}
    CONFIGURE_COMMAND <SOURCE_DIR>/configure
      --prefix=${BUILD_ARTIFACT_DIR}
      --enable-shared
      --config-cache
      --with-ensurepip=install
      --disable-test-modules
      "LDFLAGS=-Wl,-rpath,${BUILD_ARTIFACT_DIR}/lib"
    BUILD_COMMAND   ${GNU_MAKE} -j${nproc}
    INSTALL_COMMAND ${GNU_MAKE} install
    BUILD_BYPRODUCTS ${LIBPYTHON}
    USES_TERMINAL_CONFIGURE ON
    USES_TERMINAL_BUILD ON
    USES_TERMINAL_INSTALL ON
  )
 
  # Preinstall the CPU-only version of torch to avoid downloading the extra stuff needed for GPU acceleration
  ExternalProject_Add_Step(${TARGET_NAME} preinstall_torch_cpu
      COMMAND ${EXECUTABLE} -m pip install torch==${TORCH_VERSION}
            --index-url https://download.pytorch.org/whl/cpu
    DEPENDEES install
    USES_TERMINAL 1
  )
 
  # Install the requirements listed in REQUIREMENTS_TXT_PATH
  get_filename_component(req_file "${REQUIREMENTS_TXT_PATH}" ABSOLUTE)
  ExternalProject_Add_Step(${TARGET_NAME} pip_requirements
    COMMAND ${EXECUTABLE} -m pip install --no-warn-script-location -r ${req_file}
    DEPENDEES preinstall_torch_cpu
    DEPENDS   ${req_file}
    USES_TERMINAL 1
  )
 
  file(MAKE_DIRECTORY ${INCLUDE_DIR})
 
  add_library(Python::Embed SHARED IMPORTED GLOBAL)
  set_target_properties(Python::Embed PROPERTIES
    IMPORTED_LOCATION ${LIBPYTHON}
    IMPORTED_SONAME libpython${MAJOR_MINOR}.so.1.0
    INTERFACE_INCLUDE_DIRECTORIES ${INCLUDE_DIR}
    INTERFACE_LINK_DIRECTORIES ${BUILD_ARTIFACT_DIR}/lib
  )
  add_dependencies(Python::Embed ${TARGET_NAME})
 
  set(EMBEDDED_PYTHON_PREFIX ${BUILD_ARTIFACT_DIR} PARENT_SCOPE)
  set(EMBEDDED_PYTHON_EXECUTABLE ${EXECUTABLE} PARENT_SCOPE)
  set(EMBEDDED_PYTHON_TARGET ${TARGET_NAME} PARENT_SCOPE)
endfunction()
