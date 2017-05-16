#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <FS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WebSocketsServer.h>   // library: https://github.com/Links2004/arduinoWebSockets

#include <SPI.h>
#include "Adafruit_MAX31855.h"

// Example creating a thermocouple instance with software SPI on any three
// digital IO pins.
#define MAXDO   14
#define MAXCS   12
#define MAXCLK  13

// initialize the Thermocouple
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

int sample_time_1 = 2000;
int sample_time_2 = 60000;
unsigned int sampleTimestamp1;
unsigned int sampleTimestamp2;
int sample_idx_1 = 0;
int num_samples_1 = 0;
int sample_idx_2 = 0;
int num_samples_2 = 0;
#define MAX_SAMPLES 2048
uint16_t samples_1[MAX_SAMPLES];
uint16_t samples_2[MAX_SAMPLES];


/*
 * This example serves a "hello world" on a WLAN and a SoftAP at the same time.
 * The SoftAP allow you to configure WLAN parameters at run time. They are not setup in the sketch but saved on EEPROM.
 * 
 * Connect your computer or cell phone to wifi network ESP_ap with password 12345678. A popup may appear and it allow you to go to WLAN config. If it does not then navigate to http://192.168.4.1/wifi and config it there.
 * Then wait for the module to connect to your wifi and take note of the WLAN IP it got. Then you can disconnect from ESP_ap and return to your regular WLAN.
 * 
 * Now the ESP8266 is in your network. You can reach it through http://192.168.x.x/ (the IP you took note of) or maybe at http://esp8266.local too.
 * 
 * This is a captive portal because through the softAP it will redirect any http request to http://192.168.4.1/
 */

/* Set these to your desired softAP credentials. They are not configurable at runtime */
const char *softAP_ssid = "RakuTemp";
//const char *softAP_password = "12345678";

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "raku";

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
WebSocketsServer webSocket = WebSocketsServer(81);

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);


/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
long lastConnectTry = 0;

/** Current WLAN status */
int status = WL_IDLE_STATUS;

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {
  switch (type) {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED: {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    webSocket.sendTXT(num, String("D") + " " + sample_idx_1 + " " + num_samples_1 + " " + sample_idx_2 + " " + num_samples_2 + " " + MAX_SAMPLES);
    /*int idx = sample_idx_1 - num_samples_1;
    if (idx < 0) idx = MAX_SAMPLES + idx;
    for (int n=0; n<num_samples_1; n++) {
       webSocket.sendTXT(num, String("") + samples_1[idx]);
       idx++;
       if (idx >= MAX_SAMPLES) {
        idx = 0;
       }
    }*/
    Serial.println("Sending binary");
    webSocket.sendBIN(num, (uint8_t *) &samples_1[0], MAX_SAMPLES*2/2);
    webSocket.sendBIN(num, (uint8_t *) &samples_1[MAX_SAMPLES/2], MAX_SAMPLES*2/2);
    webSocket.sendBIN(num, (uint8_t *) &samples_2[0], MAX_SAMPLES*2/2);
    webSocket.sendBIN(num, (uint8_t *) &samples_2[MAX_SAMPLES/2], MAX_SAMPLES*2/2);
    Serial.println("Binary sent");
    }
    break;
  case WStype_TEXT:
    Serial.printf("[%u] get Text: %s\n", num, payload);
    /*if (payload[0] == '#') {
      // decode dimmer data
      uint32_t ab = (uint32_t)strtol((const char *)&payload[1], NULL, 16);
      setBar(barD1, ((ab >> 8) & 0xFF));
      setBar(barD2, ((ab >> 0) & 0xFF));
    }*/
    break;
  }
}

void setup() {
  delay(1000);
  Serial.begin(9600);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  //WiFi.softAP(softAP_ssid, softAP_password);
  WiFi.softAP(softAP_ssid);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS mount failed");
  }
  else {
    Serial.println("SPIFFS mount succesfull");
  }

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", handleRoot);
  server.on("/wifi", handleWifi);
  server.on("/wifisave", handleWifiSave);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  httpUpdater.setup(&server);
  server.onNotFound ( handleNotFound );
  server.serveStatic("/", SPIFFS, "/", "max-age=86400");
  server.begin(); // Web server start
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.print("WEBSOCKETS_MAX_DATA_SIZE: ");
  Serial.println(WEBSOCKETS_MAX_DATA_SIZE);
  Serial.println("HTTP server started");
  loadCredentials(); // Load WLAN credentials from network
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
  sampleTimestamp1 = millis() + sample_time_1;
  sampleTimestamp2 = millis() + sample_time_2;

  pinMode(LED_BUILTIN, OUTPUT);
  for (int n=0; n<5; n++) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100);
  }
}

void connectWifi() {
  Serial.println("Connecting as wifi client...");
  WiFi.disconnect();
  WiFi.begin ( ssid, password );
  int connRes = WiFi.waitForConnectResult();
  Serial.print ( "connRes: " );
  Serial.println ( connRes );
}

void loop() {
  if (connect) {
    Serial.println ( "Connect requested" );
    connect = false;
    connectWifi();
    lastConnectTry = millis();
  }
  {
    int s = WiFi.status();
    if (s == 0 && millis() > (lastConnectTry + 10000) ) {
      /* If WLAN disconnected and idle try to connect */
      /* Don't set retry time too low as retry interfere the softAP operation */
      connect = true;
    }
    if (status != s) { // WLAN status change
      Serial.print ( "Status: " );
      Serial.println ( s );
      status = s;
      if (s == WL_CONNECTED) {
        /* Just connected to WLAN */
        Serial.println ( "" );
        Serial.print ( "Connected to " );
        Serial.println ( ssid );
        Serial.print ( "IP address: " );
        Serial.println ( WiFi.localIP() );

        // Setup MDNS responder
        if (!MDNS.begin(myHostname)) {
          Serial.println("Error setting up MDNS responder!");
        } else {
          Serial.println("mDNS responder started");
          // Add service to MDNS-SD
          MDNS.addService("http", "tcp", 80);
        }
      } else if (s == WL_NO_SSID_AVAIL) {
        WiFi.disconnect();
      }
    }
  }


  // Do work:

  if (millis() > sampleTimestamp1) {
    // basic readout test, just print the current temp
    Serial.print("Internal Temp = ");
    Serial.println(thermocouple.readInternal());
    
    double c = thermocouple.readCelsius();
    unsigned int c_millis = millis();
    
    if (isnan(c)) {
      Serial.println("Something wrong with thermocouple!");
    } else {
      samples_1[sample_idx_1] = (uint16_t)c;
      if (++num_samples_1 > MAX_SAMPLES) num_samples_1 = MAX_SAMPLES;
      if (++sample_idx_1 >= MAX_SAMPLES) sample_idx_1 = 0;
  
      bool sendTemp2 = false;
      if (millis() > sampleTimestamp2) {
        samples_2[sample_idx_2] = (uint16_t)c;
        if (++num_samples_2 > MAX_SAMPLES) num_samples_2 = MAX_SAMPLES;
        if (++sample_idx_2 >= MAX_SAMPLES) sample_idx_2 = 0;
        sampleTimestamp2 = millis() + sample_time_2;
        sendTemp2 = true;
      }

      Serial.print("C = "); 
      Serial.println(c);
      if (sendTemp2) {
        webSocket.broadcastTXT(String("") + c + " " + c_millis + " 2");
      }
      else {
        webSocket.broadcastTXT(String("") + c + " " + c_millis);
      }
    }
    sampleTimestamp1 = millis() + sample_time_1;
  } 
  
  //DNS
  dnsServer.processNextRequest();
  //HTTP
  server.handleClient();
  //Websockets
  webSocket.loop();
}

