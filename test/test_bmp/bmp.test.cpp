#include <unity.h>
#include <bmp.h>

void test_parseBMPHeader_NOT_BMP(void)
{
  bool image_reverse = false;
  uint8_t input[2] = {0, 0};
  TEST_ASSERT_EQUAL(BMP_NOT_BMP, parseBMPHeader(input, image_reverse));
}

void test_parseBMPHeader_BMP_BAD_SIZE(void)
{
  bool image_reverse = false;
  uint8_t input[2] = {'B', 'M'};
  TEST_ASSERT_EQUAL(BMP_BAD_SIZE, parseBMPHeader(input, image_reverse));
}

void process()
{
  UNITY_BEGIN();
  RUN_TEST(test_parseBMPHeader_NOT_BMP);
  RUN_TEST(test_parseBMPHeader_BMP_BAD_SIZE);
  UNITY_END();
}

int main(int argc, char **argv)
{
  process();
  return 0;
}