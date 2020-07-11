
#ifndef WifiInterface_h
#define WifiInterface_h
#include "..\CommInterface\CommInterface.h"
#include "MemStream.h"
#include <Arduino.h>
#include <avr/pgmspace.h>
#ifdef defined(AVR_UNO)
#include <SoftwareSerial.h>
#else
#include <HardwareSerial.h>
#endif

class WifiInterface : public CommInterface
{

public:
  WifiInterface(Stream &wifiStream, const __FlashStringHelper *SSSid, const __FlashStringHelper *password, const __FlashStringHelper *hostname, const __FlashStringHelper *servername, int port);
  void process();
  void showConfiguration();
  void showInitInfo();
  void send(const char *buf);
  Stream *getStream() { return &wifiStream; }

private:
  static bool setup2(const __FlashStringHelper *SSSid, const __FlashStringHelper *password, const __FlashStringHelper *hostname, const __FlashStringHelper *servername, int port);
  static bool checkForOK(const unsigned int timeout, const char *waitfor, bool echo);
  static bool isHTML();
  static bool connected;
  static byte loopstate;
  static int datalength;
  static int connectionId;
  static const byte MAX_WIFI_BUFFER = 250;
  static char buffer[MAX_WIFI_BUFFER];
  static MemStream streamer;
#ifdef defined(AVR_UNO)
  SoftwareSerial &wifiStream;
#else
  HardwareSerial &wifiStream;
#endif
};

#endif
