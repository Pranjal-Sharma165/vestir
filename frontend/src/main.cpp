#include "backend/Database.h"
#include "model/Closet.h"
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStandardPaths>
#include <QtLogging>

// Returns the path of the writable database, creating it from the bundled
// template on first run. Set VESTIR_DB to use a specific file instead.
static QString prepareDatabase() {
  const QString override = qEnvironmentVariable("VESTIR_DB");
  if (!override.isEmpty())
    return override;

  const QString dir =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir().mkpath(dir);
  const QString path = dir + "/vestir.db";

  if (!QFile::exists(path)) {
    const QString templatePath = QStringLiteral(VESTIR_TEMPLATE_DB);
    if (!QFile::copy(templatePath, path))
      qWarning() << "Could not copy database template" << templatePath;
    else
      QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
  }
  return path;
}

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);
  QCoreApplication::setOrganizationName("Vestir");
  QCoreApplication::setApplicationName("Vestir");

  if (!Database::open(prepareDatabase()))
    qWarning() << "Running without a database; the closet will be empty.";

  Closet closet;

  QQuickStyle::setStyle("Basic");

  QQmlApplicationEngine engine;
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
  engine.setInitialProperties({{"closet", QVariant::fromValue(&closet)}});
  engine.loadFromModule("Vestir", "Main");

  return QGuiApplication::exec();
}
