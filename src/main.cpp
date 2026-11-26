#include "ImageRecognitionHelper.h"
#include "model/helpers/CameraHelper.h"
#include "model/pages/CalendarPageModel.h"
#include "model/pages/ClosetPageModel.h"
#include "model/pages/HistoryPageModel.h"
#include "model/pages/RecommendationPageModel.h"
#include "model/pages/WeatherSettingsModel.h"
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QStandardPaths>
#include <filesystem>
#include <iostream>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>
#include <QtCore/qnativeinterface.h>
#endif

#include "backend/ClothingRepository.h"
#include "backend/Database.h"
#include "backend/HistoryRepository.h"
#include "backend/PhotoStore.h"

static void loadFont(const std::string path) {
  const int id =
      QFontDatabase::addApplicationFont(QString::fromStdString(path));
  if (id == -1)
    qWarning() << "Failed to load font:" << path;
}

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  loadFont(":/qt/qml/Vestir/resources/fonts/Montserrat-VariableFont_wght.ttf");
  loadFont(":/qt/qml/Vestir/resources/fonts/Outfit-VariableFont_wght.ttf");

  // Set application-wide body font
  app.setFont(QFont(QStringLiteral("Montserrat")));

  // create DB on first run
  QFile schemaFile(QStringLiteral(":/db/schema.sql"));
  if (!schemaFile.open(QIODevice::ReadOnly))
    qFatal("Cannot read database schema");
  const std::string schemaSql = schemaFile.readAll().toStdString();

  const std::filesystem::path dataDir(
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
          .toStdString());
  Database database((dataDir / "vestir.db").string(), schemaSql);
  if (!database.initialize())
    qFatal("Cannot initialize database");
  PhotoStore photoStore(dataDir);
  ClothingRepository clothingRepository(database, photoStore);
  HistoryRepository historyRepository(database);

  MlClient mlClient;
  Closet closet(clothingRepository, photoStore);
  WeatherService weatherService(database);
  CalendarService calendarService(database, mlClient);
  RecommendationService recommendationService(
      database, mlClient, clothingRepository, weatherService, calendarService);
  CalendarPageModel calendarPageModel(calendarService, recommendationService,
                                      mlClient);
  RecommendationPageModel recommendationPageModel(recommendationService,
                                                  clothingRepository);
  HistoryPageModel historyPageModel(clothingRepository, historyRepository);
  WeatherSettingsModel weatherSettingsModel(weatherService);
  ImageRecognitionHelper imageRecognitionHelper(mlClient);
  CameraHelper cameraHelper(photoStore);

  // New or removed clothes change which outfits are possible.
  QObject::connect(&closet, &Closet::itemsListChanged, &recommendationPageModel,
                   &RecommendationPageModel::refresh);
  // A new city means new weather, so outfits are redone.
  QObject::connect(&weatherSettingsModel, &WeatherSettingsModel::cityChanged,
                   &recommendationPageModel,
                   &RecommendationPageModel::refreshWeather);
  // A new or removed calendar changes today's event, so outfits are redone.
  QObject::connect(&calendarPageModel, &CalendarPageModel::calendarChanged,
                   &recommendationPageModel, &RecommendationPageModel::refresh);

  QQuickStyle::setStyle("Basic");

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("appCloset", &closet);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
  engine.setInitialProperties(
      {{"closet", QVariant::fromValue(&closet)},
       {"recommendationPageModel",
        QVariant::fromValue(&recommendationPageModel)},
       {"calendarPageModel", QVariant::fromValue(&calendarPageModel)},
       {"historyPageModel", QVariant::fromValue(&historyPageModel)},
       {"imageRecognitionHelper", QVariant::fromValue(&imageRecognitionHelper)},
       {"cameraHelper", QVariant::fromValue(&cameraHelper)},
       {"weatherSettingsModel", QVariant::fromValue(&weatherSettingsModel)}});
  engine.loadFromModule("Vestir", "Main");

#ifdef Q_OS_ANDROID
  // Android 17 blocks private-network access (the dev ML server) until the user
  // allows it.
  QNativeInterface::QAndroidApplication::runOnAndroidMainThread([] {
    QJniObject activity = QNativeInterface::QAndroidApplication::context();
    if (!activity.isValid())
      return;
    QJniEnvironment env;
    jclass strCls = env->FindClass("java/lang/String");
    jobjectArray perms = env->NewObjectArray(1, strCls, nullptr);
    QJniObject perm = QJniObject::fromString(
        QStringLiteral("android.permission.ACCESS_LOCAL_NETWORK"));
    env->SetObjectArrayElement(perms, 0, perm.object());
    activity.callMethod<void>("requestPermissions", "([Ljava/lang/String;I)V",
                              perms, 1001);
  });
#endif

  return QGuiApplication::exec();
}
