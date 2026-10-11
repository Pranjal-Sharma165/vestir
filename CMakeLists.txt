cmake_minimum_required(VERSION 3.16)

project(Vestir VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6
    REQUIRED COMPONENTS
    Quick
    QuickControls2
)

qt_standard_project_setup(REQUIRES 6.10)

add_subdirectory(src/view/DesignTokens)

qt_add_executable(appVestir
    src/main.cpp
)

qt_add_qml_module(appVestir
    URI Vestir
    NO_CACHEGEN
    QML_FILES
        src/view/Main.qml
        src/view/SettingsPage.qml
        src/view/HomePage.qml
        src/view/ClosetPage.qml
        src/view/CalendarPage.qml
        src/view/HistoryPage.qml
        src/view/RecommendationElement.qml
    SOURCES
        src/model/recommendation.h
        src/model/recommendation.cpp
    DEPENDENCIES TARGET DesignTokens
    QML_FILES src/view/ImagePlaceholder.qml
    SOURCES src/model/OutfitFormality.h
    SOURCES src/model/weathersample.h src/model/weathersample.cpp
)

target_include_directories(appVestir PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/model)

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
    DesignTokens
)

install(TARGETS appVestir
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)