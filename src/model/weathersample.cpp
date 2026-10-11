#include "weathersample.h"

WeatherSample::WeatherSample(QObject *parent)
    : QObject(parent), m_degrees_celcius(15), m_cloudiness("Sunny"),
      m_city("Vancouver"), m_min_daily_temp_celcius(13),
      m_max_daily_temp_celcius(18), m_rain_chance(0.1) {}
