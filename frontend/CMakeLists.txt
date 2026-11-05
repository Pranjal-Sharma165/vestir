cmake_minimum_required(VERSION 3.16)

project(Vestir VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6
    REQUIRED COMPONENTS
    Quick
    QuickControls2
    Sql
)

qt_standard_project_setup(REQUIRES 6.10)

add_subdirectory(src/view/DesignTokens)

qt_add_executable(appVestir
    src/main.cpp
    src/backend/Database.h
    src/backend/Database.cpp
    src/backend/ClothingRepository.h
    src/backend/ClothingRepository.cpp
)

set_source_files_properties(src/view/Theme.qml
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(appVestir
    URI Vestir
    QML_FILES
        src/view/Main.qml
        src/view/SettingsPage.qml
        src/view/HomePage.qml
        src/view/ClosetPage.qml
        src/view/CalendarPage.qml
        src/view/HistoryPage.qml
        src/view/RecommendationElement.qml
        src/view/ImagePlaceholder.qml
        src/view/Theme.qml
    SOURCES
    src/model/OutfitRecommendation.h
    src/model/OutfitRecommendation.cpp
        src/model/OutfitFormality.h
        src/model/WeatherSnapshot.h
        src/model/WeatherSnapshot.cpp
        src/model/CalendarModel.h
        src/model/CalendarModel.cpp
    DEPENDENCIES TARGET DesignTokens
    SOURCES src/model/ClothingItem.h
    SOURCES
    QML_FILES src/view/ClosetCard.qml
    SOURCES src/model/Closet.h
    SOURCES
    SOURCES
    SOURCES src/model/ClothingCategory.h
)

target_include_directories(appVestir PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/model)

# Template database copied to the user's app-data folder on first run, and the
# folder holding the Python scripts (recommend_cli.py, calendar_sync.py).
# Works both in the single GitHub repo layout and in the local vestir-all layout.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/database/vestir.db")
    set(VESTIR_TEMPLATE_DB_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/database/vestir.db")
else()
    set(VESTIR_TEMPLATE_DB_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/../database/sqlite/vestir.db")
endif()
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/recommend_cli.py")
    set(VESTIR_AI_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}")
else()
    set(VESTIR_AI_DIR_DEFAULT "${CMAKE_CURRENT_SOURCE_DIR}/../ai-models")
endif()
set(VESTIR_TEMPLATE_DB "${VESTIR_TEMPLATE_DB_DEFAULT}" CACHE FILEPATH "Template database")
set(VESTIR_AI_DIR "${VESTIR_AI_DIR_DEFAULT}" CACHE PATH "Folder with the Python scripts")
set(VESTIR_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "Project root (holds wardrobe/)")
target_compile_definitions(appVestir PRIVATE
    VESTIR_ROOT_DIR="${VESTIR_ROOT_DIR}"
    VESTIR_TEMPLATE_DB="${VESTIR_TEMPLATE_DB}"
    VESTIR_AI_DIR="${VESTIR_AI_DIR}")

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
    PRIVATE Qt6::Sql
    DesignTokens
)

install(TARGETS appVestir
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)