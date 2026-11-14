#ifndef TESTPYTHON_H
#define TESTPYTHON_H

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

#include <cstdio>
#include <string>

static void runPythonTest() {

  PyConfig config;
  PyConfig_InitIsolatedConfig(&config);

  PyStatus status =
      PyConfig_SetBytesString(&config, &config.home, PYTHON_HOME_DIR);

  if (!PyStatus_Exception(status)) {
    status = Py_InitializeFromConfig(&config);
  }

  PyConfig_Clear(&config);
  if (PyStatus_Exception(status)) {
    std::fprintf(stderr, "Python init failed: %s\n",
                 status.err_msg ? status.err_msg : "unknown");
    return;
  }

  std::string cmd =
      std::string("import sys; sys.path.insert(0, r'") + APP_PYTHON_DIR + "')";
  int rc = PyRun_SimpleString(cmd.c_str());

  if (rc != 0) {
    std::fprintf(stderr, "Python sys.path init failed\n");
    return;
  }

  FILE *fp = std::fopen(APP_PYTHON_DIR "/test.py", "rb");
  if (fp) {
    rc = PyRun_SimpleFile(fp, "test.py");
    std::fclose(fp);
  } else {
    perror("fopen");
    return;
  }

  if (rc != 0) {
    std::fprintf(stderr, "Python sys.path init failed\n");
    return;
  }

  if (Py_FinalizeEx() < 0) {
    std::fprintf(stderr, "Python finalize failed\n");
    return;
  }
}

#endif // TESTPYTHON_H
