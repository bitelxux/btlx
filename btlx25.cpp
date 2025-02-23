#include <Arduino.h>
#include <ESP8266WebServer.h>

#include "btlx25.h"

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

Timer::Timer(){
}

Log::Log(App* app, const char* ID, const char* server){
    this->app = app;
    this->ID = ID;
    this->IP = "no IP";
    this->server = server;
}

void Log::log(char *msg){
    char buffer[1024];

    // some tweak in the case that we are sending the log to a log server

    if (this->server && this->server[0] != '\0'){
        sprintf(buffer, "[%s][%s] %s", this->ID, this->IP, msg);
	String strMsg = buffer;
        strMsg.replace(" ", "%20");
        strMsg.replace("/", "%2F");

	// complete the request
        sprintf(buffer, "%s/log/%s", this->server, strMsg.c_str());
        String toSend = buffer;
        this->app->send(toSend);
    }
    else // simple print on Serial console
    {
        sprintf(buffer, "[%s][%s]: %s", this->ID, this->IP, msg);
        Serial.println(buffer);
    }
}

void Log::setIP(char* IP) {
    this->IP = IP;
}

void App::imAlive(){
  this->logger->log("I'm alive !");
}

App::App(const char* ID, const char* log_server, int controlLed){
    this->ID = ID;
    this->log_server = log_server;
    this->SSID = SSID;
    this->password = password;
    this->timers = NULL;
    this->logger = new Log(this, this->ID, this->log_server);
    this->controlLed = controlLed;
    this->controlLedFreq = 0; // off 

    for (int i=0; i<MAX_TICKERS; i++) {
        this->tickers[i] = NULL;
    }		

    this->addTimer(60000, &App::imAlive, "imAlive");
    this->addTimer(1000, &App::handleOTA, "handleOTA");
    this->addTimer(60 * 1000, &App::updateNTP, "updateNTP");

    // handleControl is a ticker because we want the led to operate even when the code
    // is stuck at some point (such as wifi-configuration)
    // rest of timers are regular ones managed by attendTimers because some take very long
    // and the can not be tickers -they would restart the board-
    this->addTicker(0.001, &App::handleControlLed);

    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    this->wifiManager = new WiFiManager();

}

void App::blinkControlLed(int freq) {
    this->controlLedFreq = freq;
}

void App::resetWIFI() {
    WiFi.disconnect(true);
    delay(1000);
    this->wifiManager->resetSettings();
}

bool App::startWiFiManager(){
  //this->wifiManager->resetSettings();

  char passwd[20];

  if (!this->wifiManager) {
	  Serial.println("wifiManager is NULL");
	  return false;
  }

  strcpy(passwd, this->wifiManager->getWiFiPass().c_str());

  Serial.print("SSID is [");
  Serial.print(WiFi.SSID());
  Serial.println("]");
  
  Serial.print("Password is [");
  Serial.print(passwd);
  Serial.println("]");

  if (WiFi.SSID().length() > 0){
    this->blinkControlLed(500);
    WiFi.begin(WiFi.SSID(), passwd);
  } 
  else {
    this->blinkControlLed(100);
    this->wifiManager->autoConnect(this->ID);
    this->blinkControlLed(0);
  }

  for (int i=0; i<30; i++){
    if (WiFi.status() != WL_CONNECTED) {
       digitalWrite(this->controlLed, !digitalRead(this->controlLed));
       Serial.print("Wifi not connected [");
       Serial.print(WiFi.SSID());
       Serial.println("]");
       delay(1000);
    }
  }
 
  if (WiFi.status() == WL_CONNECTED){
    this->blinkControlLed(0);
    digitalWrite(this->controlLed, HIGH);
    IPAddress ip = WiFi.localIP();
    sprintf(this->IP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    this->logger->setIP(this->IP);
    Serial.print("Succesfully connected to WIFI [");
    Serial.print(this->IP);
    Serial.println("]");
    ArduinoOTA.begin();
    this->initNTP();
  }
  else {
    Serial.println("Wifi didn't connect");
  }

  return WiFi.status() == WL_CONNECTED;
}

unsigned short App::addTicker(float secs, void (App::*callback)()) {
    unsigned short nTicker = this->num_tickers;

    this->tickers[nTicker] = new Ticker();
    this->tickers[nTicker]->attach(secs, std::bind(callback, this));
    this->num_tickers ++;
    return nTicker;
}

void App::addTimer(int millis, AppCallback function, char* name){

	Timer* newTimer = new Timer();

	newTimer->type = APP_TIMER;
	newTimer->millis = millis;
	newTimer->appFunction = function;
	newTimer->name = name;
	newTimer->lastRun = 0;
	newTimer->next = NULL;


	Timer* timer = this->timers;
	if(!timer){
	    this->timers = newTimer;
	    return;
	}

	while(timer->next){
            timer = timer->next;
	}

	timer->next = newTimer;
}

void App::addTimer(int millis, function_callback function, char*name){

	Timer* newTimer = new Timer();

	newTimer->type = USER_TIMER;
	newTimer->millis = millis;
	newTimer->function = function;
	newTimer->name = name;
	newTimer->lastRun = 0;
	newTimer->next = NULL;

	Timer* timer = this->timers;
	if(!timer){
	    this->timers = newTimer;
	    return;
	}

	while(timer->next){
            timer = timer->next;
	}

	timer->next = newTimer;
}

void App::attendTimers(){

    Timer* timer = this->timers;

    if (!timer){
	    return;
    }

    while (timer != NULL){

        if (millis() - timer->lastRun > timer->millis){
	    if (timer->type == USER_TIMER){
    	       timer->function();
            }

	    if (timer->type == APP_TIMER){
    	       (*this.*timer->appFunction)();
            }
	    timer->lastRun = millis();
        }

        timer = timer->next;
    }
}

void App::initNTP(){

  char buffer[50];
  // Initialize a NTPClient to get time
  //logger.log("[NTP_UPDATE] Updating NTP time");
  // Set offset time in seconds to adjust for your timezone, for example:
  timeClient.begin();
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(0);
  this->updateNTP();
    
}

void App::updateNTP(){
  char buffer[100];
  if (timeClient.update()){
      this->tEpoch = timeClient.getEpochTime();
      this->tEpochOffset = millis()/1000;
      sprintf(buffer, "NTP Updated [%d, %d]", this->tEpoch, this->tEpochOffset); 
      this->log(buffer);
  }
  else
  {
      this->log("Error updating NTP time");
  }
}

unsigned long App::getEpochSeconds(){
  if (this->tEpoch){
      return this->tEpoch + millis()/1000 - this->tEpochOffset;
  }
  else{
      return 0;
  }
}

void App::log(char* msg){
    this->logger->log(msg);
}

void App::handleOTA(){
  ArduinoOTA.handle();
}

bool App::send(String what){

  bool result;

  if (WiFi.status() != WL_CONNECTED){
    return false;
  }

  WiFiClient client;
  HTTPClient http;
  //Serial.print("sending ");
  //Serial.println(what.c_str());
  http.begin(client, what.c_str());
  http.addHeader("Content-Type", "application/json");
  http.addHeader("device_id", this->ID);

  int httpResponseCode = http.GET();

  if (httpResponseCode == 200){
        result = true;
      }
      else {
        Serial.print("[send] Error code: ");
        Serial.println(httpResponseCode);
        result = false;
      }

  // Free resources
  http.end();

  return result;
}

String App::get(String what){

  if (WiFi.status() != WL_CONNECTED){
    return "NOWIFI";
  }

  bool result;
  WiFiClient client;
  HTTPClient http;
  http.begin(client, what.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode == 200){
    return http.getString();
  }
  else
  {
    return "ERROR";
  }
}

void App::debug(char* level, unsigned short int channels, char* message){

  char new_buffer[100];
  sprintf(new_buffer, "[%s] %s", level, message);

  if (channels & BTLX_DEBUG_SERIAL || channels & BTLX_DEBUG_ALL){
    Serial.println(new_buffer);
  }

  if (channels & BTLX_DEBUG_WIFI || channels & BTLX_DEBUG_ALL){
    this->log(new_buffer);
  }
}

unsigned short App::readBoots(){
    unsigned short boots;
    EEPROM.get(BOOTS_ADDRESS, boots);
    return boots;
}

unsigned short App::incBoots(){
  unsigned short boots = this->readBoots();
  boots++;
  EEPROM.put(BOOTS_ADDRESS, boots);
  EEPROM.commit();
  return boots;
}


void App::resetBoots(){
    unsigned short boots = 0;
    EEPROM.put(BOOTS_ADDRESS, boots);
    EEPROM.commit();
}

char* App::millis_to_human(unsigned long millis)
{
    char* buffer = new char[100];

    int seconds = millis/1000;

    int days = int(seconds/86400);
    seconds = seconds % 86400;
    int hours = int(seconds/3600);
    seconds = seconds % 3600;
    int minutes = int(seconds/60);
    seconds = seconds % 60;

    sprintf(buffer, "%d days, %d hours, %d minutes, %d seconds", days, hours, minutes, seconds);
    return buffer;
}

void App::handleControlLed() {
    static unsigned long int lastBlinked = 0;

    if (this->controlLedFreq == 0) {
	return;
    }

    if (millis() - lastBlinked > this->controlLedFreq) {
        digitalWrite(this->controlLed, !digitalRead(this->controlLed));
	lastBlinked = millis();
    }
}

void App::checkConnection()  {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Reconnecting to WiFi...");
      WiFi.reconnect();

      int t = millis();

      while (millis() - t < 60000 && WiFi.status() != WL_CONNECTED){
        delay(2000);
        Serial.println("Reconnecting to WiFi...");
      }

      if (WiFi.status() == WL_CONNECTED){
        Serial.println("WiFi reconnected!");
      }
      else {
        Serial.println("Failed to reconnect to WiFi");
      }

    }
}

int App::resetEEPROM(int start, int size) {
  int t0 = millis();
  for (int i=start; i < size; i++){
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  int t1 = millis();
  return t1 - t0;
}

