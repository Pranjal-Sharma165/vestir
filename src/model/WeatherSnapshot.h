#ifndef WEATHERSNAPSHOT_H
#define WEATHERSNAPSHOT_H

#include <QObject>
#include <QQmlEngine>

#include "../backend/WeatherService.h"

class WeatherSnapshot : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(double const degrees_celcius READ degrees_celcius CONSTANT)
  Q_PROPERTY(QString cloudiness READ cloudiness CONSTANT)
  Q_PROPERTY(QString city READ city CONSTANT)
  Q_PROPERTY(
      double const min_daily_temp_celcius READ min_daily_temp_celcius CONSTANT)
  Q_PROPERTY(
      double const max_daily_temp_celcius READ max_daily_temp_celcius CONSTANT)
  Q_PROPERTY(double const rain_chance READ rain_chance CONSTANT)

signals:

public:
  explicit WeatherSnapshot(QObject *parent = nullptr);

  /*! Real weather from WeatherService (rain chance converted from 0-100). */
  explicit WeatherSnapshot(const WeatherData &data, QObject *parent = nullptr);

  double const degrees_celcius() const { return m_degrees_celcius; }
  QString cloudiness() const { return QString::fromStdString(m_cloudiness); }
  QString city() const { return QString::fromStdString(m_city); }
  double const min_daily_temp_celcius() const {
    return m_min_daily_temp_celcius;
  }
  double const max_daily_temp_celcius() const {
    return m_max_daily_temp_celcius;
  }
  double const rain_chance() const { return m_rain_chance; }

private:
  double m_degrees_celcius;
  std::string m_cloudiness;
  std::string m_city;
  double m_min_daily_temp_celcius;
  double m_max_daily_temp_celcius;
  // Chance of rain. Value from [0,1).
  double m_rain_chance;
};

#endif // WEATHERSNAPSHOT_H
