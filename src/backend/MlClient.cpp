#include "MlClient.h"
#include "EnumStringMappings.h"
#include <QFile>
#include <QJsonArray>
#include <cctype>
#include <format>
#include <iostream>
#include <sstream>
#include <string>

#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("init")
#undef slots
#undef signals
#undef init
#include <Python.h>
#include <QDirIterator>
#include <QStandardPaths>
#pragma pop_macro("init")
#pragma pop_macro("signals")
#pragma pop_macro("slots")

namespace {
constexpr int kTimeoutMs = 30000;
constexpr int kUploadMaxSide = 1024;
constexpr int kClassifyTimeoutMs = 180000; // first run downloads the model
// Used when no weather has ever been saved (first run, offline).
constexpr double kFallbackTempC = 15;
constexpr double kFallbackRain = 0;

struct PythonPaths {
  std::string home, app, exe;
};

/*!
 * \brief Prepare the Python execution directory on first run.
 *
 * On Android, recursively traverse the `python` directory in the app's assets
 * and extract all contents to a writable folder (to support `__pycache__`). On
 * desktop, do nothing.
 * \return Paths to the expected working directory (`app`), where the Python
 * stdlib is located (`home`), and the Python executable to call (`exe`).
 */
PythonPaths preparePython() {
#ifdef Q_OS_ANDROID
  const QString src = "assets:/python";
  const QString dest =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/python";
  const QString marker = dest + "/.extracted-2";

  QDir dir;
  if (!dir.exists(dest)) {
    if (!dir.mkpath(dest)) {
      throw std::runtime_error(
          ("Failed to create Python destination directory: " + dest)
              .toStdString());
    }
  }

  int copied = 0;
  const bool complete =
      QFile::exists(marker) && QFile::exists(dest + "/lib/python3.14/os.py");
  if (!complete) {
    QDirIterator it(src, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
      const QString from = it.next();
      const QString to = dest + from.mid(src.size());
      QDir().mkpath(QFileInfo(to).absolutePath());
      QFile::remove(to);
      if (!QFile::copy(from, to))
        throw std::runtime_error(("Failed to extract " + from).toStdString());
      ++copied;
    }

    if (copied == 0)
      throw std::runtime_error(("No files found under " + src).toStdString());

    QFile markerFile(marker);
    if (!markerFile.open(QIODevice::WriteOnly)) {
      throw std::runtime_error(
          ("Could not open file " + marker + ": " + markerFile.errorString())
              .toStdString());
    }
    markerFile.close();
  }

  return {dest.toStdString(), (dest + "/app").toStdString(), ""};
#else
  return {PYTHON_HOME_DIR, APP_PYTHON_DIR, PYTHON_EXECUTABLE};
#endif
}

/*!
 * \brief Parse the result of the `classify` Python function.
 * \param classString The string to be parsed. Must be in the format of
 * `{category},{subcategory},{colour},{formality}` with each
 * string corresponding exactly to the enumerated type symbol name in C++.
 * \return The labels.
 * \throws std::runtime_error if the wrong number of space-separated strings
 * were returned.
 * \throws std::runtime_error if the wrong number of space-separated strings
 * were returned.
 */
MlClient::ClassifierLabels
parsePhotoClassificationResult(const std::string &classString) {
  std::cout << classString << std::endl;
  std::istringstream iss(classString);
  std::vector<std::string> words;
  std::string word;
  while (std::getline(iss, word, ',')) {
    words.push_back(word);
  }
  if (words.size() != MlClient::categoryCount) {
    throw std::runtime_error(
        std::format("Incorrect number of classes returned: expected {}, got {}",
                    MlClient::categoryCount, words.size()));
  }

  // From Isaac: this needs to be capitalized for some reason... dunno why the
  // Python code appears to use both lowercase and capitalized categories.
  auto categoryString = words[0];
  if (!categoryString.empty()) {
    categoryString[0] = toupper(categoryString[0]);
  }
  auto subCategoryString = words[1];
  auto colourString = words[2];
  auto formalityString = words[3];
  MlClient::ClassifierLabels classes;

  // let the program crash if the Python program returned incorrect output. What
  // else is there to do?
  classes.category = valueFor(kCategories, categoryString).value();
  classes.subCategory = valueFor(kSubCategories, subCategoryString).value();
  classes.colour = valueFor(kColours, colourString).value();
  classes.formality = valueFor(kFormalities, formalityString).value();
  return classes;
}

/*!
 * \brief Invoke the `classify` Python function in a blocking
 * fashion to classify an image.
 * \param fn Must be the `classify` function.
 * \param imagePath The path to the image.
 * \return The computed labels for the image.
 */
MlClient::ClassifierLabels
pyClassifyImage(PyObject *fn, const std::filesystem::path &imagePath) {

  auto u8_str = imagePath.u8string();
  std::string utf8_path(u8_str.begin(), u8_str.end());
  PyObject *py_image_path = PyUnicode_FromString(utf8_path.c_str());
  if (!py_image_path) {
    PyErr_Print();
    throw std::runtime_error("Failed to create Python string for path");
  }

  PyObject *pArgs = PyTuple_New(1);
  PyTuple_SetItem(pArgs, 0, py_image_path);

  PyObject *result = PyObject_CallObject(fn, pArgs);
  Py_DECREF(pArgs);

  if (!result) {
    PyErr_Print();
    throw std::runtime_error("");
  }

  if (result == Py_None) {
    throw std::runtime_error("classify returned None");
  }

  if (!PyUnicode_Check(result)) {
    PyErr_Print();
    Py_DECREF(result);
    throw std::runtime_error("");
  }

  auto resultStr = PyUnicode_AsUTF8(result);
  Py_DECREF(result);

  return parsePhotoClassificationResult(resultStr);
}

std::vector<OutfitFormality::Value>
pyClassifyEvents(PyObject *fn, const std::vector<std::string> &eventTitles) {
  Py_ssize_t inputLen = eventTitles.size();

  PyObject *pListArg = PyList_New(inputLen);
  for (Py_ssize_t i = 0; i < inputLen; i++) {
    PyObject *pStr = PyUnicode_FromString(eventTitles[i].c_str());
    PyList_SetItem(pListArg, i, pStr);
  }

  PyObject *pArgs = PyTuple_New(1);
  PyTuple_SetItem(pArgs, 0, pListArg);

  PyObject *pResult = PyObject_CallObject(fn, pArgs);
  Py_DECREF(pArgs);

  // 5. Parse the output: Python List of Strings -> C strings
  if (!pResult) {
    PyErr_Print();
    throw std::runtime_error("PyObject_CallObject failed");
  }

  if (!PyList_Check(pResult)) {
    throw std::runtime_error("Return value was not a list");
  }

  Py_ssize_t resultLen = PyList_Size(pResult);
  if (resultLen != inputLen) {
    throw std::runtime_error(
        std::format("Event formality list is of a different length {} compared "
                    "to expected length {}",
                    resultLen, inputLen));
  }

  std::vector<OutfitFormality::Value> formalities;
  formalities.resize(resultLen);
  for (Py_ssize_t i = 0; i < resultLen; i++) {
    PyObject *pItem = PyList_GetItem(pResult, i);
    if (PyUnicode_Check(pItem)) {
      const char *c_str = PyUnicode_AsUTF8(pItem);
      formalities[i] = valueFor(kFormalities, c_str).value();
    }
  }
  Py_DECREF(pResult);
  return formalities;
}

void throwIfPyException(PyStatus status) {
  if (PyStatus_Exception(status)) {
    throw std::runtime_error(
        std::format("Python failed in {}: {}", status.func ? status.func : "?",
                    status.err_msg ? status.err_msg : "unknown"));
  }
}

std::vector<OutfitRecommendation>
pyRecommend(PyObject *fn, const MlClient::RecommenderJob &params) {
  QJsonArray items;
  for (const auto &s : params.clothingItems) {
    QJsonObject mlLabels = {
        {"clothing_item_id", s.id},
        {"category_name",
         QString::fromUtf8(textFor(kCategories, s.item->category()).value())},
        {"sub_category",
         QString::fromUtf8(
             textFor(kSubCategories, s.item->subCategory()).value())},
        {"primary_color",
         QString::fromUtf8(textFor(kColours, s.item->colour()).value())},
        {"formality",
         QString::fromUtf8(textFor(kFormalities, s.item->formality()).value())},
    };
    items.append(std::move(mlLabels));
  }

  QJsonArray excludedItems;
  for (const auto &excludedItem : excludedItems) {
    // TODO have excluded items in request
  }

  QJsonObject request{
      {"temperature",
       params.weather ? params.weather->temperatureC : kFallbackTempC},
      {"rain_probability",
       params.weather ? double(params.weather->precipitationProbability)
                      : kFallbackRain},
      {"items", items},
      {"excluded_outfits", excludedItems},
      {"limit", params.requestedCount}};
  if (params.event) {
    request["event_title"] = params.event->name;
  }

  QJsonObject returned;
  QByteArray inJson = QJsonDocument(request).toJson(QJsonDocument::Compact);

  PyObject *json = PyImport_ImportModule("json");
  PyObject *arg =
      json ? PyObject_CallMethod(json, "loads", "y", inJson.constData())
           : nullptr;
  PyObject *ret =
      arg ? PyObject_CallFunctionObjArgs(fn, arg, nullptr) : nullptr;
  PyObject *str = ret ? PyObject_CallMethod(json, "dumps", "O", ret) : nullptr;

  if (str) {
    const char *utf8 = PyUnicode_AsUTF8(str);
    if (utf8)
      returned = QJsonDocument::fromJson(utf8).object();
  } else {
    PyErr_Print();
  }

  Py_XDECREF(str);
  Py_XDECREF(ret);
  Py_XDECREF(arg);
  Py_XDECREF(json);

  return {};
}

} // namespace

MlClient::MlClient(QObject *parent)
    : QObject(parent),
      m_pyThread(std::thread([this]() { startPythonWorkerLoop(); })) {}

std::future<std::vector<OutfitRecommendation>> MlClient::recommendLocal(
    const std::vector<ClothingRepository::StoredItem> &clothingItems,
    std::vector<OutfitRecommendation> pastRecommendations, int requestedCount,
    std::optional<WeatherData> weather, std::optional<CalendarEntry> event) {
  std::promise<std::vector<OutfitRecommendation>> promise;

  auto future = promise.get_future();
  std::vector<ClothingRepository::StoredItem> clothingItemsCopy;
  for (auto &storedItem : clothingItems) {
    clothingItemsCopy.push_back(ClothingRepository::StoredItem{
        .id = storedItem.id,
        .item = std::make_unique<ClothingItem>(*storedItem.item),
        .ml = storedItem.ml,
    });
  }

  {
    const std::unique_lock<std::mutex> lock(m_pyMtx);
    m_recommenderQueue.push(RecommenderJob{
        .clothingItems = std::move(clothingItemsCopy),
        .pastRecommendations = pastRecommendations,
        .weather = weather,
        .event = event,
        .requestedCount = requestedCount,
        .promise = std::move(promise),
    });
    m_pyCond.notify_one();
  }
  return future;
}

std::future<MlClient::ClassifierLabels>
MlClient::classifyPhotoLocal(const std::filesystem::path imagePath) {
  std::promise<ClassifierLabels> promise;
  auto future = promise.get_future();
  {
    const std::unique_lock<std::mutex> lock(m_pyMtx);
    m_photoClassifierQueue.push(
        PhotoClassifierJob{imagePath, std::move(promise)});
    m_pyCond.notify_one();
  }
  return future;
}

std::future<std::vector<OutfitFormality::Value>>
MlClient::classifyEventsLocal(const std::vector<std::string> titles) {
  std::promise<std::vector<OutfitFormality::Value>> promise;
  auto future = promise.get_future();
  {
    const std::unique_lock<std::mutex> lock(m_pyMtx);
    m_eventClassifierQueue.push(EventClassifierJob{titles, std::move(promise)});
    m_pyCond.notify_one();
  }
  return future;
}

void MlClient::startPythonWorkerLoop() {
  const PythonPaths paths = preparePython();
  PyConfig config;
  PyConfig_InitIsolatedConfig(&config);
  config.buffered_stdio = 0;
  if (!paths.home.empty()) {
    throwIfPyException(
        PyConfig_SetBytesString(&config, &config.home, paths.home.c_str()));
  }
  throwIfPyException(
      PyConfig_SetBytesString(&config, &config.executable, paths.exe.c_str()));
  throwIfPyException(Py_InitializeFromConfig(&config));
  PyConfig_Clear(&config);

  // Add the app's working directory to the import path
  std::string script = "import sys\n"
                       "sys.path.insert(0, '" +
                       paths.app + "')\n";
  PyRun_SimpleString(script.c_str());

  // Import the classifier module
  PyObject *mod = PyImport_ImportModule("libpython_api");
  if (!mod) {
    PyErr_Print();
    throw std::runtime_error("PyImport_ImportModule failed");
  }

  PyObject *classifyPhotoFunction = PyObject_GetAttrString(mod, "classify");
  if (!classifyPhotoFunction || !PyCallable_Check(classifyPhotoFunction)) {
    PyErr_Print();
    Py_XDECREF(classifyPhotoFunction);
    Py_DECREF(mod);
    throw std::runtime_error("Failed to get callable function from module");
  }

  PyObject *classifyEventFunction =
      PyObject_GetAttrString(mod, "classify_events");
  if (!classifyEventFunction || !PyCallable_Check(classifyEventFunction)) {
    PyErr_Print();
    Py_XDECREF(classifyEventFunction);
    Py_DECREF(classifyPhotoFunction);
    Py_DECREF(mod);
    throw std::runtime_error("Failed to get callable function from module");
  }

  PyObject *recommendFunction = PyObject_GetAttrString(mod, "recommend");
  if (!recommendFunction || !PyCallable_Check(recommendFunction)) {
    PyErr_Print();
    Py_XDECREF(recommendFunction);
    Py_DECREF(classifyEventFunction);
    Py_DECREF(classifyPhotoFunction);
    Py_DECREF(mod);
    throw std::runtime_error("Failed to get callable function from module");
  }

  // consume jobs in queue.
  //
  // Isaac: This contains a very bad race condition where, if the promise is not
  // read in time and there are other jobs waiting, the job will be reassigned
  // and the promise will be lost. As long as the queues remain of maximum size
  // 1 in real use, this is prevented. I don't have time to fix this right now.
  PhotoClassifierJob photoClassifierJob;
  EventClassifierJob eventClassifierJob;
  RecommenderJob recommenderJob;
  while (true) {
    std::unique_lock<std::mutex> lock(m_pyMtx, std::defer_lock);

    lock.lock();
    if (m_photoClassifierQueue.empty() && m_eventClassifierQueue.empty() &&
        m_recommenderQueue.empty()) {
      m_pyCond.wait(lock);
    }

    if (!m_photoClassifierQueue.empty()) {
      photoClassifierJob = std::move(m_photoClassifierQueue.front());
      m_photoClassifierQueue.pop();
      lock.unlock();

      auto labels =
          pyClassifyImage(classifyPhotoFunction, photoClassifierJob.path);
      try {
        photoClassifierJob.promise.set_value(labels);
      } catch (const std::exception &e) {
        std::cerr << "Photo classification failed: " << e.what() << std::endl;
        photoClassifierJob.promise.set_exception(std::current_exception());
      } catch (...) {
        photoClassifierJob.promise.set_exception(std::current_exception());
      }

    } else if (!m_eventClassifierQueue.empty()) {
      eventClassifierJob = std::move(m_eventClassifierQueue.front());
      m_eventClassifierQueue.pop();
      lock.unlock();

      auto formalities = pyClassifyEvents(classifyEventFunction,
                                          eventClassifierJob.eventTitles);

      try {
        eventClassifierJob.promise.set_value(formalities);
      } catch (const std::exception &e) {
        std::cerr << "Event classification failed: " << e.what() << std::endl;
        eventClassifierJob.promise.set_exception(std::current_exception());
      } catch (...) {
        eventClassifierJob.promise.set_exception(std::current_exception());
      }

    } else if (!m_recommenderQueue.empty()) {
      recommenderJob = std::move(m_recommenderQueue.front());
      m_recommenderQueue.pop();
      lock.unlock();

      auto recommendations = pyRecommend(recommendFunction, recommenderJob);

      try {
        recommenderJob.promise.set_value(recommendations);
      } catch (const std::exception &e) {
        std::cerr << "Recommendation function failed: " << e.what()
                  << std::endl;
        recommenderJob.promise.set_exception(std::current_exception());
      } catch (...) {
        recommenderJob.promise.set_exception(std::current_exception());
      }

    } else {
      lock.unlock();
    }
  }
  Py_DECREF(classifyPhotoFunction);
  Py_DECREF(mod);
}
