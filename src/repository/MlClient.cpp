#include "MlClient.h"

#include <QBuffer>
#include <QDebug>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QHttpMultiPart>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace {
constexpr int kTimeoutMs = 30000;
constexpr int kUploadMaxSide = 1024;
constexpr int kClassifyTimeoutMs = 180000; // first run downloads the model

// The ML server's address. Env VESTIR_ML_URL overrides it. The Android
// emulator reaches the PC through 10.0.2.2; on desktop the server is local.
QString serverUrl() {
  const QString url = qEnvironmentVariable("VESTIR_ML_URL");
  if (!url.isEmpty())
    return url;
#ifdef Q_OS_ANDROID
  return "http://10.0.2.2:8001";
#else
  return "http://127.0.0.1:8001";
#endif
}
}

MlClient::MlClient(QObject *parent) : QObject(parent) {}

void MlClient::recommend(const QJsonObject &request) {
  post("/vestir/recommend", request,
       [this](bool ok, const QJsonObject &r, const QString &e) {
         emit recommended(ok, r, e);
       });
}

void MlClient::eventTypes(const QStringList &titles) {
  post("/vestir/event_types",
       QJsonObject{{"titles", QJsonArray::fromStringList(titles)}},
       [this](bool ok, const QJsonObject &r, const QString &e) {
         emit eventTypesReady(ok, r, e);
       });
}

void MlClient::post(const QString &endpoint, const QJsonObject &request,
                    Done done) {
  QNetworkRequest req(QUrl(serverUrl() + endpoint));
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  req.setTransferTimeout(kTimeoutMs);
  QNetworkReply *reply = m_manager.post(
      req, QJsonDocument(request).toJson(QJsonDocument::Compact));
  connect(reply, &QNetworkReply::finished, this, [reply, done] {
    const QByteArray body = reply->readAll();
    const bool ok = reply->error() == QNetworkReply::NoError;
    const QString why = reply->errorString();
    reply->deleteLater();
    const auto doc = QJsonDocument::fromJson(body);
    if (!ok || !doc.isObject()) {
      done(false, {}, "ML server: " + why);
      return;
    }
    done(true, doc.object(), {});
  });
}

QJsonObject MlClient::classifyPhoto(const QString &imagePath,
                                    QString *error) {
  auto fail = [error](const QString &message) {
    if (error)
      *error = message;
    return QJsonObject();
  };

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

  // Runs on a worker thread, so it owns its manager and event loop.
  QNetworkAccessManager manager;
  QHttpMultiPart multi(QHttpMultiPart::FormDataType);
  QHttpPart part;
  part.setHeader(QNetworkRequest::ContentDispositionHeader,
                 R"(form-data; name="image"; filename="item.jpg")");
  part.setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");
  part.setBody(jpeg);
  multi.append(part);

  QNetworkRequest req(QUrl(serverUrl() + "/analyze"));
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
