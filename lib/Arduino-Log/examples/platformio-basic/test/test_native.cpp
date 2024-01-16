#include "ArduinoLog.h"
#include <Arduino.h>
#include <bitset>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <unity.h>

using namespace fakeit;
std::stringstream output_;

void reset_output() {
  output_.str(std::string());
  output_.clear();
}

std::string decimal_to_binary(int n) {

  std::string binary;
  while (n != 0) {
    binary = (n % 2 == 0 ? "0" : "1") + binary;
    n /= 2;
  }
  return binary;
}

void TEST_ASSERT_EQUAL_STRING_STREAM(std::stringstream const &expected_output,
                                     std::stringstream const &actual_output) {

  TEST_ASSERT_EQUAL_STRING(expected_output.str().c_str(),
                           actual_output.str().c_str());
}

void set_up_logging_captures() {
  When(OverloadedMethod(ArduinoFake(Serial), println, size_t(void)))
      .AlwaysDo([&]() -> int {
        output_ << "\n";
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print, size_t(char)))
      .AlwaysDo([&](const char x) -> int {
        output_ << x;
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print, size_t(const char[])))
      .AlwaysDo([&](const char x[]) -> int {
        output_ << x;
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print, size_t(const String &)))
      .AlwaysDo([&](const String &x) -> int {
        output_ << x.c_str();
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print, size_t(unsigned char, int)))
      .AlwaysDo([&](unsigned char x, int y) -> int {
        if (y == 2) {
          output_ << decimal_to_binary(x);
        } else {
          output_ << std::setbase(y) << (unsigned long)x;
        }
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print, size_t(unsigned long, int)))
      .AlwaysDo([&](unsigned long x, int y) -> int {
        output_ << std::fixed << std::setprecision(y) << x;
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print, size_t(int, int)))
      .AlwaysDo([&](int x, int y) -> int {
        if (y == 2) {
          output_ << decimal_to_binary(x);
        } else {
          output_ << std::setbase(y) << x;
        }
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print, size_t(unsigned int, int)))
      .AlwaysDo([&](unsigned int x, int y) -> int {
        if (y == 2) {
          output_ << decimal_to_binary(x);
        } else {
          output_ << std::setbase(y) << x;
        }
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print, size_t(double, int)))
      .AlwaysDo([&](double x, int y) -> int {
        output_ << std::fixed << std::setprecision(y) << x;
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print, size_t(long, int)))
      .AlwaysDo([&](long x, int y) -> int {
        if (y == 2) {
          output_ << std::bitset<sizeof(long)>(x).to_string();
        } else {
          output_ << std::setbase(y) << x;
        }
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print, size_t(unsigned long, int)))
      .AlwaysDo([&](unsigned long x, int y) -> int {
        if (y == 2) {
          output_ << std::bitset<sizeof(double)>(x).to_string();
        } else {
          output_ << std::setbase(y) << x;
        }
        return 1;
      });
  When(OverloadedMethod(ArduinoFake(Serial), print,
                        size_t(const __FlashStringHelper *ifsh)))
      .AlwaysDo([&](const __FlashStringHelper *x) -> int {
        auto message = reinterpret_cast<const char *>(x);
        output_ << message;
        return 1;
      });

  When(Method(ArduinoFake(Serial), flush)).AlwaysReturn();
}

void setUp(void) {
  ArduinoFakeReset();
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
  set_up_logging_captures();
}
void test_int_values() {
  reset_output();
  int int_value1 = 173;
  int int_value2 = 65536;
  Log.notice("Log as Info with integer values : %d, %d" CR, int_value1,
             int_value2);
  std::stringstream expected_output;
  expected_output << "N: Log as Info with integer values : 173, 65536\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void test_int_hex_values() {
  reset_output();
  int int_value1 = 152;
  int int_value2 = 65010;
  Log.notice(F("Log as Info with hex values     : %x, %X" CR), int_value1,
             int_value1);
  Log.notice("Log as Info with hex values     : %x, %X" CR, int_value2,
             int_value2);
  std::stringstream expected_output;
  expected_output << "N: Log as Info with hex values     : 98, 0x0098\n"
                  << "N: Log as Info with hex values     : fdf2, 0xfdf2\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void test_int_binary_values() {
  reset_output();
  int int_value1 = 2218;
  int int_value2 = 17814;
  Log.notice(F("Log as Info with binary values  : %b, %B" CR), int_value1,
             int_value2);
  std::stringstream expected_output;
  expected_output << "N: Log as Info with binary values  : 100010101010, "
                     "0b100010110010110\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}
void test_long_values() {
  reset_output();
  long long_value1 = 34359738368;
  long long_value2 = 274877906944;
  Log.notice(F("Log as Info with long values    : %l, %l" CR), long_value1,
             long_value2);
  std::stringstream expected_output;
  expected_output << "N: Log as Info with long values    : "
                     "34359738368, 274877906944\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void test_bool_values() {
  reset_output();
  bool true_value = true;
  bool false_value = false;
  Log.notice("Log as Info with bool values    : %t, %T" CR, true_value,
             true_value);
  Log.notice("Log as Info with bool values    : %t, %T" CR, false_value,
             false_value);
  std::stringstream expected_output;
  expected_output << "N: Log as Info with bool values    : T, true\n";
  expected_output << "N: Log as Info with bool values    : F, false\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void test_char_string_values() {
  reset_output();
  const char *charArray = "this is a string";
  Log.notice(F("Log as Info with string value   : %s" CR), charArray);
  std::stringstream expected_output;
  expected_output << "N: Log as Info with string value   : this is a string\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void test_flash_string_values() {
  reset_output();
  //__FlashStringHelper cannot be declared outside a function
  const __FlashStringHelper *flashCharArray2 = F("this is a string");

  const char flashCharArray1[] PROGMEM = "this is a string";
  Log.notice("Log as Info with Flash string value   : %S" CR, flashCharArray1);
  std::stringstream expected_output;
  expected_output
      << "N: Log as Info with Flash string value   : this is a string\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void test_string_values() {
  reset_output();
  String stringValue1 = "this is a string";
  Log.notice("Log as Info with string value   : %s" CR, stringValue1.c_str());
  std::stringstream expected_output;
  expected_output << "N: Log as Info with string value   : this is a string\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void test_float_values() {
  reset_output();
  float float_value = 12.34;
  Log.notice(F("Log as Info with float value   : %F" CR), float_value);
  Log.notice("Log as Info with float value   : %F" CR, float_value);
  std::stringstream expected_output;
  expected_output << "N: Log as Info with float value   : 12.34\n"
                  << "N: Log as Info with float value   : 12.34\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void test_double_values() {
  reset_output();
  double double_value = 1234.56789;
  // Log.notice(F("%D" CR), double_value);
  Log.notice(F("Log as Info with double value   : %D" CR), double_value);
  Log.notice("Log as Info with double value   : %D" CR, double_value);
  std::stringstream expected_output;
  expected_output << "N: Log as Info with double value   : 1234.57\n"
                  << "N: Log as Info with double value   : 1234.57\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void test_mixed_values() {
  reset_output();
  int int_value1 = 15826;
  int int_value2 = 31477;
  long long_value1 = 274877906944;
  long long_value2 = 68719476743;
  bool true_value = true;
  bool false_value = false;
  Log.notice(F("Log as Debug with mixed values  : %d, %d, %l, %l, %t, %T" CR),
             int_value1, int_value2, long_value1, long_value2, true_value,
             false_value);
  std::stringstream expected_output;
  expected_output
      << "N: Log as Debug with mixed values  : 15826, 31477, 274877906944, "
         "68719476743, T, false\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void test_log_levels() {
  reset_output();
  bool true_value = true;
  bool false_value = false;
  Log.trace("Log as Trace with bool value    : %T" CR, true_value);
  Log.traceln("Log as Trace with bool value    : %T", false_value);
  Log.warning("Log as Warning with bool value  : %T" CR, true_value);
  Log.warningln("Log as Warning with bool value  : %T", false_value);
  Log.error("Log as Error with bool value    : %T" CR, true_value);
  Log.errorln("Log as Error with bool value    : %T", false_value);
  Log.fatal("Log as Fatal with bool value    : %T" CR, true_value);
  Log.fatalln("Log as Fatal with bool value    : %T", false_value);
  Log.verboseln(F("Log as Verbose with bool value   : %T"), true_value);
  Log.verbose(F("Log as Verbose with bool value   : %T" CR), false_value);
  std::stringstream expected_output;
  expected_output << "T: Log as Trace with bool value    : true\n"
                     "T: Log as Trace with bool value    : false\n"
                     "W: Log as Warning with bool value  : true\n"
                     "W: Log as Warning with bool value  : false\n"
                     "E: Log as Error with bool value    : true\n"
                     "E: Log as Error with bool value    : false\n"
                     "F: Log as Fatal with bool value    : true\n"
                     "F: Log as Fatal with bool value    : false\n"
                     "V: Log as Verbose with bool value   : true\n"
                     "V: Log as Verbose with bool value   : false\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

void printTimestamp(Print *_logOutput) {
  char c[12];
  int m = sprintf(c, "%10lu ", millis());
  _logOutput->print(c);
}
void printCarret(Print *_logOutput) { _logOutput->print('>'); }

void test_prefix_and_suffix() {
  reset_output();
  When(Method(ArduinoFake(), millis)).Return(1026);
  Log.setPrefix(printTimestamp); // set timestamp as prefix
  Log.setSuffix(printCarret);    // set carret as suffix
  Log.verboseln(F("Log with suffix & prefix"));
  Log.setPrefix(NULL); // clear prefix
  Log.setSuffix(NULL); // clear suffix
  std::stringstream expected_output;
  expected_output << "      1026 V: Log with suffix & prefix>\n";
  TEST_ASSERT_EQUAL_STRING_STREAM(expected_output, output_);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_int_values);
  RUN_TEST(test_int_hex_values);
  RUN_TEST(test_int_binary_values);
  RUN_TEST(test_long_values);
  RUN_TEST(test_bool_values);
  RUN_TEST(test_char_string_values);
  RUN_TEST(test_flash_string_values);
  RUN_TEST(test_string_values);
  RUN_TEST(test_float_values);
  RUN_TEST(test_double_values);
  RUN_TEST(test_mixed_values);
  RUN_TEST(test_log_levels);
  RUN_TEST(test_prefix_and_suffix);
  UNITY_END();
}