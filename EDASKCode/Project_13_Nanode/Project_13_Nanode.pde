// Project 13 - Serial Temperature Sensor

int potPin = 0; 
int span = 500; 
float temperature = 0; 
void setup() 
{ 
  Serial.begin(19200); 
  Serial.println("LM335 Thermometer    "); 
  //analogReference(INTERNAL); 
} 

void loop() { 
  long aRead = 0L; 
  for (int i = 0; i < span; i++) { 
    aRead = aRead+analogRead(potPin); 
  } 
  aRead = aRead / span; 
  aRead = aRead - 10L;                         // korekcija Analog0 vhoda pri 2,93 V (20 st.C)
  //temperature = ((100*1.1*aRead)/1024)*10;  
  temperature = map(aRead, 0, 1023, 0, 5000);  // 0..5V = 0..1023 = 0..500.0 Kelvin
  temperature = (temperature - 2731)/10;      // convert to Celsius = Kelvin - 273.1
  temperature = temperature - 1.5;            // korekcija za 1.5 st.C (napaka LM335)
  // convert voltage to temperature 
  Serial.print("Analog in reading: "); 
  Serial.print(long(aRead));  
  // print temperature value on serial monitor 
  Serial.print(" - Calculated Temp: "); 
//  printTenths(long(temperature)); 
  Serial.println(float(temperature),1); 
  
  delay(1000); 
}
