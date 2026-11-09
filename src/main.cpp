#include "model/pages/CalendarPageModel.h"
#include "model/pages/ClosetPageModel.h"
#include "model/pages/RecommendationPageModel.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "repository/Database.h" // create DB on first run

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  // create DB on first run
  if (!Database::initialize())
    qFatal("Cannot initialize database");

  Closet closet;
  CalendarPageModel calendarPageModel;
  RecommendationPageModel recommendationPageModel;

  // New or removed clothes change which outfits are possible.
  QObject::connect(&closet, &Closet::itemsListChanged, &recommendationPageModel,
                   &RecommendationPageModel::refresh);

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
       {"calendarPageModel", QVariant::fromValue(&calendarPageModel)}});
  engine.loadFromModule("Vestir", "Main");

  return QGuiApplication::exec();
}
