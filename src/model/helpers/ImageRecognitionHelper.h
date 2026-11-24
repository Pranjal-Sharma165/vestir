#ifndef IMAGERECOGNITIONHELPER_H
#define IMAGERECOGNITIONHELPER_H

#include "../../backend/MlClient.h"
#include "ClothingItem.h"
#include <QFutureWatcher>
#include <QObject>
#include <QUrl>
#include <QtConcurrent/QtConcurrent>

/*!
 * \brief The ImageRecognitionHelper class provides helper functions relating to
 * classifying clothing in an image.
 */
class ImageRecognitionHelper : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend");

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

  explicit ImageRecognitionHelper(MlClient &mlClient, QObject *parent = nullptr)
      : QObject(parent), m_mlClient(mlClient) {}

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

    m_watcher = new QFutureWatcher<MlClient::ClassifierLabels>(this);
    connect(m_watcher, &QFutureWatcherBase::finished, this, [this, imagePath] {
      auto labels = m_watcher->result();
      // TODO replace placeholder name
      m_item = new ClothingItem(
          "Placeholder", labels.category, labels.subCategory, labels.colour,
          labels.formality, imagePath.toStdString(), this);
      m_state = Ready;
      m_watcher->deleteLater();
      m_watcher = nullptr;
      emit stateChanged();
    });
    m_watcher->setFuture(
        QtConcurrent::run(&ImageRecognitionHelper::classify, this, imagePath));
  }

  /*!
   * \brief Replaces the classifier's labels with the user's correction. The
   * item becomes a new object (same photo and formality), so stateChanged is
   * emitted.
   */
  Q_INVOKABLE void relabel(ClothingCategory::Value category,
                           ClothingCategory::SubCategory subCategory,
                           ClothingColour::Value colour) {
    if (!m_item ||
        !ClothingCategory::subcategoryIsInCategory(subCategory, category))
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
  MlClient::ClassifierLabels classify(const QString &imagePath) {
    return m_mlClient.classifyPhotoLocal(imagePath.toStdString()).get();
  }

  State m_state = Loading;
  ClothingItem *m_item = nullptr;
  QFutureWatcher<MlClient::ClassifierLabels> *m_watcher = nullptr;
  QString m_error;
  MlClient &m_mlClient;
};

#endif // IMAGERECOGNITIONHELPER_H
