#include "MlClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QProcess>
#include <QTimer>

namespace {
constexpr int kTimeoutMs = 30000;
}

MlClient::MlClient(QObject *parent) : QObject(parent) {}

QString MlClient::scriptPath() {
  const QString envDir = qEnvironmentVariable("VESTIR_ML_DIR");
  if (!envDir.isEmpty()) {
    const QString p = QDir(envDir).filePath("recommend_cli.py");
    return QFileInfo::exists(p) ? p : QString();
  }
  QDir dir(QCoreApplication::applicationDirPath());
  do {
    const QString p = dir.filePath("python/recommend_cli.py");
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

  QString python = qEnvironmentVariable("VESTIR_PYTHON");
  if (python.isEmpty())
    python = "python";

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
