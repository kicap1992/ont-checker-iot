#include <Arduino.h>

// for wifi connection
#include <ESP8266WiFi.h>
#include <ESP8266Ping.h>  // Library for ping functionality
#include <WiFiClient.h>
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
const char URL[] = "http://cekont.mywork-kkk.online/data";

// Replace these with your network credentials
// const char* ssid = "KARAN";
// const char* password = "12345679";
const char* ssid = "HOME";
const char* password = "88888888";

const char* server = "cekont.mywork-kkk.online"; // Change to your server's IP
const int port = 80;


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


// Function to test download speed
String testDownloadSpeed() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;

    Serial.println("Connecting to server for download test...");
    if (client.connect(server, port)) {
      Serial.println("Connected! Downloading data...");
      displayText2("Connecting to server for download test...", "Downloading data...");
      delay(1000);
      

      unsigned long startTime = millis();

      // Send HTTP GET request
      client.println("GET /download?id=1 HTTP/1.1");
      client.print("Host: ");
      client.println(server);
      client.println("Connection: close");
      client.println();

      // Wait for response
      while (!client.available()) {
        delay(10);
      }

      // Read and count bytes
      size_t bytesRead = 0;
      while (client.available()) {
        client.read();
        bytesRead++;
      }

      unsigned long endTime = millis();

      // Calculate download speed
      float timeTaken = (endTime - startTime) / 1000.0;  // Convert to seconds
      float speedKbps = (bytesRead * 8) / 1024.0 / timeTaken; // Convert to Kbps
      float speedMbps = speedKbps / 1024.0; // Convert to Mbps

      Serial.print("Downloaded ");
      Serial.print(bytesRead);
      displayText2("Downloaded ", String(bytesRead) + " bytes");
      delay(1000);
      Serial.print(" bytes in ");
      Serial.print(timeTaken, 2);
      Serial.println(" seconds.");
      displayText2(" bytes in ", String(timeTaken, 2) + " seconds.");
      delay(1000);
      Serial.print("Download Speed: ");
      Serial.print(speedMbps, 2);
      Serial.println(" Mbps");
      displayText2("Download Speed: ", String(speedMbps, 2) + " Mbps");
      delay(1000);

      client.stop();  // Close connection
      return String(speedMbps, 2);
    } else {
      Serial.println("Connection to server failed!");
      displayText2("Connection to server failed!", "Connection to server failed!");
      delay(1000);
      return "Connection to server failed!";
    }
  } else {
    Serial.println("WiFi disconnected!");
    displayText2("WiFi disconnected!", "WiFi disconnected!");
    delay(1000);
    return "WiFi disconnected!";
  }
}

// Function to test upload speed
String testUploadSpeed() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected!");
    displayText2("WiFi disconnected!", "WiFi disconnected!");
    delay(1000);
    return "WiFi disconnected!";
  }

  WiFiClient client;
  Serial.println("Connecting to server...");
  displayText("Connecting to server...");
  delay(1000);
  
  if (!client.connect(server, port)) {
    Serial.println("Connection failed!");
    displayText("Connection failed!");
    delay(1000);
    return "Connection failed!";
  }

  Serial.println("Connected! Uploading...");
  displayText("Connected! Uploading...");
  delay(1000);

  String boundary = "----ESP8266Upload";
  String payloadHeader = "--" + boundary + "\r\n" +
                         "Content-Disposition: form-data; name=\"file\"; filename=\"test.bin\"\r\n" +
                         "Content-Type: application/octet-stream\r\n\r\n";

  String payloadFooter = "\r\n--" + boundary + "--\r\n";

  const size_t dataSize = 100 * 1024; // 100 KB test file
  byte buffer[256];
  memset(buffer, 0xAA, sizeof(buffer));  // Fill buffer with dummy data

  size_t totalSize = payloadHeader.length() + dataSize + payloadFooter.length();

  // **Send HTTP Headers**
  client.println("POST /upload?id=1 HTTP/1.1");
  client.print("Host: ");
  client.println(server);
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.print("Content-Length: ");
  client.println(totalSize);
  client.println();  // End headers

  client.print(payloadHeader);
  client.flush();  // Ensure headers are fully sent

  // **Send File Data**
  unsigned long startTime = millis();
  for (size_t i = 0; i < dataSize; i += sizeof(buffer)) {
    client.write(buffer, sizeof(buffer));
    client.flush();  // Force send each chunk
    delay(1);  // Prevent buffer overflow
  }
  unsigned long endTime = millis();

  // **Send Footer (closing boundary)**
  client.print(payloadFooter);
  client.flush();

  Serial.println("Upload completed. Waiting for response...");
  displayText2("Upload completed", "Waiting for response...");
  delay(1000);

  // **Read server response**
  while (client.connected() && !client.available()) {
    delay(10);
  }

  while (client.available()) {
    Serial.write(client.read());
  }

  client.stop();  // Close connection

  float timeTaken = (endTime - startTime) / 1000.0;
  float speedMbps = ((dataSize * 8) / 1024.0 / timeTaken) / 1024.0;

  Serial.printf("\nUpload Speed: %.2f Mbps\n", speedMbps);
  return String(speedMbps, 2);
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
    delay(1000);
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
  delay(1000);
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");

    // Ping Google's public DNS server (8.8.8.8)
    if (Ping.ping("www.google.com")) {
      Serial.println("Internet connected");
      displayText2("WiFi connected", "Internet connected");
      delay(2000);

      String download = testDownloadSpeed();
      Serial.println(download);
      displayText(download.c_str());
      delay(2000);

      String upload = testUploadSpeed();
      Serial.println(upload);
      displayText(upload.c_str());
      delay(2000);
      


      getHttpData("?id=1&wifi=connected&internet=connected&download=" + download + "&upload=" + upload);
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
