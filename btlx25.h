#ifndef btlx25_h
#define btlx25_h

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include <EEPROM.h> //https://github.com/esp8266/Arduino/blob/master/libraries/EEPROM/EEPROM.h
#include <NTPClient.h> // install NTPClient from manage libraries
#include <Ticker.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager

/*
static const uint8_t D0   = 16;
static const uint8_t D1   = 5;
static const uint8_t D2   = 4;
static const uint8_t D3   = 0;
static const uint8_t D4   = 2;
static const uint8_t D5   = 14;
static const uint8_t D6   = 12;
static const uint8_t D7   = 13;
static const uint8_t D8   = 15;
static const uint8_t D9   = 3;
static const uint8_t D10  = 1;
*/

#define MAX_TICKERS 20

// for debug
#define BTLX_DEBUG_WIFI   0b00000001
#define BTLX_DEBUG_SERIAL 0b00000010
#define BTLX_DEBUG_ALL    0b00000100

// Boots in EEPROM
#define BOOTS_ADDRESS 0  // 2 bytes. Next to use is 0x2
#define EEPROM_SIZE 1024
			 
class App;

typedef void (*function_callback)();
typedef void (App::*AppCallback)();

#define USER_TIMER 0
#define APP_TIMER 1

class Timer{
    public:
	int millis;
	AppCallback appFunction;
	function_callback  function;
	char* name;
	int type;
	unsigned long lastRun;
	Timer* next;

	Timer();
};

class Log{
    public:
        App* app;
        const char* server;
        const char* ID;
        char* IP;

        Log(App* app, const char* ID, const char* server);
        void log(char* msg);
	void setIP(char* IP);

};

class App{
    public:

    const char* ID = NULL;
    const char* log_server = NULL;
    const char* SSID = NULL;
    const char* password = NULL;
    char IP[16];
    unsigned long tLastConnectionAttempt = 0;
    unsigned long tConnect = 0;

    // for NTP
    unsigned long tEpoch = 0;
    unsigned long tEpochOffset = 0;

    unsigned short num_tickers = 0;
    Ticker* tickers[20];

    unsigned short addTicker(float secs, void (App::*callback)());

    Timer* timers = NULL;
    Log* logger;
    App(const char* ID, const char* server, int control_led);
    WiFiManager *wifiManager = NULL;

    int controlLed = 16; // default control led
    int controlLedFreq = 500; // no blink by default
    
    void initNTP();
    void addTimer(int millis, AppCallback function, char* name);
    void addTimer(int millis, function_callback function, char* name);
    void attendTimers();
    void imAlive();
    void log(char* msg);
    void debug(char* level, unsigned short int channels, char* message);
    bool send(String what);
    String get(String what);
    void handleOTA();
    bool startWiFiManager();
    void resetWIFI();
    void updateNTP();
    unsigned long getEpochSeconds();
    unsigned short readBoots();
    unsigned short incBoots();
    void resetBoots();
    char* millis_to_human(unsigned long millis);
    void checkConnection();
    int resetEEPROM(int start, int size);
    void handleControlLed();
    void blinkControlLed(int freq);
};

#endif
