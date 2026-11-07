#ifndef RECOMMENDATIONPAGEMODEL_H
#define RECOMMENDATIONPAGEMODEL_H

#include "ClothingCategory.h"
#include "OutfitFormality.h"
#include "OutfitRecommendation.h"
#include "../../repository/RecommendationService.h"
#include <QQmlEngine>
#include <unordered_map>

/*!
 * \brief The RecommendationPageModel class is the data source for the outfit
 * recommendations page.
 *
 * The outfits come from RecommendationService (closet + weather + calendar +
 * ML). It starts when the app starts; call generateMore() for a new round.
 */
class RecommendationPageModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")
  Q_PROPERTY(QList<OutfitRecommendation *> outfits READ outfits NOTIFY
                 outfitsListChanged)
  Q_PROPERTY(OutfitRecommendation *current READ current NOTIFY currentChanged)
  Q_PROPERTY(bool noMore READ noMore NOTIFY currentChanged)
  Q_PROPERTY(bool chosen READ chosen NOTIFY currentChanged)

signals:
  /*!
   * Signal that the contents of the outfits list have changed.
   */
  void outfitsListChanged();

  /*! The outfit being shown, or the noMore/chosen state, has changed. */
  void currentChanged();

public:
  explicit RecommendationPageModel(QObject *parent = nullptr)
      : QObject(parent), m_service(this) {
    m_service.start(callback());
  }

  /*! Asks for a new set of outfits, skipping the ones already shown today. */
  Q_INVOKABLE void generateMore() { m_service.regenerate(callback()); }

  /*! "Not for me": shows the next outfit, asking for a new round at the end. */
  Q_INVOKABLE void notForMe() {
    if (m_index + 1 < m_outfits.size()) {
      ++m_index;
      emit currentChanged();
    } else {
      generateMore();
    }
  }

  /*! "This one": keeps the current outfit as the final choice. */
  Q_INVOKABLE void choose() {
    m_chosen = true;
    emit currentChanged();
  }

  QList<OutfitRecommendation *> outfits() const { return m_outfits; }
  OutfitRecommendation *current() const {
    return m_index < m_outfits.size() ? m_outfits.at(m_index) : nullptr;
  }
  bool noMore() const { return m_loaded && m_outfits.isEmpty(); }
  bool chosen() const { return m_chosen; }

private:
  RecommendationService::Callback callback() {
    return [this](bool ok, RecommendationResult &&result,
                  const QString &error) {
      if (!ok) {
        qWarning() << "Recommendations failed:" << error;
        return;
      }
      show(std::move(result));
    };
  }

  static OutfitFormality::Value formalityFor(const QString &eventType) {
    if (eventType == "work" || eventType == "school")
      return OutfitFormality::Value::SmartCasual;
    if (eventType == "party")
      return OutfitFormality::Value::Formal;
    if (eventType == "gym")
      return OutfitFormality::Value::Sporty;
    return OutfitFormality::Value::Casual;
  }

  void show(RecommendationResult &&result) {
    for (QObject *old : std::as_const(m_owned))
      old->deleteLater();
    m_owned.clear();
    m_outfits.clear();
    m_index = 0;
    m_chosen = false;
    m_loaded = true;

    std::unordered_map<qint64, ClothingItem *> itemsById;
    for (auto &stored : result.closet) {
      ClothingItem *item = stored.item.release();
      item->setParent(this);
      m_owned.append(item);
      itemsById[stored.id] = item;
    }

    const OutfitFormality::Value formality = formalityFor(result.eventType);

    for (const auto &outfit : result.outfits) {
      QList<ClothingItem *> pieces;
      for (qint64 id : outfit.itemIds)
        if (auto it = itemsById.find(id); it != itemsById.end())
          pieces.append(it->second);

      WeatherSnapshot *weather = nullptr;
      if (result.weather) {
        weather = new WeatherSnapshot(*result.weather, this);
        m_owned.append(weather);
      }
      CalendarEvent *event = nullptr;
      if (result.event) {
        event = new CalendarEvent(
            result.event->name.toStdString(),
            result.event->location.toStdString(),
            result.event->start.toSecsSinceEpoch(),
            result.event->end.toSecsSinceEpoch(), formality, -1, this);
        m_owned.append(event);
      }

      auto *rec = new OutfitRecommendation(
          pieces, "Outfit " + std::to_string(outfit.rank), formality, event,
          weather, this);
      m_owned.append(rec);
      m_outfits.append(rec);
    }
    emit outfitsListChanged();
    emit currentChanged();
  }

  RecommendationService m_service;
  QList<OutfitRecommendation *> m_outfits;
  int m_index = 0;
  bool m_chosen = false;
  bool m_loaded = false;
  QList<QObject *> m_owned; // everything created for the current round
};

#endif // RECOMMENDATIONPAGEMODEL_H