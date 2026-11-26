#include "TestHelpers.h"
#include "WeatherService.h"

#include <QCoreApplication>
#include <iostream>

static const char *kForecast = R"({
  "current": {
    "time": "2026-01-15T10:00",
    "temperature_2m": 5.5,
    "apparent_temperature": 2.1,
    "weather_code": 61,
    "cloud_cover": 80,
    "relative_humidity_2m": 70,
    "wind_speed_10m": 12.5,
    "precipitation": 0.4
  },
  "daily": {
    "temperature_2m_max": [8.0],
    "temperature_2m_min": [1.0],
    "precipitation_probability_max": [60],
    "uv_index_max": [1.5]
  }
})";

static const char *kCities = R"({
  "results": [
    {"name": "Vancouver", "latitude": 49.28, "longitude": -123.12,
     "admin1": "British Columbia", "country": "Canada"},
    {"name": "Paris", "latitude": 48.85, "longitude": 2.35,
     "admin1": "Paris", "country": "France"},
    {"name": "", "latitude": 1.0, "longitude": 1.0},
    {"name": "NoCoordinates"}
  ]
})";

static CityResult city(const QString &name, double lat, double lon) {
  CityResult c;
  c.name = name;
  c.label = name;
  c.latitude = lat;
  c.longitude = lon;
  return c;
}

template <typename E, typename F> static bool throwsError(F &&fn) {
  try {
    fn();
  } catch (const E &) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

static void testParseForecast() {
  Database database(":memory:", readSchema());
  database.initialize();
  WeatherService weather(database);

  const auto data = weather.parse(kForecast, "Vancouver");
  check(data.has_value(), "parse accepts a valid response");
  if (!data)
    return;
  check(data->location == "Vancouver", "parse keeps the location name");
  check(data->forecastTime == "2026-01-15", "parse keeps only the date");
  check(data->temperatureC == 5.5, "parse reads the temperature");
  check(data->feelsLikeC == 2.1, "parse reads feels like");
  check(data->minTempC == 1.0 && data->maxTempC == 8.0,
        "parse reads min and max");
  check(data->cloudiness == "Cloudy", "80 percent cloud cover is Cloudy");
  check(data->weatherCode == 61, "parse reads the weather code");
  check(data->humidity == 70, "parse reads humidity");
  check(data->precipitationProbability == 60, "parse reads rain chance");
  check(data->uvIndex == 1.5, "parse reads the UV index");
}

static void testParseRejectsBadJson() {
  Database database(":memory:", readSchema());
  database.initialize();
  WeatherService weather(database);

  check(!weather.parse("not json", "X").has_value(), "garbage is rejected");
  check(!weather.parse("{}", "X").has_value(), "empty object is rejected");
  check(!weather.parse(R"({"current":{"time":"2026-01-15T10:00"}})", "X")
             .has_value(),
        "response without daily data is rejected");
}

static void testParseCities() {
  Database database(":memory:", readSchema());
  database.initialize();
  WeatherService weather(database);

  const auto cities = weather.parseCities(kCities);
  check(cities.size() == 2, "cities without a name or coordinates are skipped");
  if (cities.size() != 2)
    return;
  check(cities[0].label == "Vancouver, British Columbia, Canada",
        "label has city, region and country");
  check(cities[1].label == "Paris, France",
        "label does not repeat the same name");
  check(cities[0].latitude == 49.28 && cities[0].longitude == -123.12,
        "coordinates are read");
  check(weather.parseCities("garbage").isEmpty(), "bad JSON gives no cities");
}

static void testLocation() {
  Database database(":memory:", readSchema());
  database.initialize();
  WeatherService weather(database);

  check(weather.location().name == "Vancouver", "default location is set");

  weather.setLocation(city("Calgary", 51.05, -114.07));
  const Location saved = weather.location();
  check(saved.name == "Calgary", "setLocation saves the name");
  check(saved.latitude == 51.05 && saved.longitude == -114.07,
        "setLocation saves the coordinates");

  check(throwsError<InvalidInputError>(
            [&] { weather.setLocation(city("  ", 10, 10)); }),
        "empty city name is rejected");
  check(throwsError<InvalidInputError>(
            [&] { weather.setLocation(city("Nowhere", 120, 10)); }),
        "latitude out of range is rejected");
  check(weather.location().name == "Calgary",
        "rejected city does not change the location");
}

static void testSaveAndCached() {
  Database database(":memory:", readSchema());
  database.initialize();
  WeatherService weather(database);

  check(!weather.cached().has_value(), "nothing cached at first");

  auto data = weather.parse(kForecast, "Vancouver");
  check(weather.save(*data), "save works");
  auto cached = weather.cached();
  check(cached.has_value(), "saved weather is cached");
  if (!cached)
    return;
  check(cached->temperatureC == 5.5 && cached->maxTempC == 8.0,
        "cached values match what was saved");
  check(cached->cloudiness == "Cloudy", "cached cloudiness matches");

  data->temperatureC = 7.0;
  weather.save(*data);
  check(weather.cached()->temperatureC == 7.0,
        "saving the same day again updates it");

  weather.setLocation(city("Calgary", 51.05, -114.07));
  check(!weather.cached().has_value(),
        "weather for another city is not returned");
}

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  testParseForecast();
  testParseRejectsBadJson();
  testParseCities();
  testLocation();
  testSaveAndCached();
  std::cout << (failures == 0 ? "All tests passed\n" : "Some tests failed\n");
  return failures == 0 ? 0 : 1;
}
