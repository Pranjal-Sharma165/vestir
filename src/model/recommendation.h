#ifndef RECOMMENDATION_H
#define RECOMMENDATION_H

#include "OutfitFormality.h"
#include "weathersample.h"
#include <QObject>
#include <QtQml/qqmlregistration.h>

class Recommendation : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(int pieceCount READ pieceCount CONSTANT)
  Q_PROPERTY(QString pieceList READ pieceList CONSTANT)
  Q_PROPERTY(QString title READ title CONSTANT)
  Q_PROPERTY(QString eventTitle READ eventTitle CONSTANT)
  Q_PROPERTY(QString outfitFormality READ outfitFormality CONSTANT)
  Q_PROPERTY(WeatherSample *weather READ weather CONSTANT)

signals:

public:
  explicit Recommendation(QObject *parent = nullptr);

  int const pieceCount() const { return m_clothingPieces.size(); }

  QString title() const { return QString::fromStdString(m_title); }

  QString eventTitle() const { return QString::fromStdString(m_eventTitle); }

  QString pieceList() const {
    QStringList list;
    for (const auto &piece : m_clothingPieces)
      list << QString::fromStdString(piece);
    return list.join(", ");
  }

  QString outfitFormality() const {
    switch (m_formality) {
    case OutfitFormality::Casual:
      return "Casual";
    case OutfitFormality::SmartCasual:
      return "Smart casual";
    case OutfitFormality::Formal:
      return "Formal";
    }
  }

  WeatherSample *weather() const { return m_weather; }

private:
  std::vector<std::string> m_clothingPieces;
  std::string m_title;
  std::string m_eventTitle;
  OutfitFormality m_formality;
  WeatherSample *m_weather;
};

#endif // RECOMMENDATION_H
