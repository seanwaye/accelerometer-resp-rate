/*
  AnalogReadSerial
  Reads an analog input on pin 0, prints the result to the serial monitor.
  Attach the center pin of a potentiometer to pin A0, and the outside pins to +5V and ground.
 This example code is in the public domain.
 */

int led_red = 8;

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  pinMode(led_red, OUTPUT);
  timerinit();
}

/* TO DO 
   -  interrupt for printing to serial port
   -  timer for counting bpm
   -  calculations for bpm
*/

// the loop routine runs over and over again forever:
void loop() {
  // THIS IS SET UP - only happens once
  // read the input on analog pin 0:
  int xValue = analogRead(A0);
  int yValue = analogRead(A1);
  int zValue = analogRead(A2);
  int average;
  int counter = 0;
  
  // BIG SUPERLOOP 
  while (1) {
    Serial.print(xValue);
    Serial.print("\t");
    Serial.print(yValue);
    Serial.print("\t");
    Serial.println(zValue);
    
    if ( z > 540 ){  // start with user is laying down
      while ( xValue < average );
      while ( xValue > average );
      // start the timer here
      TCCR1B |= (1 << CS10);
      while ( xValue > average ) {
        if ( /* overflow happened */ ){
          // set overflow flag to 0
          count = count + 1;
        }
      }
      while ( xValue < average ) {
        if ( /* overflow happened */ ){
          // set overflow flag to 0
          count = count + 1;
        }
      }
      // stop the timer here
      TCCR1B = 0;
      // the number [count - TCNT1] has the period
      // period = ??
      // bpm = 1/(period/60);
      // bpmint = bpm;
    }
  }
}

void timerinit(){
  // set bits of timer to 0
  // set mode
  // stop timer
  TCCR1A = 0;    // set entire TCCR1A register to 0
  TCCR1B = 0;    // set entire TCCR1B register to 0 
                 // (as we do not know the initial  values) 
  TCCR1B |= (1 << CS10); // Sets bit CS10 in TCCR1B
  
}
