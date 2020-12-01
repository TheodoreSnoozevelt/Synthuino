#include <Arduino.h>
#include <Sound.h>

#define LENGTH 256
#define AMP 127
#define OFFSET 128
#define PI2 6.283185
#define SAMPLE_RATE 15625

const float inv255 = 1 / 255.0;
const float inv2048 = 1 / 2048.0;
const float invSampleRate = 1.0 / SAMPLE_RATE;

bool doStep = false;

uint8_t sineTable[LENGTH];
uint8_t squareTable[LENGTH];
uint8_t triangleTable[LENGTH];

OscValues synthOsc;
OscValues synthOsc2;

ReleaseEnvelope<3> relOp;
ReleaseEnvelope<3> relOp2;

StepSequencer<float> melody1Sequencer;
StepSequencer<float> melody2Sequencer;


#define MELODY_SIZE 11
int8_t melody[MELODY_SIZE] = { 
  0, 5, 4, 1, 8, 7, 10, 5, 8, 4, 5
};
float melodyFreq[MELODY_SIZE];

#define MELODY2_SIZE 5
int8_t melody2[MELODY2_SIZE] = { 
  0, 1, 0, 5, 4
};
float melody2Freq[MELODY2_SIZE];

#define BPM 240
#define BPS 4

int stepCounter = 0;

#define BUFFERSIZE 800
byte buffer[BUFFERSIZE];
int bufferPos = 0;

long stepMicros = 0;
long totalMicros = 0;
long totalIterations = 0;
float maxTimePerIteration = 1000000.0 * invSampleRate;

long happySteps = 0;
long sadSteps = 0;

void initializeTables() {
  for (int i=0; i < MELODY_SIZE; i++) {
    int note = melody[i];
    if (note < 0) {
      melodyFreq[i] = 0;
    }
    else {
      melodyFreq[i] = 220 * pow(2, note / 12.0);
    }
  }

  for (int i=0; i < MELODY2_SIZE; i++) {
    int note = melody2[i];
    if (note < 0) {
      melody2Freq[i] = 0;
    }
    else {
      melody2Freq[i] = 110 * pow(2, note / 12.0);
    }
  }

  for (int i=0; i<LENGTH; i++) { 
    float v = (AMP*sin((PI2/LENGTH)*i)); 
    sineTable[i] = int(v+OFFSET);
    squareTable[i] = (i < LENGTH/2) ? 255 : 0;
    triangleTable[i] = i;
  }
}

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
  initializeTables();
  initializeTimers();

  synthOsc.length = 256;
  synthOsc2.length = 256;
  relOp.stepsPerReduction = 22;
  relOp2.stepsPerReduction = 33;

  melody1Sequencer.duration = SAMPLE_RATE / 6;
  melody1Sequencer.steps = MELODY_SIZE;
  melody1Sequencer.source = melodyFreq;
  melody1Sequencer.init();

  melody2Sequencer.duration = SAMPLE_RATE / 8;
  melody2Sequencer.steps = MELODY2_SIZE;
  melody2Sequencer.source = melody2Freq;
  melody2Sequencer.init();
}

void loop() {
  stepMicros = micros();

  if (doStep) {
    doStep = false;

    if (stepCounter == 0) {
      synthOsc.frequency = melody1Sequencer.output;
      OSC_CALCULATE_PHASE_STEP(synthOsc, invSampleRate);
    }

    if (stepCounter == 1) {
      synthOsc2.frequency = melody2Sequencer.output;
      OSC_CALCULATE_PHASE_STEP(synthOsc2, invSampleRate);
    }

    stepCounter++;
    if (stepCounter > 100)
      stepCounter = 0;

    OSC_STEP(synthOsc2, triangleTable);
    OSC_STEP(synthOsc, squareTable);

    relOp.inputGate = melody1Sequencer.gate;
    relOp.inputValue = synthOsc.output;
    relOp.step();

    relOp2.inputGate = melody2Sequencer.gate;
    relOp2.inputValue = synthOsc2.output;
    relOp2.step();


    melody1Sequencer.step();
    melody2Sequencer.step();
        
    int val = (relOp2.outputValue >> 2) + (relOp.outputValue >> 2);
    OCR2B = val;
    if (doStep)
      sadSteps++;
    else
      happySteps++;
  }

  if (totalIterations > 100000) {
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