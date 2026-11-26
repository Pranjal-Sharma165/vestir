#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

inline int failures = 0;

inline void check(bool condition, const std::string &message) {
  if (condition) {
    std::cout << "PASS: " << message << "\n";
  } else {
    std::cout << "FAIL: " << message << "\n";
    ++failures;
  }
}

inline std::string readSchema() {
  std::ifstream file(SCHEMA_PATH);
  std::stringstream text;
  text << file.rdbuf();
  return text.str();
}
