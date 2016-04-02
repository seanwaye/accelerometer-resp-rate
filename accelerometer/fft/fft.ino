#include "PlainFFT.h"

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

const uint16_t samples = 64;
double signalFrequency = 1000;
double samplingFrequency = 5000;
uint8_t signalIntensity = 100;

// These are input and output vectors

double vReal[samples];
double vImag[samples];
uint8_t runOnce = 0x00;

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02


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
int test_pin = 2;
unsigned int tcnt2;
int toggle = 0;
float MS = 0.001;
float period = 0;
int xValue;
int yValue;
int zValue;
int average;
// calculate proper value to load the timer counter
// frequency of timer increment: (CPU frequency) / (prescaler value = 128) = 125000 Hz = 8us
// (desired period = 1ms) / 8us = 125
// MAX(uint8 = 255) + 1 - 125 = 131
// save value globally for later reload in ISR
int tcnt2 = 131;

// reset routine, completed on device reset
/*
  initializes output pins led_red and test_pin
  initializes serial port communication
  call timerinit() function to define timer behaviour
*/
void setup() {
  // initialize serial com
  Serial.begin(9600);
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
  
  // load and enable the timer
  TCNT2 = tcnt2;
  // Don't enable the timer yet.. we want to synchronize it with a breath
  //TIMSK2 |= (1<< TOIE2);
}

//ISR for timer2 overflow
ISR(TIMER2_OVF_vect){
  // reload the timer
  TCNT2 = tcnt2;
  // write to digital output pin to see with oscilloscope
  digitalWrite(test_pin, toggle == 0 ? HIGH : LOW);
  toggle = ~toggle;
  // read from accelerometer every X ms
  readPins();
}

// the loop routine runs over and over again forever:
void loop() {
  if (runOnce == 0x00) {
        runOnce = 0x01;
        // Collect data
        // Want to wait until we populate an array of data points by reading from accelerometer
        
        printVector(vReal, samples, SCL_TIME);
        FFT.windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);    // Weigh data
        printVector(vReal, samples, SCL_TIME);
        FFT.compute(vReal, vImag, samples, FFT_FORWARD); // Compute FFT
        printVector(vReal, samples, SCL_INDEX);
        printVector(vImag, samples, SCL_INDEX);
        FFT.complexToMagnitude(vReal, vImag, samples); // Compute magnitudes
        printVector(vReal, (samples >> 1), SCL_FREQUENCY);   
        double x = FFT.majorPeak(vReal, samples, samplingFrequency);
        Serial.println(x, 6);
    }
}

void printVector(double *vD, uint8_t n, uint8_t scaleType) {
    double timeInterval = (1.0 / samplingFrequency);
    for (uint16_t i = 0; i < n; i++) {
        // Print abscissa value
        switch (scaleType) {
        case SCL_INDEX:
            Serial.print(i, DEC);
            break;
        case SCL_TIME:
            Serial.print((i * timeInterval), 6);
            break;
        case SCL_FREQUENCY:
            Serial.print((i / (timeInterval * (samples-1))), 6);
            break;
        }
        Serial.print(" ");
        // Print ordinate value
        Serial.print(vD[i], 6);
        Serial.println();
    }
    Serial.println();
}


// reads the values of x,y,z from the analog pins
void readPins(){
  xValue = analogRead(A0);
  yValue = analogRead(A1);
  zValue = analogRead(A2);
  Serial.print(average);
  Serial.print("\t");
  Serial.print(period);
  Serial.print("\t");
  Serial.print(xValue); //un-uncommented serial.print lines here
  Serial.print("\t");
  Serial.print(yValue);
  Serial.print("\t");
  Serial.println(zValue);
}


