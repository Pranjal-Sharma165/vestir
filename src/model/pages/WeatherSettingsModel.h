#ifndef WEATHERSETTINGSMODEL_H
#define WEATHERSETTINGSMODEL_H

#include "../../backend/Errors.h"
#include "../../backend/WeatherService.h"
#include <QQmlEngine>
#include <QRegularExpression>
#include <QStringList>

/*!
 * \brief Settings > Weather city. search() looks cities up by name; choose()
 * saves one of the results as the weather location.
 */
class WeatherSettingsModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")
  Q_PROPERTY(QString city READ city NOTIFY cityChanged)
  Q_PROPERTY(QStringList results READ results NOTIFY resultsChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString status READ status NOTIFY statusChanged)

signals:
  void cityChanged();
  void resultsChanged();
  void busyChanged();
  void statusChanged();

public:
  explicit WeatherSettingsModel(QObject *parent = nullptr) : QObject(parent) {
    connect(&m_service, &WeatherService::citiesFound, this,
            [this](bool ok, const QString &error,
                   const QList<CityResult> &cities) {
              // The API matches the city name only, so "bangkok thailand"
              // finds nothing: retry with the name and filter by the rest.
              if (ok && cities.isEmpty() && !m_retried) {
                const QString name = firstPart(m_query);
                if (name != m_query && name.size() >= 2) {
                  m_retried = true;
                  m_service.searchCity(name);
                  return;
                }
              }
              m_busy = false;
              emit busyChanged();
              m_cities.clear();
              m_results.clear();
              const QString rest = m_retried ? m_query.mid(firstPart(m_query).size()).toLower() : QString();
              const QStringList words =
                  rest.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts);
              for (const CityResult &c : cities) {
                bool match = true;
                for (const QString &w : words)
                  match = match && c.label.toLower().contains(w);
                if (match && m_cities.size() < 5) {
                  m_cities << c;
                  m_results << c.label;
                }
              }
              const bool noneFound = m_cities.isEmpty();
              emit resultsChanged();
              if (!ok)
                setStatus(tr("Can't search right now. Check your connection."));
              else if (noneFound)
                setStatus(tr("No city found."));
              else
                setStatus({});
              Q_UNUSED(error)
            });
  }

  QString city() const { return WeatherService::location().name; }
  QStringList results() const { return m_results; }
  bool busy() const { return m_busy; }
  QString status() const { return m_status; }

  Q_INVOKABLE void search(const QString &text) {
    if (text.trimmed().size() < 2) {
      clearResults();
      return;
    }
    m_busy = true;
    m_retried = false;
    m_query = text.trimmed();
    emit busyChanged();
    m_service.searchCity(m_query);
  }

  /*! Saves the result at \a index; emits cityChanged() when it worked. */
  Q_INVOKABLE void choose(int index) {
    if (index < 0 || index >= m_cities.size())
      return;
    try {
      WeatherService::setLocation(m_cities.at(index));
    } catch (const VestirError &e) {
      setStatus(e.message());
      return;
    }
    clearResults();
    emit cityChanged();
  }

  Q_INVOKABLE void clearResults() {
    m_cities.clear();
    m_results.clear();
    emit resultsChanged();
    setStatus({});
  }

private:
  void setStatus(const QString &s) {
    if (m_status == s)
      return;
    m_status = s;
    emit statusChanged();
  }

  static QString firstPart(const QString &text) {
    const int cut = text.indexOf(QRegularExpression("[,\\s]"));
    return cut < 0 ? text : text.left(cut);
  }

  WeatherService m_service;
  QString m_query;
  bool m_retried = false;
  QList<CityResult> m_cities;
  QStringList m_results;
  QString m_status;
  bool m_busy = false;
};

#endif // WEATHERSETTINGSMODEL_H
