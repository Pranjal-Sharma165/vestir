#include "MlClient.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QBuffer>
#include <QFile>
#include <QJsonArray>
#include <QImage>
#include <QImageReader>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QTimer>

namespace {
constexpr int kTimeoutMs = 30000;
constexpr int kUploadMaxSide = 1024;
constexpr int kClassifyTimeoutMs = 180000; // first run downloads the model

// The ML server's address. Android cannot run Python, so it talks to the
// server on the PC (the emulator's alias for it is 10.0.2.2). Elsewhere the
// Python CLIs are used unless VESTIR_ML_URL is set.
QString serverUrl() {
  const QString url = qEnvironmentVariable("VESTIR_ML_URL");
  if (!url.isEmpty())
    return url;
#ifdef Q_OS_ANDROID
  return "http://10.0.2.2:8001";
#else
  return {};
#endif
}

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
  run("/vestir/recommend", "recommend_cli.py", request,
      [this](bool ok, const QJsonObject &r, const QString &e) {
        emit recommended(ok, r, e);
      });
}

void MlClient::eventTypes(const QStringList &titles) {
  run("/vestir/event_types", "event_types_cli.py",
      QJsonObject{{"titles", QJsonArray::fromStringList(titles)}},
      [this](bool ok, const QJsonObject &r, const QString &e) {
        emit eventTypesReady(ok, r, e);
      });
}

void MlClient::run(const QString &endpoint, const QString &scriptName,
                   const QJsonObject &request, Done done) {
  const QString server = serverUrl();
  if (!server.isEmpty()) {
    auto *manager = new QNetworkAccessManager(this);
    QNetworkRequest req(QUrl(server + endpoint));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setTransferTimeout(kTimeoutMs);
    QNetworkReply *reply = manager->post(
        req, QJsonDocument(request).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [reply, manager, done] {
      const QByteArray body = reply->readAll();
      const bool ok = reply->error() == QNetworkReply::NoError;
      const QString why = reply->errorString();
      reply->deleteLater();
      manager->deleteLater();
      const auto doc = QJsonDocument::fromJson(body);
      if (!ok || !doc.isObject()) {
        done(false, {}, "ML server: " + why);
        return;
      }
      done(true, doc.object(), {});
    });
    return;
  }

  const QString script = scriptPath(scriptName);
  if (script.isEmpty()) {
    QTimer::singleShot(0, this, [done, scriptName] {
      done(false, {}, scriptName + " not found");
    });
    return;
  }

  const QString python = pythonExe();

  auto *process = new QProcess(this);
  // The script imports "ai.recommendation2", so run it from its own folder.
  process->setWorkingDirectory(QFileInfo(script).absolutePath());

  connect(process, &QProcess::errorOccurred, this,
          [process, done](QProcess::ProcessError e) {
            if (e == QProcess::FailedToStart) {
              done(false, {}, "Could not start Python");
              process->deleteLater();
            }
          });
  connect(process, &QProcess::finished, this,
          [this, process, done] { finish(process, done); });

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

  const QString server = serverUrl();
  if (!server.isEmpty()) {
    // The model looks at a small crop anyway; a full phone photo is several
    // MB and stalls when uploaded through the emulator.
    QImageReader reader(imagePath);
    reader.setAutoTransform(true);
    QImage image = reader.read();
    if (image.isNull())
      return fail("Could not read the photo");
    if (qMax(image.width(), image.height()) > kUploadMaxSide)
      image = image.scaled(kUploadMaxSide, kUploadMaxSide, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);
    QByteArray jpeg;
    QBuffer buffer(&jpeg);
    buffer.open(QIODevice::WriteOnly);
    image.convertToFormat(QImage::Format_RGB888).save(&buffer, "JPEG", 90);

    QNetworkAccessManager manager;
    QHttpMultiPart multi(QHttpMultiPart::FormDataType);
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader,
                   R"(form-data; name="image"; filename="item.jpg")");
    part.setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");
    part.setBody(jpeg);
    multi.append(part);

    QNetworkRequest req(QUrl(server + "/analyze"));
    req.setTransferTimeout(kClassifyTimeoutMs);
    qInfo() << "MlClient: POST" << req.url() << jpeg.size() << "bytes";
    QElapsedTimer timer;
    timer.start();
    QNetworkReply *reply = manager.post(req, &multi);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const QByteArray body = reply->readAll();
    const bool ok = reply->error() == QNetworkReply::NoError;
    const QString why = reply->errorString();
    qInfo() << "MlClient: classify finished after" << timer.elapsed()
            << "ms, error" << int(reply->error()) << why;
    reply->deleteLater();

    const QJsonObject result = QJsonDocument::fromJson(body).object();
    if (!ok || result.isEmpty())
      return fail("ML server: " + why);
    return result;
  }

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

void MlClient::finish(QProcess *process, Done done) {
  const QByteArray out = process->readAllStandardOutput();
  const QString err = QString::fromUtf8(process->readAllStandardError()).trimmed();
  const bool exitedOk = process->exitStatus() == QProcess::NormalExit &&
                        process->exitCode() == 0;
  process->deleteLater();

  QJsonParseError parseError;
  const auto doc = QJsonDocument::fromJson(out, &parseError);
  if (!exitedOk || parseError.error != QJsonParseError::NoError ||
      !doc.isObject()) {
    done(false, {}, err.isEmpty() ? "Recommender failed" : err);
    return;
  }
  done(true, doc.object(), {});
}
