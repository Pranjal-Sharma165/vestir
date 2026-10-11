#ifndef RECOMMENDATION_H
#define RECOMMENDATION_H

#include <QObject>
#include <QtQml/qqmlregistration.h>

class Recommendation : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QString userRating READ userRating WRITE setUserRating NOTIFY
                 userRatingChanged FINAL)

public:
  explicit Recommendation(QObject *parent = nullptr);

  QString const userRating() const { return m_userRating; }

  void setUserRating(QString const &value);

signals:
  void userRatingChanged();

private:
  QString m_userRating;
};

#endif // RECOMMENDATION_H
