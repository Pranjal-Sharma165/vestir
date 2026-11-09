#ifndef RECOMMENDATIONPAGEMODEL_H
#define RECOMMENDATIONPAGEMODEL_H

#include "OutfitRecommendation.h"
#include "../../repository/RecommendationService.h"
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

  /*! Starts over with the current closet (call after the closet changes). */
  Q_INVOKABLE void refresh() { m_service.refresh(callback()); }

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

  QList<OutfitRecommendation *> outfits() const { return m_outfits; }
  OutfitRecommendation *current() const {
    return m_index < m_outfits.size() ? m_outfits.at(m_index) : nullptr;
  }
  bool noMore() const { return m_loaded && m_outfits.isEmpty(); }
  bool chosen() const { return m_chosen; }

private:
  RecommendationService::Callback callback() {
    return [this](bool ok, const QList<OutfitRecommendation *> &outfits,
                  const QString &error) {
      if (!ok) {
        qWarning() << "Recommendations failed:" << error;
        return;
      }
      m_outfits = outfits;
      m_index = 0;
      m_chosen = false;
      m_loaded = true;
      emit outfitsListChanged();
      emit currentChanged();
    };
  }

  RecommendationService m_service;
  QList<OutfitRecommendation *> m_outfits;
  int m_index = 0;
  bool m_chosen = false;
  bool m_loaded = false;
};

#endif // RECOMMENDATIONPAGEMODEL_H