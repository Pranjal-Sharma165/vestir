#ifndef TESTPYTHON_H
#define TESTPYTHON_H

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
    throw std::runtime_error(
        std::format("Python init failed: %s\n",
                    status.err_msg ? status.err_msg : "unknown"));
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

  PyObject *fn = PyObject_GetAttrString(mod, "test");

  if (!fn || !PyCallable_Check(fn)) {
    PyErr_Print();
    throw std::runtime_error("");
  }

  PyObject *pArgs = PyTuple_New(1);
  PyTuple_SetItem(pArgs, 0, PyUnicode_FromString("test string from C++"));
  PyObject *result = PyObject_CallObject(fn, pArgs);
  Py_DECREF(pArgs);
  Py_DECREF(fn);
  Py_DECREF(mod);

  if (!result) {
    PyErr_Print();
    throw std::runtime_error("");
  }

  if (!PyUnicode_Check(result)) {
    PyErr_Print();
    Py_DECREF(result);
    throw std::runtime_error("");
  }

  auto resultStr = PyUnicode_AsUTF8(result);
  Py_DECREF(result);
  std::cout << "Result: " << resultStr << std::endl;
}

#endif // TESTPYTHON_H
