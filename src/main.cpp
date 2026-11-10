#include "model/pages/CalendarPageModel.h"
#include "model/pages/ClosetPageModel.h"
#include "model/pages/RecommendationPageModel.h"
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "repository/Database.h" // create DB on first run

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
