#include <Arduino.h>

// for wifi connection
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>  // Library for ping functionality
// end of wifi

// this is for sim800l
#include <SoftwareSerial.h>
#include "SIM800L.h"

#define SIM800_RX_PIN 12
#define SIM800_TX_PIN 13
#define SIM800_RST_PIN 14
// end of sim800l

// this is for oled
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C 
// Declaration for an I2C OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
// end of oled

const char APN[] = "internet";
const char URL[] = "http://cekont.mywork-kk.online/data";

// Replace these with your network credentials
// const char* ssid = "hu";
// const char* password = "12345679";
const char* ssid = "HOME";
const char* password = "88888888";

SIM800L* sim800l;

void displayText(const char* text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 30);
  display.println(text);
  display.display();
}
void displayText2(const String& text, const String& text2) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.println(text);
  display.setCursor(0, 40);
  display.println(text2);
  display.display();
  
}

void setupModule() {
 // Wait until the module is ready to accept AT commands
  while(!sim800l->isReady()) {
    Serial.println(F("Problem to initialize AT command, retry in 1 sec"));
    displayText2("Problem to initialize AT command,", " retry in 1 sec");
    delay(1000);
  }

  // Active echo mode (for some module, it is required)
  sim800l->enableEchoMode();

  Serial.println("Module ready");
  displayText("Module ready");
  delay(1000);

  // Print version
  Serial.print("Module ");
  Serial.println(sim800l->getVersion());
  Serial.print("Firmware ");
  Serial.println(sim800l->getFirmware());
  displayText2("Module : " + String(sim800l->getVersion()), "Firmware : " + String(sim800l->getFirmware()));
  delay(1000);

  // Print SIM card status
  Serial.print(F("SIM status "));
  Serial.println(sim800l->getSimStatus());
  displayText2("SIM status : ", String(sim800l->getSimStatus()));
  delay(1000);

  // Print SIM card number
  Serial.print("SIM card number ");
  Serial.println(sim800l->getSimCardNumber());
  displayText2("SIM card number : ", String(sim800l->getSimCardNumber()));
  delay(1000);

  // Wait for the GSM signal
  uint8_t signal = sim800l->getSignal();
  while(signal <= 0) {
    delay(1000);
    signal = sim800l->getSignal();
  }

  if(signal > 5) {
    Serial.print(F("Signal OK (strenght: "));
    displayText2(("Signal OK "), ("(strenght: " + String(signal) + ")").c_str());
    delay(1000);
  } else {
    Serial.print(F("Signal low (strenght: "));
    displayText2(("Signal low "), (" (strenght: " + String(signal) + ")").c_str());
    delay(1000);
  }
  Serial.print(signal);
  Serial.println(F(")"));
  delay(1000);

  // Wait for operator network registration (national or roaming network)
  NetworkRegistration network = sim800l->getRegistrationStatus();
  while(network != REGISTERED_HOME && network != REGISTERED_ROAMING) {
    delay(1000);
    network = sim800l->getRegistrationStatus();
  }
  Serial.println(F("Network registration OK"));
  displayText("Network registration OK");
  delay(1000);

  Serial.println("End of test protocol");
  displayText("End of test protocol");
  delay(1000);

  // Setup APN for GPRS configuration
  bool success = sim800l->setupGPRS(APN);
  while(!success) {
    success = sim800l->setupGPRS(APN);
    delay(5000);
  }
  Serial.println(F("GPRS config OK"));
  displayText("GPRS config OK");
  delay(1000);
}

void getHttpData(const String& params) {
  Serial.println(F("Start HTTP GET..."));
  displayText("Start HTTP GET...");
  delay(1000);

  String url = URL + params;

  // Do HTTP GET communication with 10s for the timeout (read)
  uint16_t rc = sim800l->doGet(url.c_str(), 20000);
   if(rc == 200) {
    // Success, output the data received on the serial
    Serial.print(F("HTTP GET successful ("));
    Serial.print(sim800l->getDataSizeReceived());
    Serial.println(F(" bytes)"));
    Serial.print(F("Received : "));
    Serial.println(sim800l->getDataReceived());
    displayText2("HTTP GET successful ", String(sim800l->getDataSizeReceived()) + " bytes)");
    delay(1000);
    displayText2("Received : ", String(sim800l->getDataReceived()));
    delay(3000);
  } else {
    // Failed...
    Serial.print(F("HTTP GET error "));
    Serial.println(rc);
    displayText2("HTTP GET error ", String(rc));
    delay(1000);
  }

  delay(1000);
}
void setup() {
  // Initialize Serial Monitor for debugging
  Serial.begin(115200);
  while(!Serial);

  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  displayText("Starting...");
  delay(1000);

  // Initialize a SoftwareSerial
  SoftwareSerial* serial = new SoftwareSerial(SIM800_RX_PIN, SIM800_TX_PIN);
  serial->begin(9600);
  delay(1000);

  // Initialize SIM800L driver with an internal buffer of 200 bytes and a reception buffer of 512 bytes, debug disabled
  sim800l = new SIM800L((Stream *)serial, SIM800_RST_PIN, 200, 512);

  // Equivalent line with the debug enabled on the Serial
  //sim800l = new SIM800L((Stream *)serial, SIM800_RST_PIN, 200, 512, (Stream *)&Serial);

  Serial.println("Start of test protocol");
  displayText2("Start of ", "test protocol");
  delay(1000);

  setupModule();
  WiFi.begin(ssid, password);

  
}
void loop() {
  // Establish GPRS connectivity (5 trials)
  bool connected = false;
  for(uint8_t i = 0; i < 5 && !connected; i++) {
    delay(1000);
    connected = sim800l->connectGPRS();
  }

  // Check if connected, if not reset the module and setup the config again
  if(connected) {
    Serial.print(F("GPRS connected with IP "));
    Serial.println(sim800l->getIP());
    displayText2("GPRS connected with IP ", String(sim800l->getIP()));
    delay(2000);
  } else {
    Serial.println(F("GPRS not connected !"));
    Serial.println(F("Reset the module."));
    displayText2("GPRS not connected !", "Reset the module.");
    delay(1000);
    sim800l->reset();
    setupModule();
    return;
  }

  // connect to wifi in 5 seconds
  delay(3000);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");

    // Ping Google's public DNS server (8.8.8.8)
    if (Ping.ping("www.google.com")) {
      Serial.println("Internet connected");
      displayText2("WiFi connected", "Internet connected");
      delay(2000);
      getHttpData("?id=1&wifi=connected&internet=connected");
    } else {
      Serial.println("Internet disconnected");
      displayText2("WiFi connected", "Internet disconnected");
      delay(2000);
      getHttpData("?id=1&wifi=connected&internet=disconnected");
    }
  } else {
    Serial.println("WiFi disconnected");
    displayText2("WiFi disconnected", "Internet disconnected");
    delay(2000);
    getHttpData("?id=1&wifi=disconnected&internet=disconnected");
  }


  // Close GPRS connectivity (5 trials)
  bool disconnected = sim800l->disconnectGPRS();
  for(uint8_t i = 0; i < 5 && !connected; i++) {
    delay(1000);
    disconnected = sim800l->disconnectGPRS();
  }
  
  if(disconnected) {
    Serial.println(F("GPRS disconnected !"));
    displayText("GPRS disconnected !");
    delay(1000);
  } else {
    Serial.println(F("GPRS still connected !"));
    displayText2("GPRS disconnected !", "GPRS still connected !");
    delay(1000);
  }

  // // Go into low power mode
  // bool lowPowerMode = sim800l->setPowerMode(MINIMUM);
  // if(lowPowerMode) {
  //   Serial.println(F("Module in low power mode"));
  //   displayText("Module in low power mode");
  //   delay(1000);
  // } else {
  //   Serial.println(F("Failed to switch module to low power mode"));
  //   displayText2("Failed to switch module to low power mode", "");
  //   delay(1000);
  // }

  // End of program... wait...
  // while(1);
  delay(500);
}
