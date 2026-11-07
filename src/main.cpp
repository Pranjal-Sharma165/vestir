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
  RecommendationPageModel recommendationPageModel;

  QQuickStyle::setStyle("Basic");

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("appCloset", &closet);
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
  engine.setInitialProperties(
      {{"closet", QVariant::fromValue(&closet)},
       {"recommendationPageModel",
        QVariant::fromValue(&recommendationPageModel)}});
  engine.loadFromModule("Vestir", "Main");

  return QGuiApplication::exec();
}
