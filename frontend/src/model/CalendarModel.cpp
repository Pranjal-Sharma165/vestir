#include "CalendarModel.h"
#include "../backend/Database.h"
#include <QDate>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSqlQuery>

CalendarModel::CalendarModel(QObject *parent) : QObject(parent) { reload(); }

void CalendarModel::reload() {
  QSqlDatabase db = Database::connection();
  if (!db.isOpen())
    return;

  QSqlQuery q(db);
  q.exec("CREATE TABLE IF NOT EXISTS app_settings (key TEXT PRIMARY KEY, "
         "value TEXT)");
  if (q.exec("SELECT value FROM app_settings WHERE key='ics_url'") && q.next())
    m_icsUrl = q.value(0).toString();

  m_events.clear();
  q.prepare("SELECT event_id, event_name, start_time, "
            "COALESCE(dress_code, inferred_dress_code, 'casual'), "
            "calendar_source FROM calendar_events "
            "WHERE substr(start_time,1,10) >= ? AND substr(start_time,1,10) <= ? "
            "ORDER BY start_time");
  const QDate today = QDate::currentDate();
  q.addBindValue(today.toString(Qt::ISODate));
  q.addBindValue(today.addDays(7).toString(Qt::ISODate));
  if (q.exec()) {
    while (q.next()) {
      const QDateTime start =
          QDateTime::fromString(q.value(2).toString(), "yyyy-MM-ddTHH:mm");
      QVariantMap e;
      e["id"] = q.value(0);
      e["name"] = q.value(1);
      e["when"] = start.toString("ddd MMM d, h:mm AP");
      e["formality"] = q.value(3).toString().replace('-', ' ');
      e["source"] = q.value(4);
      m_events << e;
    }
  }
  emit changed();
}

void CalendarModel::sync(const QString &url) {
  if (m_loading)
    return;

  const QString aiDir = QStringLiteral(VESTIR_AI_DIR);
  QString python = qEnvironmentVariable("VESTIR_PYTHON");
  if (python.isEmpty())
    python = aiDir + "/.venv/Scripts/python.exe";
  if (!QFile::exists(python)) {
    m_message = "Python not found: " + python;
    emit changed();
    return;
  }

  QStringList args{aiDir + "/calendar_sync.py", "--db",
                   Database::connection().databaseName()};
  if (!url.trimmed().isEmpty())
    args << "--url" << url.trimmed();

  m_loading = true;
  m_message = "Syncing...";
  emit loadingChanged();
  emit changed();

  auto *proc = new QProcess(this);
  proc->setWorkingDirectory(aiDir);
  connect(proc, &QProcess::errorOccurred, this, [this, proc](auto err) {
    if (err != QProcess::FailedToStart)
      return;
    m_loading = false;
    m_message = "Could not start Python.";
    emit loadingChanged();
    emit changed();
    proc->deleteLater();
  });
  connect(proc, &QProcess::finished, this, [this, proc] {
    const QJsonObject r =
        QJsonDocument::fromJson(proc->readAllStandardOutput()).object();
    m_loading = false;
    emit loadingChanged();
    if (r.contains("imported"))
      m_message = QString("Synced %1 events.").arg(r["imported"].toInt());
    else
      m_message = r["error"].toString("Sync failed.");
    reload();
    proc->deleteLater();
  });
  proc->start(python, args);
}

bool CalendarModel::addEvent(const QString &name, const QString &date,
                             const QString &time, const QString &formality) {
  const QDateTime start =
      QDateTime::fromString(date + "T" + time, "yyyy-MM-ddTHH:mm");
  if (name.trimmed().isEmpty() || !start.isValid()) {
    m_message = "Enter a name, a date (yyyy-mm-dd) and a time (hh:mm).";
    emit changed();
    return false;
  }
  QSqlQuery q(Database::connection());
  q.prepare("INSERT INTO calendar_events (event_name, dress_code, start_time, "
            "calendar_source) VALUES (?, ?, ?, 'manual')");
  q.addBindValue(name.trimmed());
  q.addBindValue(formality);
  q.addBindValue(start.toString("yyyy-MM-ddTHH:mm"));
  const bool ok = q.exec();
  m_message = ok ? "Event added." : "Could not add event.";
  reload();
  return ok;
}

void CalendarModel::removeEvent(int id) {
  QSqlQuery q(Database::connection());
  q.prepare("DELETE FROM calendar_events WHERE event_id = ?");
  q.addBindValue(id);
  q.exec();
  reload();
}
