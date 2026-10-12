#ifndef OUTFITRECOMMENDATION_H
#define OUTFITRECOMMENDATION_H

#include "OutfitFormality.h"
#include "WeatherSnapshot.h"
#include <QObject>
#include <QtQml/qqmlregistration.h>

class OutfitRecommendation : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(int pieceCount READ pieceCount CONSTANT)
  Q_PROPERTY(QString pieceList READ pieceList CONSTANT)
  Q_PROPERTY(QString title READ title CONSTANT)
  Q_PROPERTY(QString eventTitle READ eventTitle CONSTANT)
  Q_PROPERTY(QString outfitFormality READ outfitFormality CONSTANT)
  Q_PROPERTY(WeatherSnapshot *weather READ weather CONSTANT)

signals:

public:
  explicit OutfitRecommendation(QObject *parent = nullptr);

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

  WeatherSnapshot *weather() const { return m_weather; }

private:
  std::vector<std::string> m_clothingPieces;
  std::string m_title;
  std::string m_eventTitle;
  OutfitFormality m_formality;
  WeatherSnapshot *m_weather;
};

#endif // OUTFITRECOMMENDATION_H
