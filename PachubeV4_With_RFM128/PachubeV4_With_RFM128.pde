/*
 * Arduino + Analog Sensors + RFM12 values Posted to Pachube 
 *      Created on: Aug 31, 2011
 *                  Jan 29, 2012
 *          Author: Victor Aprea
 *                  Igor Steiner
 *   Documentation: http://wickeddevice.com
 *
 *       Source Revision: 587
 *
 * Licensed under Creative Commons Attribution-Noncommercial-Share Alike 3.0
 *
 */

#include "EtherShield.h"
#include <Ports.h>
#include <RF12.h>

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * The following #defines govern the behavior of the sketch. You can console outputs using the Serial Monitor
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define MY_MAC_ADDRESS {0x54,0x55,0x58,0x10,0x00,0x25}                // must be uniquely defined for all Nanodes, e.g. just change the last number
#define USE_DHCP                                                      // comment out this line to use static network parameters
#define PACHUBE_API_KEY "vi2cY7xBsOtFX-5dswzekyaFcYzbLUaaHWxQRXwrgN0" // change this to your API key
#define HTTPFEEDPATH "/v2/feeds/38340"                                   // change this to th relative URL of your feed "/v2/feeds/38340" 

#define SENSOR1_ANALOG_PIN 0
#define SENSOR2_ANALOG_PIN 1
#define SENSOR3_ANALOG_PIN 2
#define SENSOR4_ANALOG_PIN 3
#define DELAY_BETWEEN_PACHUBE_POSTS_MS 15000L      
#define SERIAL_BAUD_RATE 57600

#ifndef USE_DHCP // then you need to supply static network parameters, only if you are not using DHCP
  #define MY_IP_ADDRESS {192,168,  1, 25}
  #define MY_NET_MASK   {255,255,255,  0}
  #define MY_GATEWAY    {192,168,  1,  1}
  #define MY_DNS_SERVER {192,168,  1,  1}
#endif

// change the template to be consistent with your datastreams: see http://api.pachube.com/v2/
#define FEED_POST_MAX_LENGTH 300

static char feedTemplate[] = "{\"version\":\"1.0.0\",\"datastreams\":[{\"id\":\"Temp\", \"current_value\":\"%s\"},{\"id\":\"Svetl\",\"current_value\":\"%s\"},{\"id\":\"Z.temp\",\"current_value\":\"%s\"},{\"id\":\"Z.vlaga\",\"current_value\":\"%d\"},{\"id\":\"Z.svetl\",\"current_value\":\"%d\"},{\"id\":\"Gib\",\"current_value\":\"%d\"},{\"id\":\"LoBat\",\"current_value\":\"%d\"}]}";
static char feedPost[FEED_POST_MAX_LENGTH] = {0}; // this will hold your filled out template

static char charBufSensorValue1[6] = {0};
static char charBufSensorValue2[6] = {0};  
static char charBufSensorValue3[6] = {0};  


//structure of payloads (outside and external units)
struct {
    byte light;     // light sensor: 0..255
    byte moved :1;  // motion detector: 0..1
    byte humi  :7;  // humidity: 0..100
    int temp   :10; // temperature: -500..+500 (tenths)
    byte lobat :1;  // supply voltage dropped under 3.1V: 0..1
} payload;

//structure of payloads (thermocouple - inside)
struct {
    int temp;    // temperature: (tenths)
    byte count;  // counter
} ext_payload;

uint8_t fillOutTemplateWithSensorValues(uint16_t node_id, float sensorValue1, float sensorValue2, int sensorValue3, byte sensorValue4, byte sensorValue5, byte sensorValue6, byte sensorValue7){
  // change this function to be consistent with your feed template, it will be passed the node id and four sensor values by the sketch
  // if you return (1) this the sketch will post the contents of feedPost to Pachube, if you return (0) it will not post to Pachube
  // you may use as much of the passed information as you need to fill out the template

  dtostrf(sensorValue1, 5, 1, charBufSensorValue1);
  dtostrf(sensorValue2, 5, 1, charBufSensorValue2);
  dtostrf((float)sensorValue3/10, 5, 1, charBufSensorValue3);
  
  snprintf(feedPost, FEED_POST_MAX_LENGTH, feedTemplate, charBufSensorValue1, charBufSensorValue2, charBufSensorValue3, sensorValue4, sensorValue5, sensorValue6, sensorValue7); 

  return (1);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * You shouldn't need to make changes below here for configuring the sketch
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

// mac and ip (if not using DHCP) have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
static uint8_t mymac[6] = MY_MAC_ADDRESS;

// IP address of the host being queried to contact (IP of the first portion of the URL):
static uint8_t websrvip[4] = { 0, 0, 0, 0 }; // resolved through DNS

#ifndef USE_DHCP
// use the provided static parameters
static uint8_t myip[4]      = MY_IP_ADDRESS;
static uint8_t mynetmask[4] = MY_NET_MASK;
static uint8_t gwip[4]      = MY_GATEWAY;
static uint8_t dnsip[4]     = MY_DNS_SERVER;
#else
// these will all be resolved through DHCP
static uint8_t dhcpsvrip[4] = { 0,0,0,0 };    
static uint8_t myip[4]      = { 0,0,0,0 };
static uint8_t mynetmask[4] = { 0,0,0,0 };
static uint8_t gwip[4]      = { 0,0,0,0 };
static uint8_t dnsip[4]     = { 0,0,0,0 };
#endif

long lastPostTimestamp;
boolean firstTimeFlag = true;
// global string buffer for hostname message:
#define FEEDHOSTNAME "api.pachube.com\r\nX-PachubeApiKey: " PACHUBE_API_KEY
#define FEEDWEBSERVER_VHOST "api.pachube.com"

static char hoststr[150] = FEEDWEBSERVER_VHOST;

#define BUFFER_SIZE 550
static uint8_t buf[BUFFER_SIZE+1];

EtherShield es=EtherShield();

void setup(){
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("Start");

  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize ENC28J60
  es.ES_enc28j60Init(mymac, 8);

#ifdef USE_DHCP
  acquireIPAddress();
#endif

  printNetworkParameters();

  //init the ethernet/ip layer:
  es.ES_init_ip_arp_udp_tcp(mymac,myip, 80);

  // init the web client:
  es.ES_client_set_gwip(gwip);  // e.g internal IP of dsl router
  es.ES_dnslkup_set_dnsip(dnsip); // generally same IP as router
  
  Serial.println("Awaiting Client Gateway");
  while(es.ES_client_waiting_gw()){
    int plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);
    es.ES_packetloop_icmp_tcp(buf,plen);    
  }
  Serial.println("Client Gateway Complete, Resolving Host");

  resolveHost(hoststr, websrvip);
  Serial.print("Resolved host: ");
  Serial.print(hoststr);
  Serial.print(" to IP: ");
  printIP(websrvip);
  Serial.println();
  
  es.ES_client_set_wwwip(websrvip);
  
  lastPostTimestamp = millis();
  
  // LED on Pin Digital 6:
  pinMode(6, OUTPUT);

  // Initialize RFM12B as an 868Mhz module and Node 100 + Group 212:
  rf12_initialize(100, RF12_868MHZ, 212); 
}

void loop(){
  
   // If a packet is received by RFM12, if it's valid and if it's of the same size as our payload:
  if (rf12_recvDone() && rf12_crc == 0)
  {
    //Serial.println(rf12_hdr, DEC);
    if (rf12_len == sizeof payload && (rf12_hdr == 1 || rf12_hdr == 33))  // unit i=1 or i=33
    {
      //Serial.println(payload.temp, DEC);
      // Copy the received data into payload:
      memcpy(&payload, (byte*) rf12_data, sizeof(payload));
    }
    
    if (rf12_len == sizeof ext_payload && rf12_hdr == 22)  // thermocouple unit i=22
    {
      //Serial.println(ext_payload.temp, DEC);
      // Copy the received data into payload:
      memcpy(&ext_payload, (byte*) rf12_data, sizeof(ext_payload));
    }    
  }
  
  long currentTime = millis();
  
  int plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);
  es.ES_packetloop_icmp_tcp(buf,plen);
  
  if(currentTime - lastPostTimestamp > DELAY_BETWEEN_PACHUBE_POSTS_MS || firstTimeFlag){   
    firstTimeFlag = false;

 //   float sensorValue1 = WhatIsTemperature(0);    // Read temperature on Analog_0
    float sensorValue1 = (float)ext_payload.temp/10;      // Thermocouple - internal
    float sensorValue2 = WhatIsBrightness(1);     // Read brightness on Analog_1 

//  if(fillOutTemplateWithSensorValues(0, sensorValue1, sensorValue2, payload.temp, payload.humi, payload.light, payload.moved, payload.lobat)){       
    if(fillOutTemplateWithSensorValues(0, sensorValue1, sensorValue2, payload.temp, payload.humi, payload.light, ext_payload.temp/10, payload.lobat)){   
      es.ES_client_http_post(PSTR(HTTPFEEDPATH),PSTR(FEEDWEBSERVER_VHOST),PSTR(FEEDHOSTNAME), PSTR("PUT "), feedPost, &sensor_feed_post_callback);    
    }
    lastPostTimestamp = currentTime;
  }
}

#ifdef USE_DHCP
void acquireIPAddress(){
  uint16_t dat_p;
  long lastDhcpRequest = millis();
  uint8_t dhcpState = 0;
  Serial.println("Sending initial DHCP Discover");
  es.ES_dhcp_start( buf, mymac, myip, mynetmask,gwip, dnsip, dhcpsvrip );

  while(1) {
    // handle ping and wait for a tcp packet
    int plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);

    dat_p=es.ES_packetloop_icmp_tcp(buf,plen);
    //    dat_p=es.ES_packetloop_icmp_tcp(buf,es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf));
    if(dat_p==0) {
      int retstat = es.ES_check_for_dhcp_answer( buf, plen);
      dhcpState = es.ES_dhcp_state();
      // we are idle here
      if( dhcpState != DHCP_STATE_OK ) {
        if (millis() > (lastDhcpRequest + 10000L) ){
          lastDhcpRequest = millis();
          // send dhcp
          Serial.println("Sending DHCP Discover");
          es.ES_dhcp_start( buf, mymac, myip, mynetmask,gwip, dnsip, dhcpsvrip );
        }
      } 
      else {
        return;        
      }
    }
  }   
}
#endif

// hostName is an input parameter, ipAddress is an outputParame
void resolveHost(char *hostName, uint8_t *ipAddress){
  es.ES_dnslkup_request(buf, (uint8_t*)hostName );
  while(1){
    int plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);
    es.ES_packetloop_icmp_tcp(buf,plen);   
    if(es.ES_udp_client_check_for_dns_answer(buf, plen)) {
      uint8_t *websrvipptr = es.ES_dnslkup_getip();
      for(int on=0; on <4; on++ ) {
        ipAddress[on] = *websrvipptr++;
      }     
      return;
    }    
  }
}  

void sensor_feed_post_callback(uint8_t statuscode,uint16_t datapos){
/*  Serial.println();
  Serial.print("Status Code: ");
  Serial.println(statuscode, HEX);
  Serial.print("Datapos: ");
  Serial.println(datapos, DEC);
  Serial.println("PAYLOAD");
  for(int i = 0; i < 100; i++){
     Serial.print(byte(buf[i]));
  }
  
  Serial.println();
  Serial.println(); 
*/ 
}

// Output a ip address from buffer from startByte
void printIP( uint8_t *buf ) {
  for( int i = 0; i < 4; i++ ) {
    Serial.print( buf[i], DEC );
    if( i<3 )
      Serial.print( "." );
  }
}

void printNetworkParameters(){
  Serial.print( "My IP: " );
  printIP( myip );
  Serial.println();

  Serial.print( "Netmask: " );
  printIP( mynetmask );
  Serial.println();

  Serial.print( "DNS IP: " );
  printIP( dnsip );
  Serial.println();

  Serial.print( "GW IP: " );
  printIP( gwip );
  Serial.println();  
}

float WhatIsTemperature(int potPin) { 
  int span = 500; 
  float temperature = 0; 
  long aRead = 0L; 
  for (int i = 0; i < span; i++) { 
    aRead = aRead+analogRead(potPin); 
  } 
  aRead = aRead / span; 
  aRead = aRead - 10L;                         // korekcija Analog0 vhoda pri 2,93 V (20 st.C)
  temperature = map(aRead, 0, 1023, 0, 5000);  // 0..5V = 0..1023 = 0..500.0 Kelvin
  temperature = (temperature - 2731)/10;      // convert to Celsius = Kelvin - 273.1
  temperature = temperature - 1.5;            // korekcija za 1.5 st.C (napaka LM335)

  return(temperature); 
}

float WhatIsBrightness(int potPin) { 
  int span = 500; 
  float brightness = 0; 
  long aRead = 0L; 
  for (int i = 0; i < span; i++) { 
    aRead = aRead+analogRead(potPin); 
  } 
  aRead = aRead / span; 
  brightness = map(aRead, 80, 880, 100, 0);  // convert to brightness: AI=880 .. 80 = 0 .. 100% 

  return(brightness); 
}
