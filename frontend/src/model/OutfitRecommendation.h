#ifndef OUTFITRECOMMENDATION_H
#define OUTFITRECOMMENDATION_H

#include "WeatherSnapshot.h"
#include <QJsonArray>
#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

/*!
 * \brief The outfit currently shown on the Home page.
 *
 * generate() runs ai-models/recommend_cli.py (weather + today's calendar event
 * + ranking), which returns the next 5 outfits and saves them to the DB.
 * notForMe() moves to the next of the 5 and fetches 5 new ones when they run
 * out. wearToday() marks the outfit as chosen.
 */
class OutfitRecommendation : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(int pieceCount READ pieceCount NOTIFY changed)
  Q_PROPERTY(QString pieceList READ pieceList NOTIFY changed)
  Q_PROPERTY(QList<QUrl> images READ images NOTIFY changed)
  Q_PROPERTY(QString title READ title NOTIFY changed)
  Q_PROPERTY(QString eventTitle READ eventTitle NOTIFY changed)
  Q_PROPERTY(QString outfitFormality READ outfitFormality NOTIFY changed)
  Q_PROPERTY(WeatherSnapshot *weather READ weather CONSTANT)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
  Q_PROPERTY(QString message READ message NOTIFY changed)

signals:
  void changed();
  void loadingChanged();

public:
  explicit OutfitRecommendation(QObject *parent = nullptr);

  int pieceCount() const { return m_images.size(); }
  QString pieceList() const { return m_pieceList; }
  QList<QUrl> images() const { return m_images; }
  QString title() const { return m_title; }
  QString eventTitle() const { return m_eventTitle; }
  QString outfitFormality() const { return m_formality; }
  WeatherSnapshot *weather() const { return m_weather; }
  bool loading() const { return m_loading; }
  QString message() const { return m_message; }

  Q_INVOKABLE void generate();
  Q_INVOKABLE void notForMe();
  Q_INVOKABLE void wearToday();

private:
  void onFinished(int exitCode);
  void showCurrent();
  void setStatus(int recommendationId, const QString &status);
  void setMessage(const QString &text);

  QProcess *m_process = nullptr;
  QJsonArray m_outfits;
  int m_index = 0;
  bool m_loading = false;
  bool m_chosen = false;

  QString m_title = "Your outfit for the day";
  QString m_eventTitle;
  QString m_formality;
  QString m_pieceList;
  QString m_message;
  QList<QUrl> m_images;
  WeatherSnapshot *m_weather;
};

#endif // OUTFITRECOMMENDATION_H
