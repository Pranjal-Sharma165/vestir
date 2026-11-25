#ifndef RECOMMENDATIONPAGEMODEL_H
#define RECOMMENDATIONPAGEMODEL_H

#include "../../backend/ClothingRepository.h"
#include "../../backend/RecommendationService.h"
#include "OutfitRecommendation.h"
#include <QQmlEngine>

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
  Q_PROPERTY(QList<OutfitRecommendation> outfits READ outfits NOTIFY
                 outfitsListChanged)
  Q_PROPERTY(
      const OutfitRecommendation *current READ current NOTIFY currentChanged)
  Q_PROPERTY(bool noMore READ noMore NOTIFY currentChanged)
  Q_PROPERTY(bool chosen READ chosen NOTIFY currentChanged)
  Q_PROPERTY(bool closetEmpty READ closetEmpty NOTIFY currentChanged)
  Q_PROPERTY(QString error READ error NOTIFY currentChanged)

signals:
  /*!
   * Signal that the contents of the outfits list have changed.
   */
  void outfitsListChanged();

  /*! The outfit being shown, or the noMore/chosen/error state, has changed. */
  void currentChanged();

public:
  explicit RecommendationPageModel(RecommendationService &recommendationService,
                                   QObject *parent = nullptr)
      : QObject(parent), m_service(recommendationService) {
    m_service.start(callback());
  }

  /*!
   * Starts over with the current closet: forgets which outfits were shown
   * today, so they can come up again.
   */
  Q_INVOKABLE void refresh() { m_service.refresh(callback()); }

  /*! Downloads the weather again (new city or manual refresh). */
  Q_INVOKABLE void refreshWeather() { m_service.refreshWeather(callback()); }

  /*! Shows the outfits for one calendar event (a calendar_events id). */
  Q_INVOKABLE void showEvent(qint64 eventId) {
    m_service.setEvent(eventId);
    m_service.refresh(callback());
  }

  /*! Asks for a new set of outfits, skipping the ones already shown today. */
  Q_INVOKABLE void generateMore() { m_service.regenerate(callback()); }

  /*! "Not for me": shows the next outfit, asking for a new round at the end. */
  Q_INVOKABLE void notForMe() {
    if (current())
      RecommendationService::markOutfit(current()->qRecommendationId(), false);
    if (m_index + 1 < m_outfits.size()) {
      ++m_index;
      emit currentChanged();
    } else {
      generateMore();
    }
  }

  /*! "This one": keeps the current outfit as the final choice. */
  Q_INVOKABLE void choose() {
    if (current())
      RecommendationService::markOutfit(current()->qRecommendationId(), true);
    m_chosen = true;
    emit currentChanged();
  }

  QList<OutfitRecommendation> outfits() const { return m_outfits; }
  const OutfitRecommendation *current() const {
    return m_index < m_outfits.size() ? &m_outfits.at(m_index) : nullptr;
  }
  bool noMore() const {
    return m_loaded && m_outfits.isEmpty() && !m_closetEmpty &&
           m_error.isEmpty();
  }
  /*! True when there is nothing in the closet to build outfits from. */
  bool closetEmpty() const { return m_closetEmpty; }
  /*! Why the last request failed (e.g. ML server unreachable), or empty. */
  QString error() const { return m_error; }
  bool chosen() const { return m_chosen; }

private:
  RecommendationService::Callback callback() {
    return [this](bool ok, const std::vector<OutfitRecommendation> &outfits,
                  const QString &error) {
      m_error = ok ? QString() : error;
      if (!ok)
        qWarning() << "Recommendations failed:" << error;

      m_outfits.clear();
      if (ok) {
        for (auto &recommendation : outfits) {
          m_outfits.append(std::move(recommendation));
          m_outfits.last().setParent(this);
        }
      }
      m_closetEmpty =
          ok && outfits.empty() && ClothingRepository::listStored().empty();
      m_index = 0;
      m_chosen = !outfits.empty() && RecommendationService::isChosen(
                                         outfits.at(0).qRecommendationId());
      m_loaded = true;
      emit outfitsListChanged();
      emit currentChanged();
    };
  }

  RecommendationService &m_service;
  QList<OutfitRecommendation> m_outfits;
  int m_index = 0;
  bool m_chosen = false;
  bool m_loaded = false;
  bool m_closetEmpty = false;
  QString m_error;
};

#endif // RECOMMENDATIONPAGEMODEL_H
