#include <Arduino.h>
#include <Sound.h>
#include <simple_song.h>


bool doStep = false;



long stepMicros = 0;
long totalMicros = 0;
long totalIterations = 0;
long timeTaken = 0;
float maxTimePerIteration = 1000000.0 * invSampleRate;
SongData song;

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
  OCR2A = 255;
  DDRD |= (1<<PD3);

  sei();
}

void setup() {
  Serial.begin(115200);
  setup_song(&song);
  initializeTimers();
  Serial.print(F("Sample rate: "));
  Serial.print(SAMPLE_RATE);
  Serial.println(F("Hz"));
}

void loop() {
  if (doStep) {
    long start = micros();
    doStep = false;
    byte val = step(&song);

    OCR2B = val;
    
    totalIterations++;
    timeTaken += micros() - start;

    if (totalIterations > 64000) {
      float timePerIteration = (float)timeTaken / totalIterations;
      Serial.print(F("Time per step: "));
      Serial.print(timePerIteration, 5);
      Serial.print(F(" µs ("));
      Serial.print(timePerIteration / maxTimePerIteration * 100);
      Serial.println("%)");
      totalIterations = 0;
      timeTaken = 0;
    }
  }
  
}

ISR(TIMER1_COMPA_vect) { 
  doStep = true;
}