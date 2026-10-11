#include "recommendation.h"

Recommendation::Recommendation(QObject *parent)
    : QObject(parent), m_userRating("Good") {}

void Recommendation::setUserRating(QString const &value) {
  if (m_userRating == value) {
    return;
  }
  m_userRating = value;
  emit userRatingChanged();
}