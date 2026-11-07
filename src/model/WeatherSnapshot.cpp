#include "WeatherSnapshot.h"

WeatherSnapshot::WeatherSnapshot(QObject *parent)
    : QObject(parent), m_degrees_celcius(15), m_cloudiness("Sunny"),
      m_city("Vancouver"), m_min_daily_temp_celcius(13),
      m_max_daily_temp_celcius(18), m_rain_chance(0.1) {}

WeatherSnapshot::WeatherSnapshot(const WeatherData &data, QObject *parent)
    : QObject(parent), m_degrees_celcius(data.temperatureC),
      m_cloudiness(data.cloudiness.toStdString()),
      m_city(data.location.toStdString()),
      m_min_daily_temp_celcius(data.minTempC),
      m_max_daily_temp_celcius(data.maxTempC),
      m_rain_chance(data.precipitationProbability / 100.0) {}
