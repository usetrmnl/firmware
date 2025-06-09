enum ButtonPressResult
{
  LongPress,
  DoubleClick,
  NoAction,
  SoftReset
};
extern const char *ButtonPressResultNames[3];

ButtonPressResult read_button_presses();