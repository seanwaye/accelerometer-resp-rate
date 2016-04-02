/*

  Example of use of the ADC and FFT libraries
  Copyright (C) 2010 Didier Longueville

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

/* Printing function options */
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02

#include 
#include 
PlainADC ADC = PlainADC(); /* Create ADC object */
PlainFFT FFT = PlainFFT(); /* Create FFT object */

/* User defined variables */
const uint16_t samples = 128;
uint16_t frequency = 20000;
uint8_t channel = 0;
/* Data vectors */
uint8_t vData[samples]; 
double vReal[samples]; 
double vImag[samples];

void setup(){  
  /* Initialize serial comm port */
  Serial.begin(115200); // 
  /* Set acquisition parameters */
  ADC.setAcquisitionParameters(channel, samples, frequency);
}

void loop() {
  /* Acquire data and store them in a vector of bytes */
  ADC.acquireData(vData);
  /* Convert 8 bits unsigned data in 32 bits floats */
  for (uint16_t i = 0; i < samples; i++) {
    vReal[i] = double(vData[i]);
  }
  /* Weigh data */
  FFT.windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  
  /* Compute FFT: vReal and vImag vectors contain the source data 
  and will contain the result data on completion of executing the function*/
  FFT.compute(vReal, vImag, samples, FFT_FORWARD);
  /* Compute magnitudes: the resulting data can be read from the vReal vector */
  FFT.complexToMagnitude(vReal, vImag, samples); 
  /* Upload frequency spectrum */
  printVector(vReal, (samples >> 1), SCL_FREQUENCY);
  /* Pause */
  delay(5000);
}

void printVector(double *vD, uint8_t n, uint8_t scaleType) {
/* Mulitpurpose printing function */
  double timeInterval = (1.0 / frequency);
  for (uint16_t i = 0; i < n; i++) {
    /* Print abscissa value */
    switch (scaleType) {
    case SCL_INDEX: Serial.print(i, DEC); break;
    case SCL_TIME: Serial.print((i * timeInterval), 6); break;
    case SCL_FREQUENCY: Serial.print((i / (timeInterval * (samples-1))), 6); break;
    }
    Serial.print(" ");
    /* Print ordinate value */
    Serial.print(vD[i], 6);
    Serial.println();
  }
  Serial.println();
}


