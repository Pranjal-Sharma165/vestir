#ifndef WEATHERDATA_H
#define WEATHERDATA_H

#include <QString>

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

#endif // WEATHERDATA_H
