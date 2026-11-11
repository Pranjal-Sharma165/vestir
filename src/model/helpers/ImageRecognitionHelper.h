#ifndef IMAGERECOGNITIONHELPER_H
#define IMAGERECOGNITIONHELPER_H

#include "ClothingItem.h"
#include "../../backend/ClothingRepository.h"
#include "../../backend/MlClient.h"
#include <QFutureWatcher>
#include <QObject>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>
#include <optional>

/*!
 * \brief The ImageRecognitionHelper class provides helper functions relating to
 * classifying clothing in an image.
 */
class ImageRecognitionHelper : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(State state READ state NOTIFY stateChanged)
  Q_PROPERTY(ClothingItem *item READ item NOTIFY stateChanged)
  Q_PROPERTY(QString error READ error NOTIFY stateChanged)
public:
  /*!
   * \brief The State enum defines the type used by this class to indicate the
   * result of the classification.
   */
  enum State { Loading, Ready, Failed };
  Q_ENUM(State)

  explicit ImageRecognitionHelper(QObject *parent = nullptr)
      : QObject(parent) {}

  /*!
   * \brief Begin image classification. Registers a job to run in the backround,
   * setting `state` and emitting `stateChanged` upon completion.
   * \param image The path to the image.
   */
  Q_INVOKABLE void start(const QUrl &image) {
    const QString imagePath = image.toLocalFile();

    // Drop previous run if it exists
    if (m_watcher) {
      m_watcher->disconnect(this);
      m_watcher->deleteLater();
      m_watcher = nullptr;
    }
    if (m_item) {
      m_item->deleteLater();
      m_item = nullptr;
    }
    m_error.clear();
    m_state = Loading;
    emit stateChanged();

    m_watcher = new QFutureWatcher<ClassifierResult>(this);
    connect(m_watcher, &QFutureWatcherBase::finished, this, [this, imagePath] {
      ClassifierResult r;
      try {
        r = m_watcher->result();
      } catch (const std::exception &e) {
        r.error = QString::fromUtf8(e.what());
      }

      if (r.data) {
        const auto &d = *r.data;
        try {
          m_item = new ClothingItem(
              d.name, d.category, d.subCategory, d.colour, d.formality,
              std::filesystem::path(imagePath.toStdString()), this);
          m_state = Ready;
        } catch (const std::exception &e) {
          m_error = QString::fromUtf8(e.what());
          m_state = Failed;
        }
      } else {
        m_error = r.error;
        m_state = Failed;
      }
      m_watcher->deleteLater();
      m_watcher = nullptr;
      emit stateChanged();
    });
    m_watcher->setFuture(QtConcurrent::run(&classify, imagePath));
  }

  /*!
   * \brief Replaces the classifier's labels with the user's correction. The
   * item becomes a new object (same photo and formality), so stateChanged is
   * emitted.
   */
  Q_INVOKABLE void relabel(ClothingCategory::Value category,
                           ClothingCategory::SubCategory subCategory,
                           ClothingColour::Value colour) {
    if (!m_item || !ClothingCategory::subcategoryIsInCategory(subCategory,
                                                              category))
      return;
    auto *fixed = new ClothingItem(
        std::string(ClothingCategory::displayName(subCategory)), category,
        subCategory, colour, m_item->formality(),
        std::filesystem::path(m_item->qImageUrl().toLocalFile().toStdString()),
        this);
    m_item->deleteLater();
    m_item = fixed;
    emit stateChanged();
  }

  State state() const { return m_state; }
  ClothingItem *item() const { return m_item; }
  QString error() const { return m_error; }

signals:
  void stateChanged();

private:
  /*!
   * \brief The ClassifierData struct defines the values returned by
   * the job in `start()`.
   */
  struct ClassifierData {
    std::string name;
    ClothingCategory::Value category;
    ClothingCategory::SubCategory subCategory;
    ClothingColour::Value colour;
    OutfitFormality::Value formality;
  };

  /*!
   * \brief `std::optional` wrapper around `ClassifierData` with an error
   * message.
   */
  struct ClassifierResult {
    std::optional<ClassifierData> data;
    QString error;
  };

  /*!
   * \brief Classify the clothing pictured in the image.
   * \param imagePath The path to the image.
   * \return The result, with `data` unset and `error` set if there was an
   * error.
   */
  static inline ClassifierResult classify(const QString &imagePath) {
    ClassifierResult r;
    const QJsonObject ml = MlClient::classifyPhoto(imagePath, &r.error);
    if (ml.isEmpty())
      return r;

    const auto labels = ClothingRepository::labelsFromMl(ml);
    if (!labels) {
      r.error = "The classifier returned a label the app does not know";
      return r;
    }
    r.data = ClassifierData{
        std::string(ClothingCategory::displayName(labels->subCategory)),
        labels->category, labels->subCategory, labels->colour,
        labels->formality};
    return r;
  }

  State m_state = Loading;
  ClothingItem *m_item = nullptr;
  QFutureWatcher<ClassifierResult> *m_watcher = nullptr;
  QString m_error;
};

#endif // IMAGERECOGNITIONHELPER_H
