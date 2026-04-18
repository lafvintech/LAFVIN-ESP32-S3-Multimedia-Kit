/**********************************************************************
  Filename    : ADC_Battery
  Description : Use the ADC of the esp32s3 to detect the battery voltage on GPIO19.
  Modification: 2025/11/27
**********************************************************************/
#define PIN_ANALOG_IN  19 // Using GPIO9 as per original request

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ADC Battery Voltage Detector on GPIO9");

  // It's good practice to set attenuation for a predictable voltage range.
  // ADC_11db gives a full-range of 0-3.3V.
  analogSetAttenuation(ADC_11db);
}

void loop() {
  int adcVal = analogRead(PIN_ANALOG_IN);      //Gets the raw adc value.
  double voltage = adcVal / 4095.0 * 3.3;      //Convert to the voltage value at the detection point.
  
  // This calculation assumes a 1/4 voltage divider is connected to the pin.
  // The actual battery voltage would be 4 times the measured voltage.
  double battery = voltage * 4.0;              
  
  Serial.printf("ADC Val: %d, \t Voltage: %.2fV, \t Battery: %.2fV\r\n", adcVal, voltage, battery);
  delay(200);
}
