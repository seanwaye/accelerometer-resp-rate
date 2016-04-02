void PlainADC::setAcquisitionParameters(uint8_t channel, uint16_t samplesBufferSize, uint16_t frequency) {
    // Set acquisition parameters
    if (acqEngineStatus != ADC_ACQ_ENG_STOPPED) stopAcquisitionEngine();
    _samplesBufferSize = samplesBufferSize;
    _frequency = frequency;
    uint16_t timeInterval = (1000000UL / frequency); // In us from 50000 (20 hz) to 16 (64 kHz)
    cli();
    // set ADC
    // Clear control registers
    ADMUX  = 0x00;
    ADCSRA = 0x00;
    ADCSRB = 0x00;
    // 8-Bit ADC in ADCH Register (left aligned data)
    ADMUX |= (1 << ADLAR);
    // VCC as a Reference
    ADMUX |= (1 << REFS0);
    // Set channel
    ADMUX |= channel;
    // Compute adc prescaler value automatically, allowed value are 2^1 to 2^7
    _adcPrescaler = 128;
    // Decrease prescaler down to 16; for some reasons, decreasing below 16 drives to unaccurate timing
    while ((_adcPrescaler > uint16_t((timeInterval << 4) / 13)) && (_adcPrescaler > 16)) _adcPrescaler = (_adcPrescaler >> 1);
    // Set prescaler
    uint8_t adcPrescalerExponent = exponent(2, _adcPrescaler);
    ADCSRA |= (adcPrescalerExponent >> 1);
    // Enable ADC
    ADCSRA |= (1 << ADEN);
    // ADC Auto Trigger Enable
    ADCSRA |= (1 << ADATE);
    // ADC interrupt enable
    ADCSRA |= (1 << ADIE);
    // ADC Auto Trigger Source Selection
    ADCSRB |= ((1 << ADTS0) | (1 << ADTS2));
    // Set Timer1
    // Reset Timer/Counter1 control registers and Interrupt Mask Register
    TCCR1A = 0;
    TCCR1B = 0;
    TIMSK1 = 0;
    // Set Clear Timer on Compare Match (CTC) Mode
    TCCR1B |=  (1 << WGM12);
    // Compute timer prescaler, allowed values are 1, 8 or 64
    if (timeInterval < 4096) _timPrescaler = 1;
    else if (timeInterval < 32768) _timPrescaler = 8;
    else _timPrescaler = 64;
    uint8_t timPrescalerExponent = exponent(8, _timPrescaler);
    // Set prescaler
    TCCR1B |=  (timPrescalerExponent + 0x01);
    // Set upper count value: (F_CPU*Interval)/Prescaler
    _timUpperCount = (16000000UL / (frequency * _timPrescaler));
    OCR1A = _timUpperCount;
    // Reset Timer/Counter2
    TIMSK2 = 0; 
    //
    sei();
    startAcquisitionEngine();
}

void PlainADC::acquireData(uint8_t *vData) {
    if (acqEngineStatus == ADC_ACQ_ENG_STOPPED) startAcquisitionEngine();
    uint8_t originalTIMSK0 = TIMSK0; // Record timer/counter0 mask
    TIMSK0 = 0x00; // Disable timer/counter0
    // Reset number of acquired samples
    uint16_t samples = 0;
    dataAcqStatus = ADC_DAT_ACQ_WAITING;
    // Set acquisition status
    do {
        if (dataAcqStatus == ADC_DAT_ACQ_TRIGGERED) {
            PORTB ^=  (1 << PINB5); // Comment in normal operation mode
            vData[samples] = adcValue; // Store data
            samples++; // Increment sample counts
            dataAcqStatus = ADC_DAT_ACQ_WAITING; // Toggle status
        }
    } while (samples != _samplesBufferSize);
    dataAcqStatus != ADC_DAT_ACQ_IDLE; // Reset status
    TIMSK0 = originalTIMSK0; // Restore timer/counter0 operation
}
