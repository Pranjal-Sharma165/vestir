#include "MlClient.h"
#include "EnumStringMappings.h"
#include "LabelStringMappings.h"
#include <QFile>
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

void throwIfPyException(PyStatus status) {
  if (PyStatus_Exception(status)) {
    throw std::runtime_error(
        std::format("Python failed in {}: {}", status.func ? status.func : "?",
                    status.err_msg ? status.err_msg : "unknown"));
  }
}

} // namespace

MlClient::MlClient(QObject *parent)
    : QObject(parent),
      m_pyThread(std::thread([this]() { startPythonWorkerLoop(); })) {}

std::future<std::vector<OutfitRecommendation>> MlClient::recommendLocal(
    const std::vector<ClothingRepository::StoredItem> &clothingItems,
    const std::vector<OutfitRecommendation> &pastRecommendations,
    const int requestedCount, const std::optional<WeatherData> &weather,
    const std::optional<CalendarEntry> &event) {
  // TODO replace dummy values
  std::promise<std::vector<OutfitRecommendation>> promise;
  promise.set_value({});
  return promise.get_future();
}

std::future<MlClient::ClassifierLabels>
MlClient::classifyPhotoLocal(const std::filesystem::path &imagePath) {
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
MlClient::classifyEventsLocal(const QStringList &titles) {
  std::promise<std::vector<OutfitFormality::Value>> promise;
  std::vector<OutfitFormality::Value> formalities;
  for (auto &title : titles) {
    // TODO replace dummy values
    formalities.push_back(OutfitFormality::Value::Casual);
  }
  promise.set_value({});
  return promise.get_future();
}

void MlClient::startPythonWorkerLoop() {
  const PythonPaths paths = preparePython();
  PyConfig config;
  PyConfig_InitIsolatedConfig(&config);
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
  PyObject *mod = PyImport_ImportModule("classify_api");
  if (!mod) {
    PyErr_Print();
    throw std::runtime_error("PyImport_ImportModule failed");
  }

  PyObject *fn = PyObject_GetAttrString(mod, "classify");
  if (!fn || !PyCallable_Check(fn)) {
    PyErr_Print();
    Py_XDECREF(fn);
    Py_DECREF(mod);
    throw std::runtime_error("Failed to get callable function from module");
  }

  // consume jobs in queue
  PhotoClassifierJob job;
  while (true) {
    {
      std::unique_lock<std::mutex> lock(m_pyMtx);
      if (m_photoClassifierQueue.empty()) {
        m_pyCond.wait(lock);
      }
      job = std::move(m_photoClassifierQueue.front());
      m_photoClassifierQueue.pop();
    }
    auto labels = pyClassifyImage(fn, job.path);
    try {
      job.promise.set_value(labels);
    } catch (const std::exception &e) {
      std::cerr << "Photo classification failed: " << e.what() << std::endl;
      job.promise.set_exception(std::current_exception());
    } catch (...) {
      job.promise.set_exception(std::current_exception());
    }
  }
  Py_DECREF(fn);
  Py_DECREF(mod);
}
