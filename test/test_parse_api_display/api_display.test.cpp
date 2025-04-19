#include <unity.h>
#include <bmp.h>
#include <api_response_parsing.h>

void assert_clock_settings_equal(clock_settings_t* cs1, clock_settings_t* cs2)
{
  TEST_ASSERT_EQUAL(cs1->Xstart,   cs2->Xstart);
  TEST_ASSERT_EQUAL(cs1->Xend,     cs2->Xend);
  TEST_ASSERT_EQUAL(cs1->Ystart,   cs2->Ystart);
  TEST_ASSERT_EQUAL(cs1->Yend,     cs2->Yend);
  TEST_ASSERT_EQUAL(cs1->ColorFg,  cs2->ColorFg);
  TEST_ASSERT_EQUAL(cs1->ColorBg,  cs2->ColorBg);
  TEST_ASSERT_EQUAL(cs1->FontSize, cs2->FontSize);
  TEST_ASSERT_EQUAL_STRING(cs1->Format, cs2->Format);
}

void assert_response_equal(ApiDisplayResponse expected, ApiDisplayResponse actual)
{
  TEST_ASSERT_EQUAL(expected.outcome, actual.outcome);
  TEST_ASSERT_EQUAL_STRING(expected.image_url.c_str(), actual.image_url.c_str());
  TEST_ASSERT_EQUAL(expected.update_firmware, actual.update_firmware);
  TEST_ASSERT_EQUAL_STRING(expected.firmware_url.c_str(), actual.firmware_url.c_str());
  TEST_ASSERT_EQUAL_UINT64(expected.refresh_rate, actual.refresh_rate);
  TEST_ASSERT_EQUAL(expected.reset_firmware, actual.reset_firmware);
  TEST_ASSERT_EQUAL(expected.special_function, actual.special_function);
  TEST_ASSERT_EQUAL_STRING(expected.action.c_str(), actual.action.c_str());
  assert_clock_settings_equal(&expected.clock_settings, &actual.clock_settings);
}

void test_parseResponse_apiDisplay_success(void)
{
  String input = "{\"status\":200,\"image_url\":\"http://example.com/foo.bmp\",\"filename\":\"empty_state\",\"update_firmware\":true,\"firmware_url\":\"https://example.com/firmware.bin\",\"refresh_rate\":123456,\"reset_firmware\":true,\"special_function\":\"identify\",\"action\":\"special_action\"}";

  ApiDisplayResponse expected = {
      .outcome = ApiDisplayOutcome::Ok,
      .image_url = "http://example.com/foo.bmp",
      .filename = "empty_state",
      .update_firmware = true,
      .firmware_url = "https://example.com/firmware.bin",
      .refresh_rate = 123456,
      .reset_firmware = true,
      .special_function = SPECIAL_FUNCTION::SF_IDENTIFY,
      .action = "special_action",
      .clock_settings = {0}};

  assert_response_equal(expected, parseResponse_apiDisplay(input));
}

void test_parseResponse_apiDisplay_deserializationError(void)
{
  String input = "invalid json";

  ApiDisplayResponse expected = {
      .outcome = ApiDisplayOutcome::DeserializationError};

  assert_response_equal(expected, parseResponse_apiDisplay(input));
}

void test_parseResponse_apiDisplay_missing_fields(void)
{
  String input = "{}";

  ApiDisplayResponse expected = {
      .outcome = ApiDisplayOutcome::Ok,
      .image_url = "",
      .update_firmware = false,
      .firmware_url = "",
      .refresh_rate = 0,
      .reset_firmware = false,
      .special_function = SPECIAL_FUNCTION::SF_NONE,
      .action = "",
      .clock_settings = {0}};

  assert_response_equal(expected, parseResponse_apiDisplay(input));
}

void test_parseResponse_apiDisplay_extra_fields(void)
{
  String input = "{\"extra_field\":5712}";

  ApiDisplayResponse expected = {
      .outcome = ApiDisplayOutcome::Ok,
      .image_url = "",
      .update_firmware = false,
      .firmware_url = "",
      .refresh_rate = 0,
      .reset_firmware = false,
      .special_function = SPECIAL_FUNCTION::SF_NONE,
      .action = "",
      .clock_settings = {0}};

  assert_response_equal(expected, parseResponse_apiDisplay(input));
}

void test_parseResponse_apiDisplay_treats_unknown_sf_as_none(void)
{
  String input = "{\"status\":200,\"image_url\":\"http://example.com/foo.bmp\",\"update_firmware\":true,\"firmware_url\":\"https://example.com/firmware.bin\",\"refresh_rate\":123456,\"reset_firmware\":true,\"special_function\":\"unknown-string\",\"action\":\"special_action\"}";

  auto parsed = parseResponse_apiDisplay(input);
  TEST_ASSERT_EQUAL(parsed.special_function, SPECIAL_FUNCTION::SF_NONE);
}

void test_parseResponse_apiDisplay_clock_setings(void)
{
  String input = "{\"status\":200,\"image_url\":\"http://example.com/foo.bmp\",\"filename\":\"empty_state\",\"update_firmware\":true,\"firmware_url\":\"https://example.com/firmware.bin\",\"refresh_rate\":123456,\"reset_firmware\":true,\"special_function\":\"identify\",\"action\":\"special_action\""
                 ",\"clock_settings\":{\"Xstart\":10,\"Ystart\":11,\"Xend\":12,\"Yend\":13,\"ColorFg\":255,\"ColorBg\":1,\"FontSize\":24,\"Format\":\"%H:%M\"}"
                 "}";

  ApiDisplayResponse expected = {
      .outcome = ApiDisplayOutcome::Ok,
      .image_url = "http://example.com/foo.bmp",
      .filename = "empty_state",
      .update_firmware = true,
      .firmware_url = "https://example.com/firmware.bin",
      .refresh_rate = 123456,
      .reset_firmware = true,
      .special_function = SPECIAL_FUNCTION::SF_IDENTIFY,
      .action = "special_action",
      .clock_settings = {
        .Xstart = 10,
        .Xend = 12,
        .Ystart = 11,
        .Yend = 13,
        .ColorFg = 255,
        .ColorBg = 1,
        .FontSize = 24,
        .Format = "%H:%M",
      }};

  assert_response_equal(expected, parseResponse_apiDisplay(input));
}

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void process()
{
  UNITY_BEGIN();
  RUN_TEST(test_parseResponse_apiDisplay_success);
  RUN_TEST(test_parseResponse_apiDisplay_deserializationError);
  RUN_TEST(test_parseResponse_apiDisplay_treats_unknown_sf_as_none);
  RUN_TEST(test_parseResponse_apiDisplay_missing_fields);
  RUN_TEST(test_parseResponse_apiDisplay_extra_fields);
  RUN_TEST(test_parseResponse_apiDisplay_clock_setings);
  UNITY_END();
}

int main(int argc, char **argv)
{
  process();
  return 0;
}