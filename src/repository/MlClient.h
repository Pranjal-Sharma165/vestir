#ifndef MLCLIENT_H
#define MLCLIENT_H

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <functional>

class QProcess;

/*!
 * \brief Runs the ML team's Python recommender (python/recommend_cli.py) in a
 * separate process and returns its answer.
 *
 * The request is the same JSON as POST /recommendations in python/app2.py:
 * temperature, rain_probability (0-100), event or event_title, items[],
 * excluded_outfits[][], limit. The result has "event" and "outfits", each
 * outfit being {item_ids, score}, best first.
 *
 * Where it finds things (PC only for now):
 *  - Python: env VESTIR_PYTHON, otherwise "python" from PATH.
 *  - Script: env VESTIR_ML_DIR (the python folder), otherwise the first
 *    python/recommend_cli.py found walking up from the app's folder.
 *
 * TODO later:
 *  - Android cannot start a Python process. Keep this class's interface and
 *    swap the implementation (embedded Python, or a server) when we get there.
 *  - The image classifier (torch) is not covered here yet.
 */
class MlClient : public QObject {
  Q_OBJECT
public:
  explicit MlClient(QObject *parent = nullptr);

  /*! Starts the recommender. Emits recommended() once, when it is done. */
  void recommend(const QJsonObject &request);

  /*!
   * Asks the ML which event type (school/work/party/hangout/gym) each calendar
   * title is. Emits eventTypesReady() once; "types" maps title to type.
   */
  void eventTypes(const QStringList &titles);

  /*!
   * Labels one clothing photo with python/classify_cli.py and waits for the
   * answer (up to a couple of minutes: the model is loaded on every call).
   * Blocks, so call it from a worker thread. Returns {category_name,
   * sub_category, primary_color, formality, confidence, ...}; on failure the
   * result is empty and \a error says why.
   */
  static QJsonObject classifyPhoto(const QString &imagePath, QString *error);

  /*! Full path of a script in the python folder, or "" if not found. */
  static QString scriptPath(const QString &name = "recommend_cli.py");

signals:
  void recommended(bool ok, const QJsonObject &result, const QString &error);
  void eventTypesReady(bool ok, const QJsonObject &result,
                       const QString &error);

private:
  using Done = std::function<void(bool, const QJsonObject &, const QString &)>;
  /*! Sends the request to the server, or to the CLI script when no server. */
  void run(const QString &endpoint, const QString &script,
           const QJsonObject &request, Done done);
  void finish(QProcess *process, Done done);
};

#endif // MLCLIENT_H
