#ifndef WEATHERSERVICE_H
#define WEATHERSERVICE_H

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <optional>

/*! One day's weather for one location, as stored in weather_records. */
struct WeatherData {
  QString location;
  QString forecastTime; // the date, e.g. "2026-11-07"
  double temperatureC = 0;
  double minTempC = 0;
  double maxTempC = 0;
  double feelsLikeC = 0;
  QString cloudiness; // "Sunny", "Partly cloudy" or "Cloudy"
  // TODO: WeatherSnapshot (Isaac) uses rain chance on a 0-1 scale; this is
  // 0-100, which is what the ML recommender expects. Convert in one place.
  int weatherCode = 0;
  double uvIndex = 0;
  int precipitationProbability = 0; // 0-100
  double precipitationMm = 0;
  int humidity = 0;
  double windSpeedKmh = 0;
};

struct Location {
  QString name;
  double latitude;
  double longitude;
};

/*!
 * \brief Gets today's weather from Open-Meteo (free, no API key) and keeps the
 * latest result in the database so it still works offline.
 *
 * The location comes from app_settings (location_name, location_lat,
 * location_lon) and defaults to Vancouver until the user picks a city.
 *
 * TODO later:
 *  - Settings page: let the user search a city (Open-Meteo geocoding API) and
 *    store the three location_* keys.
 *  - Only today is fetched. For an event on another day, request more
 *    forecast_days and pick the matching date.
 *  - Call fetch() on app start / when the user asks for outfits, and skip it
 *    if cached() is already from today.
 *  - Android needs the INTERNET permission in the manifest.
 */
class WeatherService : public QObject {
  Q_OBJECT
public:
  explicit WeatherService(QObject *parent = nullptr);

  /*! The saved location, or Vancouver if none has been chosen. */
  static Location location();

  /*! The most recent saved weather for the current location, if any. */
  static std::optional<WeatherData> cached();

  /*! Reads an Open-Meteo response. Returns nothing if it is malformed. */
  static std::optional<WeatherData> parse(const QByteArray &json,
                                          const QString &locationName);

  /*! Inserts or updates the record for that location and day. */
  static bool save(const WeatherData &data);

  /*!
   * Requests fresh weather in the background and saves it. Emits fetched()
   * when done. On failure, cached() still returns the last saved weather.
   */
  void fetch();

signals:
  void fetched(bool ok, const QString &error);

private:
  QNetworkAccessManager m_network;
};

#endif // WEATHERSERVICE_H
