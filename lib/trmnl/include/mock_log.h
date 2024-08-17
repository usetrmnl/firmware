
#pragma once

/** Empty reimplementation of ArduinoLog for use by tests */
class Logging
{
public:
  Logging() {}

  template <class T>
  void fatal(T msg, ...) {}

  template <class T>
  void error(T msg, ...) {}

  template <class T>
  void info(T msg, ...) {}
};

Logging Log;