#include <Arduino.h>
#include <Sound.h>

#define LENGTH 256
#define AMP 127
#define OFFSET 128
#define PI2 6.283185

const float inv255 = 1 / 255.0;
const float inv2048 = 1 / 2048.0;
const float invSampleRate = 1 / 15625.0;

bool doStep = false;

uint8_t sineTable[LENGTH];
uint8_t squareTable[LENGTH];
uint8_t triangleTable[LENGTH];

OscValues synthOsc;
OscValues synthOsc2;
OscValues lfo;
OscValues bassDrumOsc;
OscValues envelopeGenerator;

Oscillator bassOsc;

StepSequencer<float> melodySequencer;

StepSequencer<int8_t> drumSequencer;
int8_t currentDrumValue = 0;

int8_t melody[32] = { 
  0, -1, 0, 7, 5, -1, 3, -1, 
  2, -1, 2, 2, 5, -1, 3, 2,
  0, -1, 0, 15, 14, 15, 14, 15,
  0, -1, 0, 15, 14, 15, 14, 15, 
};
float melodyFreq[32];
uint8_t melodyIndex = 0;

int8_t drumBeat[2] = {1, 0};
uint8_t drumIndex = 0;

uint8_t envelope[256];
uint8_t envelopeIndex = 0;
uint8_t playEnvelope = 1;
float currentEnvelopeValue = 0;

int stepCounter = 0;
byte dutyCycle = 0;

#define BUFFERSIZE 800
byte buffer[BUFFERSIZE];
int bufferReadPos = 0;
int bufferWritePos = BUFFERSIZE / 2;

long stepMicros = 0;
long totalMicros = 0;
long totalIterations = 0;
float maxTimePerIteration = 1000000.0 * invSampleRate;

long happySteps = 0;
long sadSteps = 0;

void initializeTables() {
  for (int i=0; i < 32; i++) {
    int note = melody[i];
    if (note < 0) {
      melodyFreq[i] = 0;
    }
    else {
      melodyFreq[i] = 440 * pow(2, note / 12.0);
    }
  }

  for (int i=0; i < 128; i++) {
    float norm = i * 2 * inv255;
    envelope[i] = 255 - pow(norm, 1.1) * 255;
  }

  for (int i=0; i<LENGTH; i++) { 
    float v = (AMP*sin((PI2/LENGTH)*i)); 
    sineTable[i] = int(v+OFFSET);
    squareTable[i] = i < LENGTH/2 ? 255 : 0;
    triangleTable[i] = i;
    float norm = i * inv255;
    envelope[i] = 255 - pow(norm, .3) * 255;
    Serial.print("env(");
    Serial.print(i);
    Serial.print(") = ");
    Serial.println(envelope[i]);
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
  Serial.begin(9600);
  initializeTables();
  initializeTimers();

  melodySequencer.source = melodyFreq;
  melodySequencer.steps = 32;
  melodySequencer.duration = 4000;

  drumSequencer.source = drumBeat;
  drumSequencer.steps = 2;
  drumSequencer.duration = 4000;

  synthOsc.length = LENGTH;
  synthOsc.frequency = 220.0;
  osc_calculate_phase_step(&synthOsc, invSampleRate);

  synthOsc2.length = LENGTH;
  synthOsc2.frequency = 110.0;
  osc_calculate_phase_step(&synthOsc2, invSampleRate);

  lfo.length = LENGTH;
  lfo.frequency = 99.0;
  osc_calculate_phase_step(&lfo, invSampleRate);

  envelopeGenerator.length = LENGTH;
  envelopeGenerator.phaseStep = 0;

  bassDrumOsc.length = LENGTH;
  bassDrumOsc.frequency = 220;
}

void loop() {
  stepMicros = micros();

  if (doStep) {
    doStep = false;

    stepCounter++;
    
    OSC_STEP(synthOsc, sineTable);
    OSC_STEP(synthOsc2, sineTable);
    OSC_STEP(lfo, sineTable);
    OSC_STEP(bassDrumOsc, sineTable);
    OSC_STEP(envelopeGenerator, envelope);

    melodySequencer.step();
    drumSequencer.step();


    if (currentDrumValue == 0 && drumSequencer.output == 1) {
      currentDrumValue = 1;
      envelopeGenerator.frequency = 1.5;
      OSC_CALCULATE_PHASE_STEP(envelopeGenerator, invSampleRate);
    }

    if (currentDrumValue == 1 && drumSequencer.output == 0) {
      currentDrumValue = 0;
      envelopeGenerator.phaseStep = 0;
      envelopeGenerator.currentStep = 0;
      bassDrumOsc.phaseStep = 0;
      bassDrumOsc.output = 0;
      bassDrumOsc.currentStep = 0;
    }

    if (envelopeGenerator.output < 100) {
      envelopeGenerator.phaseStep = 0;
      envelopeGenerator.currentStep = 0;
      bassDrumOsc.phaseStep = 0;
    }


    if (stepCounter == 50) {
      if (currentDrumValue == 1) {
        bassDrumOsc.frequency = 500.0 * envelopeGenerator.output * inv255;
        OSC_CALCULATE_PHASE_STEP(bassDrumOsc, invSampleRate);
      }

      synthOsc.frequency = melodySequencer.output;
      OSC_CALCULATE_PHASE_STEP(synthOsc, invSampleRate);
      synthOsc2.frequency = melodySequencer.output * 4.0;
      OSC_CALCULATE_PHASE_STEP(synthOsc2, invSampleRate);
      stepCounter = 0;
    }
    
    int val = 
    //OCR2B = 
      ((synthOsc.output)>>2) + 
     /// (synthOsc2.output >> 3) + 
      ((bassDrumOsc.output)>>2);

    if (bufferWritePos == BUFFERSIZE)
      bufferWritePos = 0;
    buffer[bufferWritePos++] = val > 255 ? 255 : val;      
  }

  if (totalIterations > 100000) {
    float timePerIteration = (float)totalMicros / totalIterations;
    //Serial.print("Time per step: ");
    //Serial.print(timePerIteration, 5);
    //Serial.println(" microseconds");
    //Serial.print("capacity used: ");
    Serial.print(timePerIteration / maxTimePerIteration * 100);
    Serial.println("%");
    totalIterations = 0;
    totalMicros = 0;

    Serial.print(bufferReadPos);
    Serial.print(" ");
    Serial.println(bufferWritePos);
  }
  
  int diff = micros() - stepMicros;
  totalMicros += diff;
  totalIterations += 1;
}

ISR(TIMER1_COMPA_vect) { 
  doStep = true;
  if (bufferReadPos == BUFFERSIZE)
    bufferReadPos = 0;
  OCR2B = buffer[bufferReadPos++];
}