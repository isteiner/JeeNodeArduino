/*
2020-11-21  I.Steiner derivate from wemos-simple_co2meter & mqtt_esp_OLED_OTA_STI20201106
2020-11-24  I.Steiner add temperature sensor DS18B20

*/

#include <WiFi.h>
#include <PubSubClient.h>
#include "SSD1306.h"
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
// Initialize the OLED display using Wire library
SSD1306Wire  display(0x3c, 5, 4); //5,4  21,22

// built-in LED to GPIO16
#define LED 16

//Senseair Sensor UART pins
#define TXD2 2
#define RXD2 14

#define N  3   // set number of measurements to avearage value

#include <OneWire.h>
#include <DallasTemperature.h>

// GPIO where the DS18B20 is connected to
const int oneWireBus = 15;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

byte CO2req[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};
byte CO2out[] = {0, 0, 0, 0, 0, 0, 0};

// Update these with values suitable for your network.
// Beni
//const char* ssid = "Beni";
//const char* password = "Alfa123456";
// LinksysExt2
const char* ssid = "LinksysExt2";
const char* password = "dvq7vjxucz";
// Linksys02327
//const char* ssid = "Linksys02327";
//const char* password = "dvq7vjxucz";
// Connect to Emon mqtt on local LAN
const char* mqtt_server = "10.146.223.20";
// Connect to external eclipse mqtt server
//const char* mqtt_server = "mqtt.eclipse.org";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
unsigned long counter = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  char newpayload[]="     ";
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    newpayload[i] = payload[i];
  }
  Serial.println();
 /*
  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
  // clear the display
 
  display.clear();
  display.drawString(0, 0, topic);
  sprintf(msg, "%s", payload);
  display.drawString(40, 40, (char*) newpayload);
  display.display(); */
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if (client.connect(clientId.c_str())) {
    if (client.connect("ESP32client", "emonpi", "emonpimqtt2016")) {
    //if (client.connect("STI")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("emon/ESP32/outTopic", "");
      // ... and resubscribe
      //client.subscribe("inTopic");
      client.subscribe("emon/ESP32/inTopic");      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void RequestCO2 ()
{
  while (!Serial1.available())
  {
    Serial1.write(CO2req, 7);
    delay(50);
  }

  int timeout = 0;
  while (Serial1.available() < 7 )
  {
    timeout++;
    if (timeout > 10)
    {
      while (Serial1.available())
        Serial1.read();
      break;
    }
    delay(50);
  }

  for (int i = 0; i < 7; i++)
  {
    CO2out[i] = Serial1.read();
  }
}

unsigned long CO2count()
{
  int high = CO2out[3];
  int low = CO2out[4];
  unsigned long val = high * 256 + low;
  return val * 1; // S8 = 1. K-30 3% = 3, K-33 ICB = 10
}

void setup() {
 // PC COM Port
  Serial.begin(57600);
  Serial.println("\n[wemos_co2_mqtt_esp_OLED_OTA_STI20201121]");
  pinMode(LED, OUTPUT);
  // Start the DS18B20 sensor
  sensors.begin();
  // UART to Sensair CO2 Sensor
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);
  // Setup display
  Serial.println("Booting");
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_16);   
  display.clear();
  display.drawString(0, 0, "Booting");
  display.display();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    display.clear();
    display.drawString(0, 0, "ConnFailed");
    display.display();
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  // ArduinoOTA.setHostname("myesp32");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //Serial.println(WiFi.localIP());
  display.clear();
  display.drawString(0, 0, "IP address:");
  //display.drawString(90, 0, String(counter));
  display.drawString(0, 20, WiFi.localIP().toString());
  display.display();
  delay(2000);
     
  ArduinoOTA.onStart([]() {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(0, 0, "OTA Update");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.drawProgressBar(4, 32, 120, 8, progress / (total / 100) );
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(0, 0, "Restart");
    display.display();
  });

  // Align text vertical/horizontal center
  //display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  //display.drawString(0, 0, "Ready for OTA:\n" + WiFi.localIP().toString());
  //display.display();

  //pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  // Initialising the UI will init the display too. 
  display.setFont(ArialMT_Plain_24);
}

void loop() { 
  //ArduinoOTA.handle();
  
  //measure CO2 and temperature and calculate average values in N steps
  unsigned long CO2 = 0;
  float temperatureC = 0;
  
  for (byte i=0; i <N; ++i) {
    ArduinoOTA.handle();
    // Get Data from the CO2 Sensor
    RequestCO2();
    CO2 = CO2 + CO2count();

    // Get Data from temperature DS18B20 Sensor
    sensors.requestTemperatures(); 
    temperatureC = temperatureC + sensors.getTempCByIndex(0); 
       
    delay(4000);
  }

  temperatureC = temperatureC/N;
  CO2 = CO2/N;
  String CO2s = "CO2: " + String(CO2);

  // Display Data on OLED
  display.clear();
  display.drawString(10, 20, CO2s);
  display.display();

  // switch alarm LED when CO2 is high (pin LOW = led ON)
  if (CO2 > 1500) digitalWrite(LED, LOW);
  else digitalWrite(LED, HIGH);
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  ++counter;
  sprintf(msg, "%d", counter);
  client.publish("emon/CO2/counter_2", msg);    //publish to mqtt [counter] value - second sensor
  //client.publish("STI/CO2/counter", msg);    //publish to mqtt [counter] value
  Serial.print("Publish message [counter]: ");
  Serial.println(counter);    
  sprintf(msg, "%d", CO2);
  client.publish("emon/CO2/CO2_2", msg);        //publish to mqtt [CO2] value - second sensor
  //client.publish("STI/CO2/CO2", msg);        //publish to mqtt [CO2] value
  Serial.print("Publish message [CO2]: ");
  Serial.println(CO2);  
  sprintf(msg, "%.2f", temperatureC);
  client.publish("emon/CO2/temperatureC_2", msg);  //publish to mqtt [temperatureC] value - second sensor
  //client.publish("STI/CO2/temperatureC", msg);  //publish to mqtt [temperatureC] value
  Serial.print("Publish message [temperatureC]: ");
  Serial.println(temperatureC);    
}
