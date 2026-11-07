cmake_minimum_required(VERSION 3.16)

project(Vestir VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20) # Set C++ standard to 20

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6
    REQUIRED COMPONENTS
    Quick
    QuickControls2
    Multimedia
    Sql
)

qt_standard_project_setup(REQUIRES 6.10)

add_subdirectory(src/view/DesignTokens)

qt_add_executable(appVestir
    src/main.cpp
    src/repository/Database.h
    src/repository/Database.cpp
    src/repository/PhotoStore.h
    src/repository/PhotoStore.cpp
)

set_source_files_properties(src/database/schema.sql PROPERTIES QT_RESOURCE_ALIAS db/schema.sql)
set_source_files_properties(src/database/seed.sql PROPERTIES QT_RESOURCE_ALIAS db/seed.sql)
qt_add_resources(appVestir "database"
    PREFIX "/"
    FILES
        src/database/schema.sql
        src/database/seed.sql
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
        src/view/pages/edititem/EditItemPage.qml
        src/view/pages/additem/AddItemPage.qml
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
    DEPENDENCIES TARGET DesignTokens
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
    PRIVATE Qt6::Sql
    DesignTokens
)

install(TARGETS appVestir
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)