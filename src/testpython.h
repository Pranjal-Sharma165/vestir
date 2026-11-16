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

#include <format>
#include <string>

static void initPython() {
  PyConfig config;
  PyConfig_InitIsolatedConfig(&config);

  PyStatus status =
      PyConfig_SetBytesString(&config, &config.home, PYTHON_HOME_DIR);

  if (!PyStatus_Exception(status)) {
    status = Py_InitializeFromConfig(&config);
  }

  PyConfig_Clear(&config);
  if (PyStatus_Exception(status)) {
    throw std::runtime_error(std::format(
        "Python init failed in {}: {} (home={})",
        status.func ? status.func : "?",
        status.err_msg ? status.err_msg : "unknown", PYTHON_HOME_DIR));
  }

  std::string cmd =
      std::string("import sys; sys.path.insert(0, r'") + APP_PYTHON_DIR + "')";
  int rc = PyRun_SimpleString(cmd.c_str());

  if (rc != 0) {
    throw std::runtime_error("Python sys.path init failed\n");
  }
}

static void runPythonTest() {
  initPython();
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
      std::filesystem::weakly_canonical("./app_python/tests/images") /
      "nike_dress.jpg";
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
  Py_XDECREF(result);

  std::cout << "Result: " << resultStr << std::endl;
  Py_DECREF(fn);
  Py_DECREF(pArgs); // Also decrements py_image_path
  Py_DECREF(mod);
}

#endif // TESTPYTHON_H
