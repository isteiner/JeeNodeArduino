// Project 13 - Serial Temperature Sensor

int potPin = 0; 
float temperature = 0; 
void setup() 
{ 
  Serial.begin(9600); 
  Serial.println("LM35 Thermometer     "); 
  analogReference(INTERNAL); 
} 
void printTenths(int value) { 
  // prints a value of 123 as 12.3 
    Serial.print(value / 10); 
    Serial.print("."); 
    Serial.println(value % 10); 
} 
void loop() { 
  int span = 20; 
  int aRead = 0; 
  for (int i = 0; i < span; i++) { 
    aRead = aRead+analogRead(potPin); 
  } 
  aRead = aRead / 20; 
  
  temperature = ((100*1.1*aRead)/1024)*10;  
  // convert voltage to temperature 
  Serial.print("Analog in reading: "); 
  Serial.print(long(aRead));  
  // print temperature value on serial monitor 
  Serial.print(" - Calculated Temp: "); 
  printTenths(long(temperature)); 
  
  delay(500); 
}