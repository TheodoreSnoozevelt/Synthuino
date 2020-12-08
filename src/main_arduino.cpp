#include <Arduino.h>
#include <Sound.h>
#include <simple_song.h>
#include <avr/io.h>

bool doStep = false;

long stepMicros = 0;
long totalMicros = 0;
long totalIterations = 0;
long timeTaken = 0;
float maxTimePerIteration = 1000000.0 * invSampleRate;

SongData song;

unsigned short adcV = 0;
float adcFloat = 1;

void initializeTimers() {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = CLOCK_DIVIDER;
  TCCR1B |= (1 << CS12);
  TCCR1B |= (1 << WGM12);
  TIMSK1 |= (1 << OCIE1A);

  TCCR2A = (1<<COM2B1) + (1<<WGM21) + (1<<WGM20);
  TCCR2B = (1<<CS20) + (1<<WGM22);
  OCR2A = 127;
  DDRD |= (1<<PD3);

  sei();
}

void initialize_adc() {
  ADMUX=(1<<REFS0); //Vref=AVcc
  // ADSC=1 ADC Enable
  // ADPS[2:0]=111, prescaler=128
  // ADIE=1, ADC interrupt Enable
  //ADATE=1, ADC Auto Triggering Enable
  ADCSRA=(1<<ADEN)|(7<<ADPS0)|(1<<ADSC)|(1<<ADIE)|(1<<ADATE);
  
  //ADTS[2:0]= 100 , Timer0 overflow select as trigger source
  ADCSRB=(4<<ADTS0);
}

void setup() {
  Serial.begin(115200);
  setup_song(&song);
  initializeTimers();
  initialize_adc();
  Serial.print(F("Sample rate: "));
  Serial.print(SAMPLE_RATE);
  Serial.println(F("Hz"));
}

void loop() {
  if (doStep) {
    long start = micros();
    doStep = false;
    byte val = step(&song);
    song.FM = adcFloat;

    OCR2B = val >> 1;
    
    totalIterations++;
    timeTaken += micros() - start;

    if (totalIterations > 64000) {
      float timePerIteration = (float)timeTaken / totalIterations;
      Serial.print(F("Time per step: "));
      Serial.print(timePerIteration, 5);
      Serial.print(F(" µs ("));
      Serial.print(timePerIteration / maxTimePerIteration * 100);
      Serial.println("%)");
      Serial.print("ADC: ");
      Serial.println(adcFloat);
      totalIterations = 0;
      timeTaken = 0;
    }
  }
  
}

ISR(TIMER1_COMPA_vect) { 
  doStep = true;
}

ISR(ADC_vect)
{
  adcFloat = (1024 - ADC) / 1024.0;
}