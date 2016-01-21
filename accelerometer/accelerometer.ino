/*
  Measure respiratory rate functionality
  NOTE: This code revision is not compatible with the current python code and 
        does not write the expected values to the serial port.
  Functions:
    setup(): Set up function. Runs once on device reset. initializes serial
                 port communication, output LEDs, timer, and output pin.
    timerinit(): Initializes the timer that is used to control the respiratory
                 rate time measurement functionality. The timer is configured
                 to count up normally, and overflow once every millisecond. This
                 allows us to determine how many milliseconds have passed by counting 
                 how many overflows have occurred.
    ISR(): Interrupt service routine. On the interrupt trigger (in this case the
                 timer overflow), the interrupt service routine is called.
                 If you dont know what an ISR is:
                 https://en.wikipedia.org/wiki/Interrupt_handler
    loop(): The function that implements the bulk of the arduino functionality, and
                 repeats indefinitely.
    readPins(): does a simple analog read of the x,y,z values from the accelerometer.
    initializePinsx(): initializes the 'average' analog value to better approximate 
                       the length of a single patient breath.
*/

// global variables
/*
  led_red: the red LED that is turned on when the initialize_pins function is being executed.
            indicates that the device is setting up.
  test_pin: the digital pin that will oscillate at the same speed of ISR trigger. The pin 
            should change value once every millisecond (period = 2ms). You should use an
            oscilloscope to confirm the accuracy of the ISR frequency...
  tcnt2: the reload value of the timer to ensure that the timer will overflow every millisecond.
            this is the value that would have to be fine tuned if the ISR frequency was not right.
  toggle: the variable that is assigned to the test_pin to make it ascillate
  MS: a float 'constant' that represents one millisecond, or 0.001 seconds. Added to the period 
            when the ISR is triggered.
  period: the time that has passed during the current breath cycle. Printed to the serial port.
  xValue, yValue, zValue: the variables that keep track of the x, y, z values read from the 
                          accelerometer.
  average: the variable that is initialized in initializePins routines to better approximate the average 
            value recieved on the accelerometer pins
*/
int led_red = 8;
int test_pin = ##;
unsigned int tcnt2;
int toggle = 0;
float MS = 0.001;
float period = 0;
int xValue;
int yValue;
int zValue;
int average;

// reset routine, completed on device reset
/*
  initializes output pins led_red and test_pin
  initializes serial port communication
  call timerinit() function to define timer behaviour
*/
void setup() {
  // initialize serial com
  Serial.begin(9600);
  // initialize red LED 8 as output
  pinMode(led_red, OUTPUT);
  // initialize timer to measure respiratory rate
  timerinit();
  // initialize output pin for testing
  pinMode(test_pin, OUTPUT);
}

// the timer init routine is called by setup
// this init configures the timer 2 to overflow every millisecond
// on overflow, the ISR will execute, incrementing the global period variable
/*
  SECTIONS BELOW REFER TO THE ATMEGA328 DATASHEET FOUND ON SPARKFUN
  This is the best explanation I can manage for this function (I'm not an expert)
  This funciton writes to a bunch of hardware registers on the ATMega that
  initialize behaviour of a timer called timer 2. 
  These websites introduce some of the concepts
  http://maxembedded.com/2011/06/avr-timers-timer2/
  http://maxembedded.com/2011/06/introduction-to-avr-timers/
  In plain English, this function does the following things:
    turns off the interrupts so that we can edit the timer
    configures the timer to work in 'normal' mode (we just need it to count up normally)
    selects the clock source (sets it to be an on-board oscillator)
    sets the interrupt mask to trigger interrupt when the timer overflows
    prescales the timer to a manageable frequency
    calculates the reload-value that we want the timer to be initialized to
  basically, the timer will work by being initialized to a value and counting up indefinitely.
  however, the timer is only 8 bits, so eventually the timer will max out and overflow.
  we want that to happen every millisecond, so we need to calculate a reload value accordingly.
*/
void timerinit(){
  // disable the timer overflow interrupt 
  // Section 17.11.6
  TIMSK2 &= ~(1<<TOIE2);
  // configure timer 2 in normal mode, no PWM
  // Section 17.11.1
  TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
  // Section 17.11.2
  TCCR2B &= ~(1<<WGM22);
  // select clock source: internal IO clock
  // Section 17.11.8
  ASSR &= ~(1<<AS2);
  // disable compare match - only want overflow
  // Section 17.11.6
  TIMSK2 &= ~(1<<OCIE2A);
  // configure the prescaler to cpu clock divided by 128
  // Section 17.11.2
  TCCR2B |= (1<<CS22) | (1<<CS20); // Set Bits
  TCCR2B &= ~(1<<CS21); // Clear Bit
  
  // calculate proper value to load the timer counter
  // frequency of timer increment: (CPU frequency) / (prescaler value = 128) = 125000 Hz = 8us
  // (desired period = 1ms) / 8us = 125
  // MAX(uint8 = 255) + 1 - 125 = 131
  // save value globally for later reload in ISR
  tcnt2 = 131;
  
  // load and enable the timer
  TCNT = tcnt2;
  // Don't enable the timer yet.. we want to synchronize it with a breath
  //TIMSK2 |= (1<< TOIE2);
}

//ISR for timer2 overflow
ISR(TIMER2_OVF_vect){
  // reload the timer
  TCNT = tcnt2;
  // write to digital output pin to see with oscilloscope
  digitalWrite(test_pin, toggle == 0 ? HIGH : LOW);
  toggle = ~toggle;
  // increment period by 1 ms
  period = period + MS;
}

// the loop routine runs over and over again forever:
void loop() {
  readPins();
  
  // CASE 1 user is laying down
  // z should chill around 600 while others around 500
  // use x - oscillations
  if ( zValue > 540 ){
    // setup average
    initializePinsx();
    // now that we have initialized the average, we shouldn't do it again - it takes a few breaths
    // stay in while loop while we're still in the same position. Should only leave if the values are 
    // changing.
    while( zValue > 540){
      int min = 1024;
      int max = 0;
      // wait while low and determine min value
      while ( xValue < average ){ 
        readPins();
        if(xValue < min){
          min = xValue;
        }
      }
      // wait while high and determine max value
      while ( xValue > average ){
        readPins();
        if(xValue > max){
          max = xValue;
        }
      }
      // update averge constantly
      average = (min + max)/2;
      // start the timer here - breath went low
      TIMSK2 |= (1<< TOIE2);
      // Timer is going up and counting ms - wait one period
      while (xValue < average){
        readPins();
      }
      while (xValue > average){
        readPins();
      }
      // one period has passed - stop the timer and reset timer value
      TIMSK2 &= ~(1<<TOIE2);
      TCNT = tcnt2;
      // great! now number of ms passed should be in period
      Serial.print(period);
      // reset period to 0 so that we can go back to the beginning and increment again
      // reset to measure next breath
      period = 0;
    }
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
// assumes the user is lying down - looks at x value from the accelerometer only
// update global variable average for use in loop()
void initializePinsx(){
  digitalWrite(led_red, HIGH);
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
  digitalWrite(led_red, LOW);
}
