#include "MlClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QTimer>

namespace {
constexpr int kTimeoutMs = 30000;
constexpr int kClassifyTimeoutMs = 180000; // first run downloads the model

QString pythonExe() {
  const QString python = qEnvironmentVariable("VESTIR_PYTHON");
  return python.isEmpty() ? QString("python") : python;
}
}

MlClient::MlClient(QObject *parent) : QObject(parent) {}

QString MlClient::scriptPath(const QString &name) {
  const QString envDir = qEnvironmentVariable("VESTIR_ML_DIR");
  if (!envDir.isEmpty()) {
    const QString p = QDir(envDir).filePath(name);
    return QFileInfo::exists(p) ? p : QString();
  }
  QDir dir(QCoreApplication::applicationDirPath());
  do {
    const QString p = dir.filePath("python/" + name);
    if (QFileInfo::exists(p))
      return p;
  } while (dir.cdUp());
  return {};
}

void MlClient::recommend(const QJsonObject &request) {
  const QString script = scriptPath();
  if (script.isEmpty()) {
    QTimer::singleShot(0, this, [this] {
      emit recommended(false, {}, "recommend_cli.py not found");
    });
    return;
  }

  const QString python = pythonExe();

  auto *process = new QProcess(this);
  // The script imports "ai.recommendation2", so run it from its own folder.
  process->setWorkingDirectory(QFileInfo(script).absolutePath());

  connect(process, &QProcess::errorOccurred, this,
          [this, process](QProcess::ProcessError e) {
            if (e == QProcess::FailedToStart) {
              emit recommended(false, {}, "Could not start Python");
              process->deleteLater();
            }
          });
  connect(process, &QProcess::finished, this,
          [this, process] { finish(process); });

  QTimer::singleShot(kTimeoutMs, process, [process] {
    if (process->state() != QProcess::NotRunning)
      process->kill();
  });

  process->start(python, {script});
  process->write(QJsonDocument(request).toJson(QJsonDocument::Compact));
  process->closeWriteChannel();
}

QJsonObject MlClient::classifyPhoto(const QString &imagePath,
                                  QString *error) {
  auto fail = [error](const QString &message) {
    if (error)
      *error = message;
    return QJsonObject();
  };

  const QString script = scriptPath("classify_cli.py");
  if (script.isEmpty())
    return fail("classify_cli.py not found");

  QProcess process;
  process.setWorkingDirectory(QFileInfo(script).absolutePath());
  process.start(pythonExe(), {script, imagePath});
  if (!process.waitForStarted())
    return fail("Could not start Python");
  if (!process.waitForFinished(kClassifyTimeoutMs)) {
    process.kill();
    return fail("The classifier took too long");
  }

  // The script prints {"error": ...} itself when it fails.
  const auto doc = QJsonDocument::fromJson(process.readAllStandardOutput());
  const QJsonObject result = doc.object();
  if (process.exitCode() != 0 || result.isEmpty() || result.contains("error")) {
    const QString why = result.value("error").toString();
    return fail(why.isEmpty() ? "Classifier failed" : why);
  }
  return result;
}

void MlClient::finish(QProcess *process) {
  const QByteArray out = process->readAllStandardOutput();
  const QString err = QString::fromUtf8(process->readAllStandardError()).trimmed();
  const bool exitedOk = process->exitStatus() == QProcess::NormalExit &&
                        process->exitCode() == 0;
  process->deleteLater();

  QJsonParseError parseError;
  const auto doc = QJsonDocument::fromJson(out, &parseError);
  if (!exitedOk || parseError.error != QJsonParseError::NoError ||
      !doc.isObject()) {
    emit recommended(false, {}, err.isEmpty() ? "Recommender failed" : err);
    return;
  }
  emit recommended(true, doc.object(), {});
}
