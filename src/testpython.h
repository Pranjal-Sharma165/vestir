#ifndef TESTPYTHON_H
#define TESTPYTHON_H

#include <filesystem>
#include <iostream>
#include <stdexcept>
#pragma push_macro("slots")
#pragma push_macro("signals")
#pragma push_macro("init")
#undef slots
#undef signals
#undef init
#include <Python.h>
#pragma pop_macro("init")
#pragma pop_macro("signals")
#pragma pop_macro("slots")

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <format>
#include <string>

struct PythonPaths {
  std::string home, app, exe;
};

static PythonPaths preparePython() {
#ifdef Q_OS_ANDROID
  const QString src = "assets:/python";
  const QString dest =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
      "/python";
  const QString marker = dest + "/.extracted-1";

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

static void initPython(const PythonPaths &p) {
  std::cout << "p.home=" << p.home.c_str() << std::endl;
  std::cout << "p.app=" << p.app.c_str() << std::endl;
  std::cout << "p.exe=" << p.exe.c_str() << std::endl;

  PyConfig config;
  PyConfig_InitIsolatedConfig(&config);

  PyStatus status = PyStatus_Ok();

  if (!p.home.empty()) {
    status = PyConfig_SetBytesString(&config, &config.home, p.home.c_str());
  }

  if (!PyStatus_Exception(status) && !p.exe.empty()) {
    status =
        PyConfig_SetBytesString(&config, &config.executable, p.exe.c_str());
  }

  if (!PyStatus_Exception(status)) {
    status = Py_InitializeFromConfig(&config);
  }

  PyConfig_Clear(&config);

  if (!PyStatus_Exception(status)) {
    std::string script = "import sys\n"
                         "sys.path.insert(0, '" +
                         p.app + "')\n";

    PyRun_SimpleString(script.c_str());
  }

  if (PyStatus_Exception(status)) {
    throw std::runtime_error(std::format(
        "Python init failed in {}: {}", status.func ? status.func : "?",
        status.err_msg ? status.err_msg : "unknown"));
  }
}

static void runPythonTest() {
  const PythonPaths paths = preparePython();
  initPython(paths);
  PyObject *mod = PyImport_ImportModule("test");
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

  std::filesystem::path testImage =
      std::filesystem::path(paths.app) / "tests" / "images" / "nike_dress.jpg";
  auto u8_str = testImage.u8string();
  std::string utf8_path(u8_str.begin(), u8_str.end());

  PyObject *py_image_path = PyUnicode_FromString(utf8_path.c_str());
  if (!py_image_path) {
    PyErr_Print();
    Py_DECREF(fn);
    Py_DECREF(mod);
    throw std::runtime_error("Failed to create Python string for path");
  }

  PyObject *pArgs = PyTuple_New(1);
  PyTuple_SetItem(pArgs, 0, py_image_path);

  PyObject *result = PyObject_CallObject(fn, pArgs);
  Py_DECREF(fn);
  Py_DECREF(pArgs);
  Py_DECREF(mod);

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

  std::cout << "Result: " << resultStr << std::endl;
  Py_DECREF(result);
}

#endif // TESTPYTHON_H
