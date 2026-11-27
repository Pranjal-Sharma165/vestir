cmake_minimum_required(VERSION 3.16)

project(Vestir VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20) # Set C++ standard to 20

set(CMAKE_CXX_STANDARD_REQUIRED ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(EmbedPython)

set(SQLITE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/sqlite)

add_library(sqlite3 STATIC
    ${SQLITE_DIR}/sqlite3.c
    ${SQLITE_DIR}/sqlite3.h
)
target_include_directories(sqlite3 PUBLIC ${SQLITE_DIR})

find_package(Qt6
    REQUIRED COMPONENTS
    Quick
    QuickControls2
    Multimedia
    Network
)

qt_standard_project_setup(REQUIRES 6.10)

add_subdirectory(src/view/DesignTokens)

qt_add_executable(appVestir
    src/main.cpp
    src/backend/Database.h
    src/backend/Database.cpp
    src/backend/PhotoStore.h
    src/backend/PhotoStore.cpp
    src/backend/ClothingRepository.h
    src/backend/ClothingRepository.cpp
    src/backend/WeatherService.h
    src/backend/WeatherService.cpp
    src/backend/Errors.h
    src/backend/HistoryRepository.h
    src/backend/HistoryRepository.cpp
    src/backend/CalendarService.h
    src/backend/CalendarService.cpp
    src/backend/MlClient.h
    src/backend/MlClient.cpp
    src/backend/RecommendationService.h
    src/backend/RecommendationService.cpp
)

# The files in ${CMAKE_SOURCE_DIR}/android are copied into this location 
# before androiddeployqt packs them into the APK
set_property(TARGET appVestir PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_BINARY_DIR}/android)

if (ANDROID)
  setup_python_android()
else()
  setup_python_desktop(appVestir)
endif()

set_source_files_properties(src/database/schema.sql PROPERTIES QT_RESOURCE_ALIAS db/schema.sql)
qt_add_resources(appVestir "database"
    PREFIX "/"
    FILES
        src/database/schema.sql
)

set_source_files_properties(src/view/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(appVestir
    URI Vestir
    QML_FILES
        src/view/Main.qml
        src/view/pages/settings/SettingsPage.qml
        src/view/pages/recommendations/HomePage.qml
        src/view/pages/recommendations/components/RecommendationElement.qml
        src/view/pages/camera/CameraPage.qml
        src/view/pages/closet/ClosetPage.qml
        src/view/pages/closet/components/ClosetCard.qml
        src/view/pages/calendar/CalendarPage.qml
        src/view/pages/history/HistoryPage.qml
        src/view/components/ImagePlaceholder.qml
        src/view/components/NavigationIcon.qml
        src/view/Theme.qml
        src/view/pages/edititem/EditItemPage.qml
        src/view/pages/additem/AddItemPage.qml
        src/view/pages/calendar/components/DayOfWeekButton.qml
        src/view/pages/calendar/components/EventCard.qml
        src/view/components/StyledButton.qml
        src/view/components/StyledButtonDark.qml
        src/view/components/NavTabButton.qml
        src/view/components/ItemLabelPicker.qml
        src/view/pages/closet/components/CategoryBlock.qml
    SOURCES
        src/model/OutfitRecommendation.h
        src/model/OutfitFormality.h
        src/model/pages/ClosetPageModel.h
        src/model/ClothingCategory.h
        src/model/CalendarEvent.h
        src/model/pages/RecommendationPageModel.h
        src/model/ClothingItem.h
        src/model/ClothingColour.h
        src/model/helpers/CameraHelper.h
        src/model/helpers/ImageRecognitionHelper.h
        src/model/pages/CalendarPageModel.h
        src/model/pages/HistoryPageModel.h
        src/model/pages/WeatherSettingsModel.h
        src/model/DisplayText.h
    DEPENDENCIES TARGET DesignTokens
    RESOURCES
        resources/fonts/Montserrat-VariableFont_wght.ttf
        resources/fonts/Outfit-VariableFont_wght.ttf
        SOURCES src/backend/CalendarEntry.h
        SOURCES src/backend/WeatherData.h
)

target_include_directories(appVestir PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/model)
target_include_directories(appVestir PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/model/helpers)
target_include_directories(appVestir PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/model/pages)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appVestir PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appVestir
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appVestir
    PRIVATE Qt6::Quick
    PRIVATE Qt6::QuickControls2
    PRIVATE Qt6::Multimedia
    PRIVATE Qt6::Network
    PRIVATE sqlite3
    DesignTokens
)

install(TARGETS appVestir
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(ANDROID)
  file(COPY ${CMAKE_SOURCE_DIR}/android/ DESTINATION ${CMAKE_BINARY_DIR}/android)
  set_property(TARGET appVestir APPEND PROPERTY QT_ANDROID_EXTRA_LIBS
      ${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/${CMAKE_ANDROID_ARCH_ABI}/libcrypto_3.so
      ${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/${CMAKE_ANDROID_ARCH_ABI}/libssl_3.so)
  set(ANDROID_WHEEL_LIBS
      libtiff.so
      libjpeg.so
      libfreetype.so
      libwebp.so
      libwebpdemux.so
      libwebpmux.so
      libsharpyuv.so
      libturbojpeg.so
      libyaml.so)
  foreach(so IN LISTS ANDROID_WHEEL_LIBS)
      set_property(TARGET appVestir APPEND PROPERTY QT_ANDROID_EXTRA_LIBS
          ${ANDROID_PIP_LINK_DIR}/opt/lib/${so})
  endforeach()

  target_link_libraries(appVestir PRIVATE Python::Embed)

  file(GLOB_RECURSE PYTHON_SOURCES CONFIGURE_DEPENDS
       ${CMAKE_SOURCE_DIR}/python/*)
  list(FILTER PYTHON_SOURCES EXCLUDE REGEX "/(\\.venv|venv|__pycache__)/")
  set(ANDROID_ASSETS_STAMP ${CMAKE_BINARY_DIR}/android-assets.stamp)
  set(libs ${ANDROID_PY_ASSETS}/lib/python${PYTHON_MAJOR_MINOR})
  add_custom_command(OUTPUT ${ANDROID_ASSETS_STAMP}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${ANDROID_PIP_PACKAGES}/lib/python${PYTHON_MAJOR_MINOR}
            ${libs}
    COMMAND ${CMAKE_COMMAND} -E rename
            ${libs}/site-packages/onnxruntime/capi/onnxruntime_pybind11_state.cpython-314.so
            ${libs}/site-packages/onnxruntime/capi/onnxruntime_pybind11_state.so
    COMMAND ${CMAKE_COMMAND} -DSRC=${CMAKE_SOURCE_DIR}/python -DDST=${ANDROID_PY_ASSETS}/app
            -P ${CMAKE_SOURCE_DIR}/cmake/CopyWithExcludeVenv.cmake
    COMMAND ${CMAKE_COMMAND} -E touch ${ANDROID_ASSETS_STAMP}
    DEPENDS ${PYTHON_SOURCES} ${ANDROID_PYTHON_BUILD_DIR}/android/${ANDROID_ABI}-pip.stamp
    VERBATIM)
  add_custom_target(android_python_assets DEPENDS ${ANDROID_ASSETS_STAMP})
  add_dependencies(android_python_assets ${ANDROID_PYTHON_TARGET} ${FASHIONCLIP_TARGET})
  add_dependencies(appVestir android_python_assets)
endif()

add_dependencies(appVestir ${FASHIONCLIP_TARGET})

if(NOT ANDROID)
  enable_testing()
  add_subdirectory(tests)
endif()
