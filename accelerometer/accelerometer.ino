/*
  Measures resp rate through 1ms trigger ISR overflow
 */

int led_red = 8;
int init_led = ##;
int test_pin = ##;
unsigned int tcnt2;
int toggle = 0;
float MS = 0.01;
float period = 0;
int xValue;
int yValue;
int zValue;
int average;

// reset routine
void setup() {
  // initialize serial com
  Serial.begin(9600);
  // initialize red LED 8 as output
  pinMode(led_red, OUTPUT);
  pinMode(init_led, OUTPUT);
  // initialize timer to measure respiratory rate
  timerinit();
  // initialize output pin for testing
  pinMode(test_pin, OUTPUT);
}

// the timer init routine is called by setup
// this init configures the timer 2 to overflow every millisecond
// on overflow, the ISR will execute, incrementing the global count variable
void timerinit(){
  // disable the timer overflow interrupt 
  TIMSK2 &= ~(1<<TOIE2);
  // configure timer 2 in normal mode, no PWM
  TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
  TCCR2B &= ~(1<<WGM22);
  // select clock source: internal IO clock
  ASSR &= ~(1<<AS2);
  //disable compare match - only want overflow
  TIMSK2 &= ~(1<<OCIE2A);
  // configure the prescaler to cpu clock divided by 128
  TCCR2B |= (1<<CS22) | (1<<CS20); // Set Bits
  TCCR2B &= ~(1<<CS21); // Clear Bit
  
  // calculate proper value to load the timer counter
  // (CPU frequency) / (prescaler value) = 125000 Hz = 8us
  // (desired period) / 8us = 125
  // MAX(uint8) + 1 - 125 = 131
  // save value globally for later reload in ISR
  tcnt2 = 131;
  
  // load and enable the timer
  TCNT = tcnt2;
  //TIMSK2 |= (1<< TOIE2);
}

//ISR for timer2 overflow
ISR(TIMER2_OVF_vect){
  // reload the timer
  TCNT = tcnt2;
  // write to digital output pin
  digitalWrite(test_pin, toggle == 0 ? HIGH : LOW);
  toggle = ~toggle;
  // increment period by 1 ms
  period = period + MS;
}

// the loop routine runs over and over again forever:
void loop() {
  readPins();
  // CASE user is laying down
  // z should chill around 600 while others around 500
  // use x - oscillations
  if ( zValue > 540 ){
    // setup average
    initializePinsx();
    // wait while low and determine min value
    while ( xValue < average ){ 
      readPins();
    }
    // wait while high and determine max value
    while ( xValue > average ){
      readPins();
    }
    // start the timer here - breath went low
    TIMSK2 |= (1<< TOIE2);
    // Timer is going up and counting ms - wait one period
    while (xValue < average){
      readPins();
    }
    while (xValue > average){
      readPins();
    }
    // one period is passed - stop the timer and reset
    TIMSK2 &= ~(1<<TOIE2);
    TCNT = tcnt2;
    // great! now number of ms passed should be in period
    Serial.print(period);
  }
}

// reads the values of x,y,z from the analog pins
void readPins(){
  xValue = analogRead(A0);
  yValue = analogRead(A1);
  zValue = analogRead(A2);
  //Serial.print(xValue);
  //Serial.print("\t");
  //Serial.print(yValue);
  //Serial.print("\t");
  //Serial.println(zValue);
}

// initializes the pins such that we have a beginning average value
// should be around 500
void initializePinsx(){
  digitalWrite(init_led, HIGH);
  int xValMax = 0;
  int xValMin = 1024;
  
  readPins();
  // wait before starting
  while(xValue > 500);
  while(xValue < 500);
  
  //start measuring average
  while(xValue > 500){
    readPins();
    if(xValue > xValMax){
      xValMax = xValue;
    }
  }
  while(xValue < 500){
    readPins();
    if(xValue < xValMin){
      xValMin = xValue;
    }
  }
  average = (xValMin + xValMax) / 2;
  digitalWrite(init_led, LOW);
}
