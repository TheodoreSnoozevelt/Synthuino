#include <simple_song.h>
#include <stdint.h>
#include <stdlib.h>

#define DELAYBITS 10
const int offset = (1 << DELAYBITS) - 2;
static unsigned char buffer[1 << DELAYBITS] = {0};
static unsigned int bufferIndex = 0;

unsigned char reverbStep(unsigned char value) {
  unsigned int tempWriteIndex = (bufferIndex + offset);
  unsigned int writeIndex1 = tempWriteIndex & ((1 << DELAYBITS) - 1); 
  unsigned char newValue = value + buffer[bufferIndex];
  buffer[writeIndex1] = scale<3>(newValue, 3);
  bufferIndex++;
  bufferIndex = bufferIndex & ((1 << DELAYBITS) - 1);
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
      data->melodyFreq[i] = 440 * pow(2, note / 12.0);
    }
  }

  for (int i=0; i < MELODY2_SIZE; i++) {
    int note = data->melody2[i];
    if (note < 0) {
      data->melody2Freq[i] = 0;
    }
    else {
      data->melody2Freq[i] = 880 * pow(2, note / 12.0);
    }
  }

  for (int i=0; i<LENGTH; i++) { 
    float v = (AMP*sin((PI2/LENGTH)*i)); 
    data->sineTable[i] = int(v+OFFSET);
    data->triangleTable[i] = (i < LENGTH/2) ? 255 : 0;
    data->noiseTable[i] = (unsigned char)rand();
  }

  data->synthOsc.length = LENGTH;
  data->synthOsc.currentStep = 0;
  data->synthOsc.phaseStep = 0;
  data->synthOsc2.length = LENGTH;
  data->synthOsc2.currentStep = 0;
  data->synthOsc2.phaseStep = 0;
  data->relOp.stepsPerReduction = 6;

  data->relOp2.stepsPerReduction = 8;
  data->relSnare.stepsPerReduction = 4;
  data->relBass.stepsPerReduction = 2;
  data->relBassFreq.stepsPerReduction = 2;

  data->snareOsc.length = LENGTH;
  data->snareOsc.currentStep = 0;
  data->snareOsc.phaseStep = 1<<(PHASE_STEP_SCALE-2);

  data->bassOsc.length = LENGTH;
  data->bassOsc.currentStep = 0;
  data->bassOsc.phaseStep = 0;

  //printf("PS: %d\n", data->snareOsc.phaseStep);

  data->melody1Sequencer.duration = SAMPLE_RATE / 8;
  data->melody1Sequencer.steps = MELODY_SIZE;
  data->melody1Sequencer.source = data->melodyFreq;
  data->melody1Sequencer.init();

  data->melody2Sequencer.duration = SAMPLE_RATE / 4;
  data->melody2Sequencer.steps = MELODY2_SIZE;
  data->melody2Sequencer.source = data->melody2Freq;
  data->melody2Sequencer.init();

  data->snareSeq.duration = SAMPLE_RATE / 4;
  data->snareSeq.steps = 8;
  data->snareSeq.source = data->snareSteps;
  data->snareSeq.init();

  data->bassSeq.duration = SAMPLE_RATE / 4;
  data->bassSeq.steps = 8;
  data->bassSeq.source = data->bassSteps;
  data->bassSeq.init();
}

uint8_t step(SongData *data) {
    //printf("STEP");
    if (data->stepCounter == 0) {
      data->synthOsc.frequency = data->melody1Sequencer.output;
      OSC_CALCULATE_PHASE_STEP(data->synthOsc, invSampleRate);
    }

    if (data->stepCounter == 1) {
      data->synthOsc2.frequency = data->melody2Sequencer.output;
      OSC_CALCULATE_PHASE_STEP(data->synthOsc2, invSampleRate);
    }

    if (data->stepCounter == 2) {
      data->bassOsc.frequency = data->relBassFreq.outputValue;
      OSC_CALCULATE_PHASE_STEP(data->bassOsc, invSampleRate);
    }

    data->stepCounter++;
    if (data->stepCounter > 150)
      data->stepCounter = 0;

    OSC_STEP(data->synthOsc2, data->triangleTable);
    OSC_STEP(data->synthOsc, data->triangleTable);
    OSC_STEP(data->bassOsc, data->sineTable);
    data->relOp.inputGate = data->melody1Sequencer.gate;
    data->relOp.inputValue = data->synthOsc.output;
    data->relOp.step();

    data->relOp2.inputGate = data->melody2Sequencer.gate;
    data->relOp2.inputValue = data->synthOsc2.output;
    data->relOp2.step();

    //if (data->stepCounter & 0b10) {
      OSC_STEP(data->snareOsc, data->noiseTable);
      data->relSnare.inputGate = data->snareSeq.gate ? data->snareSeq.output > 0 : 0;
      data->relSnare.inputValue = data->snareOsc.output;
      data->relSnare.step();
    //}

    data->relBass.inputGate = data->bassSeq.gate ? data->bassSeq.output > 0 : 0;
    data->relBass.inputValue = data->bassOsc.output;
    data->relBass.step();

    data->relBassFreq.inputGate = data->bassSeq.gate ? data->bassSeq.output > 0 : 0;
    data->relBassFreq.inputValue = 255;
    data->relBassFreq.step();

    data->melody1Sequencer.step();
    data->melody2Sequencer.step();
    data->snareSeq.step();
    data->bassSeq.step();
    int instr = reverbStep((data->relOp2.outputValue >> 2) + (data->relOp.outputValue >> 2)) >> 1;
    //int val = (data->relOp2.outputValue >> 3) + (data->relOp.outputValue >> 3) + (data->relSnare.outputValue >> 3) + (data->relBass.outputValue >> 1);
    int val =  instr + (data->relSnare.outputValue >> 2) + (data->relBass.outputValue >> 1);
    return val;
}