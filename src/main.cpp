#include "model/pages/ClosetPageModel.h"
#include "model/pages/RecommendationPageModel.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  Closet closet;
  RecommendationPageModel recommendationPageModel;

  QQuickStyle::setStyle("Basic");

  QQmlApplicationEngine engine;
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
