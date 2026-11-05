#include "WeatherSnapshot.h"

WeatherSnapshot::WeatherSnapshot(QObject *parent) : QObject(parent) {}

void WeatherSnapshot::set(double degrees, const QString &cloudiness,
                          const QString &city, double minTemp, double maxTemp,
                          double rainChance) {
  m_degrees_celcius = degrees;
  m_cloudiness = cloudiness;
  m_city = city;
  m_min_daily_temp_celcius = minTemp;
  m_max_daily_temp_celcius = maxTemp;
  m_rain_chance = rainChance;
  emit changed();
}
