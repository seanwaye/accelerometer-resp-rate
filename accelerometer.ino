/*
  AnalogReadSerial
  Reads an analog input on pin 0, prints the result to the serial monitor.
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.
 This example code is in the public domain.
 */
 
// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int xValue = analogRead(A0);
  int zValue = analogRead(A1);
  int yValue = analogRead(A2);
  
  // print out the value you read:
  Serial.print("X value is: ");
  Serial.println(xValue);
  Serial.print("Y value is: ");
  Serial.println(yValue);
  Serial.print("Z value is: ");
  Serial.println(zValue);
  Serial.println(" "); // Makes a space between the readings
  delay(1000);  // delay of 1000 ms (1 second) - Gives us time to better read the Serial Monitor
                // speed this up by lowering the number
}
