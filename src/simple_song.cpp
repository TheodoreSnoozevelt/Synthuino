#include <simple_song.h>
#include <stdint.h>
#include <stdlib.h>


#define DELAYLENGTH 512
const int offset = 499;
static unsigned char buffer[DELAYLENGTH] = {0};
static unsigned int bufferIndex = 0;

unsigned char reverbStep(unsigned char value) {
  unsigned int tempWriteIndex = (bufferIndex + offset);
  unsigned int writeIndex1 = tempWriteIndex & ((1 << 9) - 1); 
  unsigned char newValue = value + buffer[bufferIndex];
  buffer[writeIndex1] = scale<3>(newValue, 2);
  bufferIndex++;
  bufferIndex = bufferIndex & ((1 << 9) - 1);
  return scale<2>(newValue, 2);
}

void setup_song(SongData *data) {
  data->stepCounter = 0;
  for (int i=0; i < MELODY_SIZE; i++) {
    int note = data->melody[i];
    if (note < 0) {
      data->melodyFreq[i] = 0;
    }
    else {
      data->melodyFreq[i] = 220 * pow(2, note / 12.0);
    }
  }

  for (int i=0; i < MELODY2_SIZE; i++) {
    int note = data->melody2[i];
    if (note < 0) {
      data->melody2Freq[i] = 0;
    }
    else {
      data->melody2Freq[i] = 110 * pow(2, note / 12.0);
    }
  }

  for (int i=0; i<LENGTH; i++) { 
    float v = (AMP*sin((PI2/LENGTH)*i)); 
    data->sineTable[i] = int(v+OFFSET);
    data->squareTable[i] = (i < LENGTH/2) ? 255 : 0;
    data->triangleTable[i] = i;
  }

  data->synthOsc.length = 256;
  data->synthOsc.currentStep = 0;
  data->synthOsc.phaseStep = 0;
  data->synthOsc2.length = 256;
  data->synthOsc2.currentStep = 0;
  data->synthOsc2.phaseStep = 0;
  data->relOp.stepsPerReduction = 3;
  data->relOp2.stepsPerReduction = 3;

  data->melody1Sequencer.duration = SAMPLE_RATE / 2;
  data->melody1Sequencer.steps = MELODY_SIZE;
  data->melody1Sequencer.source = data->melodyFreq;
  data->melody1Sequencer.init();

  data->melody2Sequencer.duration = SAMPLE_RATE / 2;
  data->melody2Sequencer.steps = MELODY2_SIZE;
  data->melody2Sequencer.source = data->melody2Freq;
  data->melody2Sequencer.init();

  data->filter.lpf(500, 10, invSampleRate);
  data->filter.calculate_scaled();
}

uint8_t step(SongData *data) {
    if (data->stepCounter == 0) {
      data->synthOsc.frequency = data->melody1Sequencer.output;
      OSC_CALCULATE_PHASE_STEP(data->synthOsc, invSampleRate);
    }

    if (data->stepCounter == 1) {
      data->synthOsc2.frequency = data->melody2Sequencer.output;
      OSC_CALCULATE_PHASE_STEP(data->synthOsc2, invSampleRate);
    }

    data->stepCounter++;
    if (data->stepCounter > 100)
      data->stepCounter = 0;

    OSC_STEP(data->synthOsc2, data->triangleTable);
    OSC_STEP(data->synthOsc, data->squareTable);

    data->relOp.inputGate = data->melody1Sequencer.gate;
    data->relOp.inputValue = data->synthOsc.output;
    data->relOp.step();

    data->relOp2.inputGate = data->melody2Sequencer.gate;
    data->relOp2.inputValue = data->synthOsc2.output;
    data->relOp2.step();


    data->melody1Sequencer.step();
    data->melody2Sequencer.step();
        
    int val = (data->relOp2.outputValue >> 1) + (data->relOp.outputValue >> 1);
    return reverbStep(val);
}