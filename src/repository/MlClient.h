#ifndef MLCLIENT_H
#define MLCLIENT_H

#include <QJsonObject>
#include <QObject>
#include <QString>

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

  /*! Full path of recommend_cli.py, or an empty string if not found. */
  static QString scriptPath();

signals:
  void recommended(bool ok, const QJsonObject &result, const QString &error);

private:
  void finish(QProcess *process);
};

#endif // MLCLIENT_H
