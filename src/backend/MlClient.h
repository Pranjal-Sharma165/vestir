#ifndef MLCLIENT_H
#define MLCLIENT_H

#include "CalendarEntry.h"
#include "ClothingRepository.h"
#include "OutfitRecommendation.h"
#include "WeatherData.h"
#include <QObject>
#include <QString>
#include <QStringList>
#include <condition_variable>
#include <future>
#include <mutex>
#include <queue>
#include <thread>

/*!
 * \brief class MlClient allows callers to invoke Python workloads for
 * classification and recommendation tasks.
 *
 * Most functions in this class delegate control to a shared thread that
 * contains a preinitialized Python interpreter, blocking until that thread is
 * done its work. In the case of classifier-related workloads, functions could
 * block for quite a while if they need to load the classifier's weights into
 * memory.
 */
class MlClient : public QObject {
  Q_OBJECT
  QML_UNCREATABLE("Owned by backend")
public:
  explicit MlClient(QObject *parent = nullptr);

  ~MlClient();

  static constexpr int categoryCount = 4;
  /*!
   * \brief Classes that the classifier returns.
   */
  struct ClassifierLabels {
    ClothingCategory::Value category;
    ClothingCategory::SubCategory subCategory;
    ClothingColour::Value colour;
    OutfitFormality::Value formality;
  };

  struct PhotoClassifierJob {
    std::filesystem::path path;
    std::promise<std::optional<ClassifierLabels>> promise;
  };

  struct EventClassifierJob {
    std::vector<std::string> eventTitles;
    std::promise<std::vector<OutfitFormality::Value>> promise;
  };

  struct RecommenderJob {
    std::vector<ClothingRepository::StoredItem> clothingItems;
    std::vector<OutfitRecommendation> pastRecommendations;
    std::optional<WeatherData> weather;
    std::optional<CalendarEntry> event;
    int requestedCount;
    std::promise<std::vector<OutfitRecommendation>> promise;
  };

  /*!
   * \brief Load the weights for the classifier into memory. Must be called
   * before any function that needs the classifier is called.
   *
   * This function must be explicitly called so that the caller is aware of
   * the lifetime of the model weights, which consume a lot of memory, and has
   * an idea of when to call `unloadWeights()`.
   */
  void loadWeights();

  /*!
   * \brief Unload the weights for the classifier to free up memory.
   */
  void unloadWeights();

  /*!
   * \brief Generate `requestedCount` new outfits to recommend to the user.
   * \param weather The weather to consider when generating outfits.
   * \param clothingItems The pieces of clothing to create outfits from.
   * \param pastRecommendations Any past outfit recommendations that should
   * not be repeated.
   * \param requestedCount The number of outfits to create.
   */
  std::future<std::vector<OutfitRecommendation>> recommendLocal(
      const std::vector<ClothingRepository::StoredItem> &clothingItems,
      std::vector<OutfitRecommendation> pastRecommendations, int requestedCount,
      std::optional<WeatherData> weather = std::nullopt,
      std::optional<CalendarEntry> event = std::nullopt);

  /*!
   * \brief Classify an image of a clothing item.
   * \param imagePath the filesystem path to the image.
   * \throws std::filesystem::filesystem_error if the file does not exist.
   * \return The classifications, or std::nullopt if the image was not
   * recognized to be a piece of clothing.
   */
  std::future<std::optional<MlClient::ClassifierLabels>>
  classifyPhotoLocal(const std::filesystem::path imagePath);

  /*!
   * \brief Classify an image of a clothing item.
   * \param imagePath the filesystem path to the image.
   * \throws std::filesystem::filesystem_error if the file does not exist.
   * \throws std::runtime_exception If loadWeights was not called beforehand.
   * \return The classifications.
   */
  std::future<std::vector<OutfitFormality::Value>>
  classifyEventsLocal(const std::vector<std::string> titles);

private:
  // shared condition variable for all job queues.
  std::condition_variable_any m_pyCond;
  // shared mutex for all job queues.
  std::mutex m_pyMtx;
  std::queue<PhotoClassifierJob> m_photoClassifierQueue;
  std::queue<EventClassifierJob> m_eventClassifierQueue;
  std::queue<RecommenderJob> m_recommenderQueue;
  void startPythonWorkerLoop();
  std::atomic<bool> m_pyThreadStop;
  std::thread m_pyThread;
};

#endif // MLCLIENT_H
