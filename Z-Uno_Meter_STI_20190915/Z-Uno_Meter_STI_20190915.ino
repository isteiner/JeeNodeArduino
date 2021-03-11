/*
  Z-Uno meter, as an example

 */

// 2019-09-15 <isteiner> example

//initial period of the blink is 1 second
byte dimmerValue=100;

//Next line sets up Z-Uno channels. In this case it adds the Switch Multilevel channel
//ZUNO_SETUP_CHANNELS(ZUNO_SWITCH_MULTILEVEL(getSwitchMultilevelValue,setSwitchMultilevelValue));
//ZUNO_SETUP_CHANNELS(ZUNO_SWITCH_MULTILEVEL(getter, setter));

ZUNO_SETUP_CHANNELS(
    ZUNO_METER(
        ZUNO_METER_TYPE_ELECTRIC,
        METER_RESET_ENABLE,
        ZUNO_METER_ELECTRIC_SCALE_KWH,
        ZUNO_METER_ELECTRIC_SCALE_WATTS,
        ZUNO_METER_ELECTRIC_SCALE_AMPS,
        getterFunction,
        resetterFunction
    )
);

int currentValue;

// the setup function runs once, when you press reset or power the board
void setup() {
  // set digital pin 13 and 14 as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(14, OUTPUT);  
  dimmerValue = 100;
  Serial.begin();         // For USB baudrate is fixed on 115200 bps 
}

// the loop function runs over and over again forever
void loop() {
  //digitalWrite(LED_BUILTIN, HIGH); // turn the LED on (HIGH is the voltage level)
  if (dimmerValue > 0) digitalWrite(14, HIGH); // switch ON
  else digitalWrite(14, LOW);                 // switch OFF
  //delay(dimmerValue*10);           // wait for timeout
  //digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
  //if (dimmerValue == 0) digitalWrite(14, LOW);  
  //delay(dimmerValue*10);           // wait for timeout
  Serial.println(currentValue);
}

//The setSwitchMultilevelValue runs only when a new value comes from the controller by Z-Wave
void setSwitchMultilevelValue(byte newValue) {
  //save new value in a variable
  dimmerValue = newValue;
}

//The setSwitchMultilevelValue runs only when the controller asks for the current blink rate by Z-Wave
byte getSwitchMultilevelValue(void) {
  //return previously saved value
  return dimmerValue;
}

DWORD getterFunction() {
    currentValue++;
    return currentValue;
}

void resetterFunction() {
    currentValue = 0;
}
