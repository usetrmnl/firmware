enum ButtonPressResult
{
  LongPress,
  DoubleClick,
  NoAction
};
extern const char *ButtonPressResultNames[3];

ButtonPressResult read_button_presses();