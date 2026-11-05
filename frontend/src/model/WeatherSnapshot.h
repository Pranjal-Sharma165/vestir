#ifndef WEATHERSNAPSHOT_H
#define WEATHERSNAPSHOT_H

#include <QObject>
#include <QQmlEngine>

class WeatherSnapshot : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(double degrees_celcius READ degrees_celcius NOTIFY changed)
  Q_PROPERTY(QString cloudiness READ cloudiness NOTIFY changed)
  Q_PROPERTY(QString city READ city NOTIFY changed)
  Q_PROPERTY(double min_daily_temp_celcius READ min_daily_temp_celcius NOTIFY
                 changed)
  Q_PROPERTY(double max_daily_temp_celcius READ max_daily_temp_celcius NOTIFY
                 changed)
  Q_PROPERTY(double rain_chance READ rain_chance NOTIFY changed)

signals:
  void changed();

public:
  explicit WeatherSnapshot(QObject *parent = nullptr);

  double degrees_celcius() const { return m_degrees_celcius; }
  QString cloudiness() const { return m_cloudiness; }
  QString city() const { return m_city; }
  double min_daily_temp_celcius() const { return m_min_daily_temp_celcius; }
  double max_daily_temp_celcius() const { return m_max_daily_temp_celcius; }
  // Chance of rain, from 0 to 1.
  double rain_chance() const { return m_rain_chance; }

  void set(double degrees, const QString &cloudiness, const QString &city,
           double minTemp, double maxTemp, double rainChance);

private:
  double m_degrees_celcius = 15;
  QString m_cloudiness = "Unknown";
  QString m_city = "Vancouver";
  double m_min_daily_temp_celcius = 13;
  double m_max_daily_temp_celcius = 18;
  double m_rain_chance = 0.1;
};

#endif // WEATHERSNAPSHOT_H
