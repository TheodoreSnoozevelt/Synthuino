#include <Arduino.h>
#include <Sound.h>
#include <simple_song.h>


bool doStep = false;

byte testo = 0;

#define BUFFERSIZE 800
byte buffer[BUFFERSIZE];
int bufferPos = 0;

long stepMicros = 0;
long totalMicros = 0;
long totalIterations = 0;
float maxTimePerIteration = 1000000.0 * invSampleRate;

long happySteps = 0;
long sadSteps = 0;

SongData song;

void initializeTimers() {
  cli();

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 3;
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


}

void loop() {
  stepMicros = micros();

  if (doStep) {
    doStep = false;
    byte val = step(&song);

    OCR2B = val;
    if (doStep)
      sadSteps++;
    else
      happySteps++;
  }

  if (totalIterations > 32000) {
    float timePerIteration = (float)totalMicros / totalIterations;
    Serial.print(F("Time per step: "));
    Serial.print(timePerIteration, 5);
    Serial.println(F(" microseconds"));
    Serial.print(F("capacity used: "));
    Serial.print(timePerIteration / maxTimePerIteration * 100);
    Serial.println("%");
    Serial.print(F("Number of good/bad steps: "));
    Serial.print(happySteps);
    Serial.print(" ");
    Serial.print(sadSteps);
    Serial.print(" (");
    Serial.print((float)sadSteps * 100 / happySteps);
    Serial.println(")");
    totalIterations = 0;
    totalMicros = 0;
  }
  
  int diff = micros() - stepMicros;
  totalMicros += diff;
  totalIterations += 1;
}

ISR(TIMER1_COMPA_vect) { 
  doStep = true;
}