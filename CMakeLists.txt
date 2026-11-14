cmake_minimum_required(VERSION 3.16)

project(Vestir VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20) # Set C++ standard to 20

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(APP_PYTHON_DIR ${CMAKE_BINARY_DIR}/app_python)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(EmbedPython)

# Call function defined in cmake/EmbedPython.cmake
add_python()

find_package(Qt6
    REQUIRED COMPONENTS
    Quick
    QuickControls2
    Multimedia
    Sql
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
    src/backend/EventFormality.h
    src/backend/HistoryRepository.h
    src/backend/HistoryRepository.cpp
    src/backend/CalendarService.h
    src/backend/CalendarService.cpp
    src/backend/MlClient.h
    src/backend/MlClient.cpp
    src/backend/RecommendationService.h
    src/backend/RecommendationService.cpp
)

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
        src/model/WeatherSnapshot.h
        src/model/WeatherSnapshot.cpp
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
        SOURCES src/testpython.h
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
    QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/android
)

if(ANDROID)
    # Qt's TLS backend needs OpenSSL bundled in the APK for HTTPS.
    set_property(TARGET appVestir APPEND PROPERTY QT_ANDROID_EXTRA_LIBS
        ${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/${CMAKE_ANDROID_ARCH_ABI}/libcrypto_3.so
        ${CMAKE_CURRENT_SOURCE_DIR}/android/openssl/${CMAKE_ANDROID_ARCH_ABI}/libssl_3.so)
endif()

target_link_libraries(appVestir
    PRIVATE Qt6::Quick
    PRIVATE Qt6::QuickControls2
    PRIVATE Qt6::Multimedia
    PRIVATE Qt6::Sql
    PRIVATE Qt6::Network
    DesignTokens
)

target_link_libraries(appVestir PRIVATE Python::Embed)
target_compile_definitions(appVestir PRIVATE PYTHON_HOME_DIR="${EMBEDDED_PYTHON_PREFIX}")
set_target_properties(appVestir PROPERTIES BUILD_RPATH ${EMBEDDED_PYTHON_PREFIX}/lib)

install(TARGETS appVestir
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Copy python/ dir to app_python/
file(GLOB_RECURSE APP_PYTHON_SOURCES CONFIGURE_DEPENDS
     ${CMAKE_SOURCE_DIR}/python/*)
add_custom_command(
  OUTPUT  ${APP_PYTHON_DIR}/.stamp
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/python ${APP_PYTHON_DIR}
  COMMAND ${CMAKE_COMMAND} -E touch ${APP_PYTHON_DIR}/.stamp
  DEPENDS ${APP_PYTHON_SOURCES}
)
add_custom_target(copy_app_python DEPENDS ${APP_PYTHON_DIR}/.stamp)
add_dependencies(appVestir copy_app_python)
target_compile_definitions(appVestir PRIVATE APP_PYTHON_DIR="${APP_PYTHON_DIR}")
