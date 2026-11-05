#include "OutfitRecommendation.h"
#include "../backend/Database.h"
#include "../backend/ImagePath.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>

OutfitRecommendation::OutfitRecommendation(QObject *parent)
    : QObject(parent), m_weather(new WeatherSnapshot(this)) {}

void OutfitRecommendation::setMessage(const QString &text) {
  m_message = text;
  emit changed();
}

void OutfitRecommendation::generate() {
  if (m_loading)
    return;

  const QString aiDir = QStringLiteral(VESTIR_AI_DIR);
  QString python = qEnvironmentVariable("VESTIR_PYTHON");
  if (python.isEmpty())
    python = aiDir + "/.venv/Scripts/python.exe";
  if (!QFile::exists(python)) {
    setMessage("Python not found: " + python);
    return;
  }

  m_loading = true;
  emit loadingChanged();
  setMessage("Finding outfits...");

  m_process = new QProcess(this);
  m_process->setWorkingDirectory(aiDir);
  connect(m_process, &QProcess::finished, this,
          [this](int code, QProcess::ExitStatus) { onFinished(code); });
  connect(m_process, &QProcess::errorOccurred, this,
          [this](QProcess::ProcessError e) {
            if (e == QProcess::FailedToStart)
              onFinished(-1);
          });
  m_process->start(python,
                   {aiDir + "/recommend_cli.py", "--db",
                    Database::connection().databaseName()});
}

void OutfitRecommendation::onFinished(int exitCode) {
  if (!m_process)
    return;
  const QByteArray out = m_process->readAllStandardOutput();
  const QString err = QString::fromUtf8(m_process->readAllStandardError());
  m_process->deleteLater();
  m_process = nullptr;
  m_loading = false;
  emit loadingChanged();

  const QJsonObject root = QJsonDocument::fromJson(out).object();
  if (exitCode != 0 || root.isEmpty()) {
    setMessage("Could not get recommendations. " + err.right(200));
    return;
  }

  const QJsonObject w = root["weather"].toObject();
  m_weather->set(w["temperature"].toDouble(), w["cloudiness"].toString(),
                 w["city"].toString(), w["min"].toDouble(), w["max"].toDouble(),
                 w["rain"].toDouble());

  const QJsonObject e = root["event"].toObject();
  m_eventTitle = e["name"].toString();
  QString f = e["formality"].toString();
  if (!f.isEmpty())
    f[0] = f[0].toUpper();
  m_formality = f.replace('-', ' ');

  m_outfits = root["outfits"].toArray();
  m_index = 0;
  m_chosen = false;
  if (m_outfits.isEmpty()) {
    m_images.clear();
    m_pieceList.clear();
    setMessage("No more new outfits today. Add more clothes to your closet.");
    return;
  }
  showCurrent();
}

void OutfitRecommendation::showCurrent() {
  const QJsonObject o = m_outfits.at(m_index).toObject();
  m_images.clear();
  QStringList names;
  for (const auto &v : o["items"].toArray()) {
    const QJsonObject it = v.toObject();
    m_images << imageUrlFromPath(it["image_path"].toString());
    names << it["color"].toString() + " " + it["sub_category"].toString();
  }
  m_pieceList = names.join(", ");
  m_title = QString("Outfit %1 of %2").arg(m_index + 1).arg(m_outfits.size());
  m_message.clear();
  emit changed();
}

void OutfitRecommendation::setStatus(int recommendationId,
                                     const QString &status) {
  QSqlQuery q(Database::connection());
  q.prepare("UPDATE outfit_recommendations SET status = ? WHERE "
            "recommendation_id = ?");
  q.addBindValue(status);
  q.addBindValue(recommendationId);
  q.exec();
}

void OutfitRecommendation::notForMe() {
  if (m_outfits.isEmpty() || m_loading)
    return;
  setStatus(m_outfits.at(m_index).toObject()["recommendation_id"].toInt(),
            "rejected");
  if (++m_index < m_outfits.size())
    showCurrent();
  else
    generate();
}

void OutfitRecommendation::wearToday() {
  if (m_outfits.isEmpty() || m_chosen)
    return;
  setStatus(m_outfits.at(m_index).toObject()["recommendation_id"].toInt(),
            "chosen");
  m_chosen = true;
  setMessage("Saved. Enjoy your outfit!");
}
