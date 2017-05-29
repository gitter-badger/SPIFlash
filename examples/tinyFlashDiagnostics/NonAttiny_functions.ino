/*
  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
  |                                                            NonAttiny_functions.ino                                                            |
  |                                                               SPIFlash library                                                                |
  |                                                                   v 2.7.0                                                                     |
  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
  |                                                                    Marzogh                                                                    |
  |                                                                  26.04.2017                                                                   |
  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
  |                                                                                                                                               |
  |                                  For a full diagnostics rundown - with error codes and details of the errors                                  |
  |                                uncomment #define RUNDIAGNOSTIC in SPIFlash.cpp in the library before compiling                                |
  |                                             and loading this application onto your Arduino.                                                   |
  |                                                                                                                                               |
  |~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|
*/

#if !defined (__AVR_ATtiny85__)
void startup(void) {
  Serial.begin(BAUD_RATE);
#if defined (ARDUINO_ARCH_SAMD) || (__AVR_ATmega32U4__)
  while (!Serial) ; // Wait for Serial monitor to open
#endif
  Serial.print(F("Initialising Flash memory"));
  for (int i = 0; i < 10; ++i)
  {
    Serial.print(F("."));
  }
  Serial.println();
#if defined (CHIPSIZE)
  flash.begin(CHIPSIZE); //use flash.begin(CHIPSIZE) if using non-Winbond flash (Refer to '#define CHIPSIZE' above)
#else
  flash.begin();
#endif
  Serial.println();
  Serial.println();

#if defined (ARDUINO_ARCH_ESP32)
  randomSeed(65535537);
#else
  randomSeed(analogRead(RANDPIN));
#endif
}

void clearprintBuffer(char *bufPtr)
{
  for (uint8_t i = 0; i < 128; i++) {
    //printBuffer[i] = 0;
    *bufPtr++ = 0;
  }
}

void printLine() {
  for (uint8_t i = 0; i < 230; i++) {
    Serial.print(F("-"));
  }
  Serial.println();
}

void printPass() {
  Serial.print(F("Pass"));
}

void printFail() {
  Serial.print(F("Fail"));
}

void printTab(uint8_t a, uint8_t b) {
  for (uint8_t i = 0; i < a; i++) {
    Serial.print(F("\t"));
  }
  if (b > 0) {
    Serial.print("||");
    for (uint8_t i = 0; i < b; i++) {
      Serial.print(F("\t"));
    }
  }
}

void printTime(uint32_t _wTime, uint32_t _rTime) {
  printTab(2, 1);
  printTimer(_wTime);
  printTab(2, 1);
  printTimer(_rTime);
}

void printTimer(uint32_t _us) {

  if (_us > 1000000) {
    float _s = _us / (float)1000000;
    Serial.print(_s, 4);
    Serial.print(" s");
  }
  else if (_us > 10000) {
    float _ms = _us / (float)1000;
    Serial.print(_ms, 4);
    Serial.print(" ms");
  }
  else {
    Serial.print(_us);
    Serial.print(F(" us"));
  }
}

void getID() {
  char printBuffer[128];
  printLine();
  for (uint8_t i = 0; i < 68; i++) {
    Serial.print(F(" "));
  }
  Serial.print(F("SPIFlash Library version"));
#ifdef LIBVER
  uint8_t _ver, _subver, _bugfix;
  flash.libver(&_ver, &_subver, &_bugfix);
  clearprintBuffer(&printBuffer[1]);
  sprintf(printBuffer, ": %d.%d.%d", _ver, _subver, _bugfix);
  Serial.println(printBuffer);
#else
  Serial.println(F("< 2.5.0"));
#endif
  printLine();

  for (uint8_t i = 0; i < 80; i++) {
    Serial.print(F(" "));
  }
  Serial.println(F("Get ID"));
  printLine();
  uint8_t b1, b2;
  uint16_t b3;
  uint32_t JEDEC = flash.getJEDECID();
  uint32_t maxPage = flash.getMaxPage();
  uint32_t capacity = flash.getCapacity();
  b1 = (JEDEC >> 16);
  b2 = (JEDEC >> 8);
  b3 = (JEDEC >> 0);


  printLine();
  //---------------------------------------------------------------------------------------------//

  clearprintBuffer(&printBuffer[1]);
  sprintf(printBuffer, "\t\t\tJEDEC ID: %04lxh", JEDEC);
  Serial.println(printBuffer);
  clearprintBuffer(&printBuffer[1]);
  sprintf(printBuffer, "\t\t\tManufacturer ID: %02xh\n\t\t\tMemory Type: %02xh\n\t\t\tCapacity: %lu bytes\n\t\t\tMaximum pages: %lu", b1, b2, capacity, maxPage);
  Serial.println(printBuffer);
}

bool checkPage(uint8_t *data_buffer) {
  for (int i = 0; i < 256; i++) {
    if (data_buffer[i] != i)
      return false;
  }
  return true;
}

void printHeader(uint8_t _t) {
  if (_t == DATA_FUNCTION && data_header == false) {
    printLine();

    for (uint8_t i = 0; i < 79; i++) {
      Serial.print(F(" "));
    }
    Serial.println(F("Data Check"));
    printLine();

    Serial.println(F("\tData Type\t||\tWrite Check\t||\tRead Check\t"));
    printLine();

    data_header = true;
  }
  if (_t == OTHER_FUNCTION && power_header == false) {
    printLine();

    for (uint8_t i = 0; i < 74; i++) {
      Serial.print(F(" "));
    }
    Serial.println(F("Other Function Check"));
    printLine();

    Serial.println(F("\tTest Type\t||\tStatus"));
    printLine();

    power_header = true;
  }
}
void diagnose() {
  getID();

  uint16_t _stat;
  if (prevWritten()) {
    Serial.println(dataPacket.test, BIN);
    if (dataPacket.test & INT) {
      printHeader(DATA_FUNCTION);
      intDiag();
    }
    if (dataPacket.test & FLOAT) {
      printHeader(DATA_FUNCTION);
      floatDiag();
    }
    if (dataPacket.test & STRING) {
      printHeader(DATA_FUNCTION);
      stringDiag();
    }
    if (dataPacket.test & STRUCT) {
      printHeader(DATA_FUNCTION);
      structDiag();
    }
    if (dataPacket.test & ARRAY) {
      printHeader(DATA_FUNCTION);
      arrayDiag();
    }
    if (dataPacket.test & ERASE) {
      printHeader(OTHER_FUNCTION);
      eraseDiag();
    }
    if (dataPacket.test & POWER) {
      printHeader(OTHER_FUNCTION);
      powerDiag();
    }
  }
  else {
    Serial.println(F("No data found on chip. Please run this sketch on a an ATTiny85 connected to this flash chip first"));
  }
}

void intDiag(void) {
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                       Integer                                                                       //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(1, 0);
  Serial.print(F("Integer"));
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                        Write                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(2, 1);
  if (dataPacket.Status & iW) {
    printPass();
  }
  else {
    printFail();
  }
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                         Read                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(2, 1);
  if (dataPacket.Status & iR) {
    printPass();
  }
  else {
    printFail();
  }
  Serial.println();
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //-----------------------------------------------------------------------------------------------------------------------------------------------------//
}

void floatDiag(void) {
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                       Float                                                                         //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(1, 0);
  Serial.print(F("Float"));
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                        Write                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(2, 1);
  if (dataPacket.Status & fW) {
    printPass();
  }
  else {
    printFail();
  }
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                         Read                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(2, 1);
  if (dataPacket.Status & fR) {
    printPass();
  }
  else {
    printFail();
  }
  Serial.println();
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //-----------------------------------------------------------------------------------------------------------------------------------------------------//
}

void stringDiag(void) {
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                       Integer                                                                       //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(1, 0);
  Serial.print(F("String"));
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                        Write                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(2, 1);
  if (dataPacket.Status & sgW) {
    printPass();
  }
  else {
    printFail();
  }
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                         Read                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(2, 1);
  if (dataPacket.Status & sgR) {
    printPass();
  }
  else {
    printFail();
  }
  Serial.println();
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //-----------------------------------------------------------------------------------------------------------------------------------------------------//
}

void structDiag(void) {
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                       Integer                                                                       //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(1, 0);
  Serial.print(F("Integer"));
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                        Write                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(2, 1);
  if (dataPacket.Status & scW) {
    printPass();
  }
  else {
    printFail();
  }
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                         Read                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(2, 1);
  if (dataPacket.Status & scR) {
    printPass();
  }
  else {
    printFail();
  }
  Serial.println();
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //-----------------------------------------------------------------------------------------------------------------------------------------------------//
}

void arrayDiag(void) {
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                       Integer                                                                       //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(1, 0);
  Serial.print(F("Array"));
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                        Write                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(2, 1);
  if (dataPacket.Status & aW) {
    printPass();
  }
  else {
    printFail();
  }
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                         Read                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(2, 1);
  if (dataPacket.Status & aR) {
    printPass();
  }
  else {
    printFail();
  }
  Serial.println();
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //-----------------------------------------------------------------------------------------------------------------------------------------------------//
}

void eraseDiag(void) {
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                   Erase Block                                                                       //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(1, 0);
  Serial.print(F("Erase Block"));
  printTab(2, 1);
  if (dataPacket.Status & eB) {
    printPass();
  }
  else {
    printFail();
  }
  Serial.println();
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                    Erase Chip                                                                       //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(1, 0);
  Serial.print(F("Erase Chip"));
  printTab(2, 1);
  if (dataPacket.Status & eC) {
    printPass();
  }
  else {
    printFail();
  }
  Serial.println();
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //-----------------------------------------------------------------------------------------------------------------------------------------------------//
}

void powerDiag(void) {
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                    Power on                                                                         //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(1, 0);
  Serial.print(F("Power off"));
  printTab(2, 1);
  if (dataPacket.Status & pOFF) {
    printPass();
  }
  else {
    printFail();
  }
  Serial.println();
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  //                                                                    Power off                                                                        //
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  printTab(1, 0);
  Serial.print(F("Power on"));
  printTab(2, 1);
  if (dataPacket.Status & pON) {
    printPass();
  }
  else {
    printFail();
  }
  Serial.println();
  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //-----------------------------------------------------------------------------------------------------------------------------------------------------//
}
#endif
