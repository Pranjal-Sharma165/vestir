set(BACKEND_DIR ${CMAKE_SOURCE_DIR}/src/backend)
set(SCHEMA_FILE ${CMAKE_SOURCE_DIR}/src/database/schema.sql)

function(add_backend_test name)
  add_executable(${name} ${ARGN})
  target_include_directories(${name} PRIVATE ${BACKEND_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(${name} PRIVATE "SCHEMA_PATH=\"${SCHEMA_FILE}\"")
  target_link_libraries(${name} PRIVATE sqlite3)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_backend_test(test_database test_database.cpp ${BACKEND_DIR}/Database.cpp)

add_backend_test(test_history_repository test_history_repository.cpp
  ${BACKEND_DIR}/Database.cpp
  ${BACKEND_DIR}/HistoryRepository.cpp)

set(MODEL_DIR ${CMAKE_SOURCE_DIR}/src/model)

add_backend_test(test_clothing_repository test_clothing_repository.cpp
  ${BACKEND_DIR}/Database.cpp
  ${BACKEND_DIR}/PhotoStore.cpp
  ${BACKEND_DIR}/ClothingRepository.cpp
  ${MODEL_DIR}/ClothingItem.h
  ${MODEL_DIR}/ClothingCategory.h
  ${MODEL_DIR}/ClothingColour.h
  ${MODEL_DIR}/OutfitFormality.h)
target_include_directories(test_clothing_repository PRIVATE ${MODEL_DIR})
target_link_libraries(test_clothing_repository PRIVATE Qt6::Qml)

add_backend_test(test_weather_service test_weather_service.cpp
  ${BACKEND_DIR}/Database.cpp
  ${BACKEND_DIR}/WeatherService.cpp
  ${BACKEND_DIR}/WeatherService.h)
target_link_libraries(test_weather_service PRIVATE Qt6::Network)

add_backend_test(test_calendar_service test_calendar_service.cpp
  MlClientStub.cpp
  ${BACKEND_DIR}/Database.cpp
  ${BACKEND_DIR}/CalendarService.cpp
  ${BACKEND_DIR}/CalendarService.h
  ${BACKEND_DIR}/MlClient.h
  ${MODEL_DIR}/ClothingItem.h
  ${MODEL_DIR}/ClothingCategory.h
  ${MODEL_DIR}/ClothingColour.h
  ${MODEL_DIR}/OutfitFormality.h
  ${MODEL_DIR}/CalendarEvent.h
  ${MODEL_DIR}/OutfitRecommendation.h)
target_include_directories(test_calendar_service PRIVATE ${MODEL_DIR})
target_link_libraries(test_calendar_service PRIVATE Qt6::Qml Qt6::Network)

add_backend_test(test_photo_store test_photo_store.cpp
  ${BACKEND_DIR}/PhotoStore.cpp)

add_backend_test(test_recommendation_service test_recommendation_service.cpp
  MlClientStub.cpp
  ${BACKEND_DIR}/Database.cpp
  ${BACKEND_DIR}/PhotoStore.cpp
  ${BACKEND_DIR}/ClothingRepository.cpp
  ${BACKEND_DIR}/WeatherService.cpp
  ${BACKEND_DIR}/CalendarService.cpp
  ${BACKEND_DIR}/RecommendationService.cpp
  ${BACKEND_DIR}/RecommendationService.h
  ${BACKEND_DIR}/CalendarService.h
  ${BACKEND_DIR}/WeatherService.h
  ${BACKEND_DIR}/MlClient.h
  ${MODEL_DIR}/ClothingItem.h
  ${MODEL_DIR}/ClothingCategory.h
  ${MODEL_DIR}/ClothingColour.h
  ${MODEL_DIR}/OutfitFormality.h
  ${MODEL_DIR}/CalendarEvent.h
  ${MODEL_DIR}/OutfitRecommendation.h)
target_include_directories(test_recommendation_service PRIVATE ${MODEL_DIR})
target_link_libraries(test_recommendation_service PRIVATE Qt6::Qml Qt6::Network)

# Integration tests for MlClient
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(EmbedPython)
add_backend_test(test_mlclient_integration test_mlclient_integration.cpp
    ${BACKEND_DIR}/MlClient.cpp
    ${BACKEND_DIR}/MlClient.h
    ${BACKEND_DIR}/ClothingRepository.cpp
    ${BACKEND_DIR}/PhotoStore.cpp
    ${BACKEND_DIR}/Database.cpp
    ${MODEL_DIR}/ClothingItem.h
    ${MODEL_DIR}/ClothingCategory.h
    ${MODEL_DIR}/ClothingColour.h
    ${MODEL_DIR}/OutfitFormality.h
    ${MODEL_DIR}/CalendarEvent.h
    ${MODEL_DIR}/OutfitRecommendation.h
)
add_dependencies(test_mlclient_integration ${FASHIONCLIP_TARGET})
setup_python_desktop(test_mlclient_integration)
target_include_directories(test_mlclient_integration PRIVATE ${MODEL_DIR})
target_link_libraries(test_mlclient_integration PRIVATE Qt6::Qml)
target_compile_definitions(test_mlclient_integration PRIVATE
    "TEST_IMAGE_DIR=\"${CMAKE_SOURCE_DIR}/tests/test_images\"")
