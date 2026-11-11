#ifndef MLCLIENT_H
#define MLCLIENT_H

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

/*!
 * \brief HTTP client for the ML server (python/vestir_server.py).
 *
 * The recommend request is: temperature, rain_probability (0-100), event or
 * event_title, items[], excluded_outfits[][], limit. The result has "event"
 * and "outfits", each outfit being {item_ids, score}, best first.
 *
 * Server address: env VESTIR_ML_URL, otherwise http://10.0.2.2:8001 on Android
 * (the emulator's alias for the PC) and http://127.0.0.1:8001 elsewhere.
 */
class MlClient : public QObject {
  Q_OBJECT
public:
  explicit MlClient(QObject *parent = nullptr);

  /*! Called once per request: success flag, JSON answer, error text. */
  using Done = std::function<void(bool, const QJsonObject &, const QString &)>;

  /*! Starts the recommender and calls \a done once with the answer. */
  void recommend(const QJsonObject &request, Done done);

  /*!
   * Asks the ML which event type (school/work/party/hangout/gym) each calendar
   * title is. Calls \a done once; "types" maps title to type.
   */
  void eventTypes(const QStringList &titles, Done done);

  /*!
   * Labels one clothing photo through the ML server's /analyze and waits for
   * the answer (up to a couple of minutes on the first call).
   * Blocks, so call it from a worker thread. Returns {category_name,
   * sub_category, primary_color, formality, confidence, ...}; on failure the
   * result is empty and \a error says why.
   */
  static QJsonObject classifyPhoto(const QString &imagePath, QString *error);

private:
  /*! POSTs the JSON request to the ML server and calls \a done with the answer. */
  void post(const QString &endpoint, const QJsonObject &request, Done done);

  QNetworkAccessManager m_manager;
};

#endif // MLCLIENT_H
