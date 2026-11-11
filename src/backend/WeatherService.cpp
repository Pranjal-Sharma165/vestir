#include "WeatherService.h"
#include "Database.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QPair>
#include <QNetworkRequest>
#include <QSqlError>
#include <QSqlQuery>
#include <QUrl>
#include <QUrlQuery>

namespace {

// Used until the user picks a city (setLocation).
const Location kDefaultLocation{"Vancouver", 49.2827, -123.1207};

QString cloudinessText(double cloudCover) {
  if (cloudCover < 25)
    return "Sunny";
  if (cloudCover < 60)
    return "Partly cloudy";
  return "Cloudy";
}

double firstOf(const QJsonObject &daily, const char *key) {
  return daily.value(QLatin1String(key)).toArray().at(0).toDouble();
}

} // namespace

WeatherService::WeatherService(QObject *parent) : QObject(parent) {
  m_network.setTransferTimeout(10000);
}

Location WeatherService::location() {
  const QString name = Database::setting("location_name");
  bool latOk = false, lonOk = false;
  const double lat = Database::setting("location_lat").toDouble(&latOk);
  const double lon = Database::setting("location_lon").toDouble(&lonOk);
  if (name.isEmpty() || !latOk || !lonOk)
    return kDefaultLocation;
  return {name, lat, lon};
}

void WeatherService::setLocation(const CityResult &city) {
  const QString name = city.name.trimmed();
  if (name.isEmpty() || city.latitude < -90 || city.latitude > 90 ||
      city.longitude < -180 || city.longitude > 180)
    throw InvalidInputError("Choose a city from the search results");

  QSqlDatabase db = Database::connection();
  if (!db.transaction())
    throw DatabaseError("Cannot save the location");
  const QList<QPair<QString, QString>> values{
      {"location_name", name},
      {"location_lat", QString::number(city.latitude, 'f', 4)},
      {"location_lon", QString::number(city.longitude, 'f', 4)}};
  for (const auto &kv : values) {
    if (!Database::setSetting(kv.first, kv.second)) {
      db.rollback();
      throw DatabaseError("Cannot save the location");
    }
  }
  if (!db.commit())
    throw DatabaseError("Cannot save the location");
}

QList<CityResult> WeatherService::parseCities(const QByteArray &json) {
  QList<CityResult> cities;
  const QJsonArray results =
      QJsonDocument::fromJson(json).object().value("results").toArray();
  for (const QJsonValue &v : results) {
    const QJsonObject o = v.toObject();
    CityResult c;
    c.name = o.value("name").toString();
    c.latitude = o.value("latitude").toDouble();
    c.longitude = o.value("longitude").toDouble();
    if (c.name.isEmpty() || !o.contains("latitude") ||
        !o.contains("longitude"))
      continue;
    QStringList parts{c.name};
    for (const char *key : {"admin1", "country"}) {
      const QString part = o.value(QLatin1String(key)).toString();
      if (!part.isEmpty() && !parts.contains(part))
        parts << part;
    }
    c.label = parts.join(", ");
    cities.append(c);
  }
  return cities;
}

std::optional<WeatherData>
WeatherService::parse(const QByteArray &json, const QString &locationName) {
  const QJsonObject root = QJsonDocument::fromJson(json).object();
  const QJsonObject current = root.value("current").toObject();
  const QJsonObject daily = root.value("daily").toObject();
  const QString time = current.value("time").toString();
  if (time.size() < 10 || !current.contains("temperature_2m") ||
      daily.value("temperature_2m_max").toArray().isEmpty())
    return std::nullopt;

  WeatherData d;
  d.location = locationName;
  d.forecastTime = time.left(10);
  d.temperatureC = current.value("temperature_2m").toDouble();
  d.feelsLikeC = current.value("apparent_temperature").toDouble();
  d.cloudiness = cloudinessText(current.value("cloud_cover").toDouble());
  d.weatherCode = current.value("weather_code").toInt();
  d.humidity = current.value("relative_humidity_2m").toInt();
  d.windSpeedKmh = current.value("wind_speed_10m").toDouble();
  d.precipitationMm = current.value("precipitation").toDouble();
  d.minTempC = firstOf(daily, "temperature_2m_min");
  d.maxTempC = firstOf(daily, "temperature_2m_max");
  d.uvIndex = firstOf(daily, "uv_index_max");
  d.precipitationProbability =
      qRound(firstOf(daily, "precipitation_probability_max"));
  return d;
}

bool WeatherService::save(const WeatherData &d) {
  QSqlQuery q(Database::connection());
  q.prepare(
      "INSERT INTO weather_records (location, forecast_time, temperature_c, "
      "min_temp_c, max_temp_c, feels_like_c, cloudiness, weather_code, "
      "uv_index, precipitation_probability, precipitation_mm, humidity, "
      "wind_speed_kmh) VALUES (:loc, :time, :temp, :min, :max, :feels, "
      ":cloud, :code, :uv, :prob, :mm, :hum, :wind) "
      "ON CONFLICT(location, forecast_time) DO UPDATE SET "
      "temperature_c = excluded.temperature_c, "
      "min_temp_c = excluded.min_temp_c, max_temp_c = excluded.max_temp_c, "
      "feels_like_c = excluded.feels_like_c, "
      "cloudiness = excluded.cloudiness, weather_code = excluded.weather_code, "
      "uv_index = excluded.uv_index, "
      "precipitation_probability = excluded.precipitation_probability, "
      "precipitation_mm = excluded.precipitation_mm, "
      "humidity = excluded.humidity, wind_speed_kmh = excluded.wind_speed_kmh, "
      "created_at = CURRENT_TIMESTAMP");
  q.bindValue(":loc", d.location);
  q.bindValue(":time", d.forecastTime);
  q.bindValue(":temp", d.temperatureC);
  q.bindValue(":min", d.minTempC);
  q.bindValue(":max", d.maxTempC);
  q.bindValue(":feels", d.feelsLikeC);
  q.bindValue(":cloud", d.cloudiness);
  q.bindValue(":code", d.weatherCode);
  q.bindValue(":uv", d.uvIndex);
  q.bindValue(":prob", d.precipitationProbability);
  q.bindValue(":mm", d.precipitationMm);
  q.bindValue(":hum", d.humidity);
  q.bindValue(":wind", d.windSpeedKmh);
  if (!q.exec()) {
    qWarning() << "Cannot save weather:" << q.lastError().text();
    return false;
  }
  return true;
}

std::optional<WeatherData> WeatherService::cached() {
  QSqlQuery q(Database::connection());
  q.prepare("SELECT location, forecast_time, temperature_c, min_temp_c, "
            "max_temp_c, feels_like_c, cloudiness, weather_code, uv_index, "
            "precipitation_probability, precipitation_mm, humidity, "
            "wind_speed_kmh FROM weather_records WHERE location = :loc "
            "ORDER BY forecast_time DESC LIMIT 1");
  q.bindValue(":loc", location().name);
  if (!q.exec() || !q.next())
    return std::nullopt;

  WeatherData d;
  d.location = q.value(0).toString();
  d.forecastTime = q.value(1).toString();
  d.temperatureC = q.value(2).toDouble();
  d.minTempC = q.value(3).toDouble();
  d.maxTempC = q.value(4).toDouble();
  d.feelsLikeC = q.value(5).toDouble();
  d.cloudiness = q.value(6).toString();
  d.weatherCode = q.value(7).toInt();
  d.uvIndex = q.value(8).toDouble();
  d.precipitationProbability = q.value(9).toInt();
  d.precipitationMm = q.value(10).toDouble();
  d.humidity = q.value(11).toInt();
  d.windSpeedKmh = q.value(12).toDouble();
  return d;
}

void WeatherService::searchCity(const QString &text) {
  const QString name = text.trimmed();
  if (name.size() < 2) {
    emit citiesFound(true, {}, {});
    return;
  }
  QUrl url("https://geocoding-api.open-meteo.com/v1/search");
  QUrlQuery query;
  query.addQueryItem("name", name);
  query.addQueryItem("count", "10");
  query.addQueryItem("language", "en");
  query.addQueryItem("format", "json");
  url.setQuery(query);

  QNetworkReply *reply = m_network.get(QNetworkRequest(url));
  connect(reply, &QNetworkReply::finished, this, [this, reply] {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      emit citiesFound(false, reply->errorString(), {});
      return;
    }
    emit citiesFound(true, {}, parseCities(reply->readAll()));
  });
}

void WeatherService::fetch() {
  const Location where = location();

  QUrl url("https://api.open-meteo.com/v1/forecast");
  QUrlQuery query;
  query.addQueryItem("latitude", QString::number(where.latitude, 'f', 4));
  query.addQueryItem("longitude", QString::number(where.longitude, 'f', 4));
  query.addQueryItem(
      "current", "temperature_2m,apparent_temperature,weather_code,cloud_cover,"
                 "relative_humidity_2m,wind_speed_10m,precipitation");
  query.addQueryItem("daily", "temperature_2m_max,temperature_2m_min,"
                              "precipitation_probability_max,uv_index_max");
  query.addQueryItem("timezone", "auto");
  // Only today for now; raise this if events on later days need weather.
  query.addQueryItem("forecast_days", "1");
  url.setQuery(query);

  QNetworkReply *reply = m_network.get(QNetworkRequest(url));
  connect(reply, &QNetworkReply::finished, this, [this, reply, where] {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      emit fetched(false, reply->errorString());
      return;
    }
    const auto data = parse(reply->readAll(), where.name);
    if (!data) {
      emit fetched(false, "Unexpected weather response");
      return;
    }
    if (!save(*data)) {
      emit fetched(false, "Cannot save weather");
      return;
    }
    emit fetched(true, {});
  });
}
