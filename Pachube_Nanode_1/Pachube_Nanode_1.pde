////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////Networking variables and constants//////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
/*
 * Arduino ENC28J60 Ethernet shield/Nanode DHCP client test
*/

// If using a Nanode (www.nanode.eu) instead of Arduino and ENC28J60 EtherShield then
// use this define:
#define NANODE
#include <LiquidCrystal.h>
#include <EtherShield.h>
#ifdef NANODE
#include <NanodeMAC.h>
#endif

// Please modify the following lines. mac and ip have to be unique
// in your local area network. You can not have the same numbers in
// two devices:
// how did I get the mac addr? Translate the first 3 numbers into ascii is: TUX
#ifdef NANODE
static uint8_t mymac[6] = { 0,0,0,0,0,0 };
#else
static uint8_t mymac[6] = { 0x54,0x55,0x58,0x12,0x34,0x56 };
#endif

static uint8_t myip[4] = { 0,0,0,0 };
static uint8_t mynetmask[4] = { 0,0,0,0 };


// Default gateway. The ip address of your DSL router. 
static uint8_t gwip[4] = { 0,0,0,0};

static uint8_t dnsip[4] = { 0,0,0,0 };
static uint8_t dhcpsvrip[4] = { 0,0,0,0 };

#define DHCPLED 6

// listen port for tcp/www:
#define MYWWWPORT 80

#define BUFFER_SIZE 750
static uint8_t buf[BUFFER_SIZE+1];

EtherShield es=EtherShield();
#ifdef NANODE
NanodeMAC mac( mymac );
#endif

///WEB SERVER/////////
uint16_t http200ok(void)
{
  return(es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n")));
}

char tempBuf[32] = {0}; // <--- for floats to chars

// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf)
{
 
  uint16_t plen;
  plen=http200ok();
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<html><head><title>Arduino ENC28J60 Ethernet Shield V1.0</title></head><body>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<center><h1>Welcome to Arduino ENC28J60 Ethernet Shield V1.0</h1>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<center><h1>"));
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("@ HSBXL the temperature is:</h1>"));
  plen=es.ES_fill_tcp_data(buf, plen, tempBuf);
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("<center><h1>"));
   plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("Pachube IP is:</h1>"));
   
   
  plen=es.ES_fill_tcp_data(buf, plen, tempBuf); 
  plen=es.ES_fill_tcp_data_p(buf,plen,PSTR("</body></html>"));
  return(plen);
}




////////////////DNS and define Pachube host/////////////////////////
#define HOSTNAME "www.pachube.com\r\nX-PachubeApiKey: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"      // API key
static uint8_t websrvip[4] = { 0,0,0,0 };	                                                 // Get pachube ip by DNS call
#define WEBSERVER_VHOST "www.pachube.com"
//#define HTTPPATH "/api/0000.csv"                                                                  // Set your own feed ID here
static uint8_t resend=0;
static int8_t dns_state=0;

////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////Thermistor variables  and constants//////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
// Thanks to Pieter Heremans and George Laskowsky for their advises and lessons.
// read voltage passing trough a voltage divider and geting temperature
const float rinfinit = 93.79645;   // This you should calculate yours
const float voltage = 5.04;        // should change it to 5040 for 5.04V
const float B = 4100;
const float unodivr = 93.79645;
const float resistor = 9920;       //9920 actually, but put 10000
int sensorPin = A0;                // select the input pin for the potentiometer
int sensorValue = 0;               // variable to store the value coming from the sensor
/////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void setup() {
Serial.begin(19200);                                    //  setup serial

//////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////Networking //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
Serial.println("DHCP Client test");
for( int i=0; i<6; i++ ) {
    Serial.print( mymac[i], HEX );
    Serial.print( i < 5 ? ":" : "" );
  }
  Serial.println();
  
  // Initialise SPI interface
  es.ES_enc28j60SpiInit();

  // initialize enc28j60
  Serial.println("Init ENC28J60");
#ifdef NANODE
  es.ES_enc28j60Init(mymac,8);
#else
  es.ES_enc28j60Init(mymac);
#endif


  // init the ethernet/ip layer:
  Serial.println("Init done");
  Serial.print( "ENC28J60 version " );
  Serial.println( es.ES_enc28j60Revision(), HEX);
  if( es.ES_enc28j60Revision() <= 0 ) {
    Serial.println( "Failed to access ENC28J60");
    while(1);    // Just loop here
  }
  
  Serial.println("Requesting IP Addresse");
  // Get IP Address details
  if( es.allocateIPAddress(buf, BUFFER_SIZE, mymac, 80, myip, mynetmask, gwip, dhcpsvrip, dnsip ) > 0 ) {
    // Display the results:
    Serial.print( "My IP: " );
    printIP( myip );
    Serial.println();


es.ES_init_ip_arp_udp_tcp(mymac,myip, MYWWWPORT);  
Serial.print( "Netmask: " );
    printIP( mynetmask );
    Serial.println();

    Serial.print( "DNS IP: " );
    printIP( dnsip );
    Serial.println();

    Serial.print( "GW IP: " );
    printIP( gwip );
    Serial.println();
   

  } else {
    Serial.println("Failed to get IP address");

  }
}
//////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////end of Networking in void setup //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////


// other furnctions
// Output a ip address from buffer
void printIP( uint8_t *buf ) {
  for( int i = 0; i < 4; i++ ) {
    Serial.print( buf[i], DEC );
    if( i<3 )
      Serial.print( "." );
  }
}


///////////funcin temperatura//////////////////////
float temperatura (){
sensorValue = analogRead(sensorPin);                //read analog 0    
float v2 = (voltage*float(sensorValue))/1024.0f;    // calculate volts in an awesome world
float r1a = (voltage*float(resistor))/v2;  
float r1 =r1a - resistor;
  //r1=10000; <---uncomment to check if the next eq is working
float T = B/log(r1*unodivr);
//Serial.print("Temperature in Celcius:"); 
T=T-273.15;
 //Serial.println(T, 4);
dtostrf(T, 3, 1, tempBuf);
 return(T);
 }
/////////////////////////////////


void loop() {
  
   uint16_t plen, dat_p; //web server
// DNS
  int sec = 0;
  long lastDnsRequest = 0L;
  dns_state=0;
//end dns

   while(1) {
  // read packet, handle ping and wait for a tcp packet:
  
    plen = es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf);
    dat_p=es.ES_packetloop_icmp_tcp(buf,plen);
    temperatura();
    
  
/////////////////////////////////DNS TEST

if( plen > 0 ) {
      // We have a packet
      // Check if IP data
      if (dat_p == 0) {
        if (es.ES_client_waiting_gw() ){
          // No ARP received for gateway
          continue;
        }
        // It has IP data
        if (dns_state==0){
          sec=0;
          dns_state=1;
          lastDnsRequest = millis();
          es.ES_dnslkup_request(buf,(uint8_t*)WEBSERVER_VHOST);
          
          continue;
        }
        if (dns_state==1 && es.ES_udp_client_check_for_dns_answer( buf, plen ) ){
          dns_state=2;
          es.ES_client_set_wwwip(es.ES_dnslkup_getip());
          Serial.print("pachube ip 1:");
          printIP(es.ES_dnslkup_getip());
        }
        if (dns_state!=2){
          // retry every minute if dns-lookup failed:
          if (millis() > (lastDnsRequest + 60000L) ){
            dns_state=0;
            lastDnsRequest = millis();
          }
          // don't try to use web client before
          // we have a result of dns-lookup
          continue;
        }
      } else {
        if (dns_state==1 && es.ES_udp_client_check_for_dns_answer( buf, plen ) ){
          dns_state=2;
          es.ES_client_set_wwwip(es.ES_dnslkup_getip());
          Serial.print("pachube ip 2:");
          printIP(es.ES_dnslkup_getip());
        }
      }
    }
    // If we have IP address for server and its time then request data

/////////////////////////////////end DNS TEST
     
///////////////////////////////// web server
    // dat_p will be unequal to zero if there is a valid 
     // http get 
    if(dat_p==0){
      continue;
      }
    // tcp port 80 begin
    if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0){
      // head, post and other methods:
      dat_p=http200ok();
      dat_p=es.ES_fill_tcp_data_p(buf,dat_p,PSTR("<h1>200 OK</h1>"));
      goto SENDTCP;
    }
    // just one web page in the "root directory" of the web server
    if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0){
      dat_p=print_webpage(buf);
      goto SENDTCP;
    }
    else{
      dat_p=es.ES_fill_tcp_data_p(buf,0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
      goto SENDTCP;
    }
SENDTCP:
    es.ES_www_server_reply(buf,dat_p); // send web page data
    // tcp port 80 end
  delay (1000);
}

}



//////////////////Other comments/////////////////////////////////
 // plen=temperatura();
 //
 // as talked with George
 //#include<stdlib.h>
//dtostrf(FLOAT,WIDTH,PRECSISION,BUFFER)
// dtostrf(temp, 3, 1, tempBuf); <--- float to string
// char tempBuf[32] = {0};  <-- as seen in https://github.com/Nanode/Webserver-Live-Thermistor-Reading/blob/master/Webserver_Live_Thermistor_Reading.pde
 //
 //
 //
 
 
 /* for(i=0; client_ip[i]!='\0'; i++){
            buf[TCP_DATA_P+plen]=client_ip[i];
            plen++;
        }
 */

