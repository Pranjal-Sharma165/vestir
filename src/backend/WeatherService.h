#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include "Database.h"
#include "Errors.h"
#include "WeatherData.h"
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <optional>

struct Location {
  QString name;
  double latitude;
  double longitude;
};

struct CityResult {
  QString label; // for display, e.g. "Vancouver, British Columbia, Canada"
  QString name;  // the city name alone, e.g. "Vancouver"
  double latitude = 0;
  double longitude = 0;
};

class WeatherService : public QObject {
  Q_OBJECT
public:
  explicit WeatherService(Database &database, QObject *parent = nullptr);

  Location location() const;

  std::optional<WeatherData> cached() const; // most recent saved weather for the current location

  void setLocation(const CityResult &city);

  QList<CityResult> parseCities(const QByteArray &json) const;

  // Reads an Open-Meteo response. Returns nothing if it is malformed
  std::optional<WeatherData> parse(const QByteArray &json,
                                   const QString &locationName) const;

  bool save(const WeatherData &data);

  void fetch();

  void searchCity(const QString &text);

signals:
  void fetched(bool ok, const QString &error);
  void citiesFound(bool ok, const QString &error,
                   const QList<CityResult> &cities);

private:
  Database &m_database;
  QNetworkAccessManager m_network;
};

#endif // WEATHERSERVICE_H
