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
        src/view/Theme.qml
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
    DEPENDENCIES TARGET DesignTokens
    QML_FILES src/view/pages/edititem/EditItemPage.qml
    SOURCES src/model/ClothingColour.h
)

target_include_directories(appVestir PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/model)
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
    DesignTokens
)

install(TARGETS appVestir
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)