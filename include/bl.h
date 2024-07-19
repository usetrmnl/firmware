#ifndef BL_H
#define BL_H

enum SPECIAL_FUNCTION {
  SF_NONE,
  SF_IDENTIFY,
  SF_SLEEP,
  SF_ADD_WIFI,
  SF_RESTART_PLAYLIST,
  SF_REWIND,
  SF_SEND_TO_ME,
};

/**
 * @brief Function to init business logic module
 * @param none
 * @return none
 */
void bl_init(void);

/**
 * @brief Function to process business logic module
 * @param none
 * @return none
 */
void bl_process(void);

#endif