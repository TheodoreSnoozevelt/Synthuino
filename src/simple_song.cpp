#include <simple_song.h>
#include <stdint.h>
#include <stdlib.h>
#include <maybe_progmem.h>

#define DELAYBITS 10
const int offset = (1 << DELAYBITS) - 2;
static unsigned char buffer[1 << DELAYBITS] = {0};
static unsigned int bufferIndex = 0;

unsigned char reverbStep(unsigned char value) {
  unsigned int tempWriteIndex = (bufferIndex + offset);
  unsigned int writeIndex1 = tempWriteIndex & ((1 << DELAYBITS) - 1); 
  unsigned char newValue = value + buffer[bufferIndex];
  buffer[writeIndex1] = scale<4>(newValue, 3);
  bufferIndex++;
  bufferIndex = bufferIndex & ((1 << DELAYBITS) - 1);
  return scale<2>(newValue, 2);
}

void setup_song(SongData *data) {
  data->stepCounter = 0;
  data->synthOsc.length = LENGTH;
  data->synthOsc.currentStep = 0;
  data->synthOsc.phaseStep = 0;
  data->synthOsc2.length = LENGTH;
  data->synthOsc2.currentStep = 0;
  data->synthOsc2.phaseStep = 0;
  data->relOp.stepsPerReduction = 12;

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

  data->melody1Sequencer.duration = SAMPLE_RATE / 4;
  data->melody1Sequencer.steps = MELODY_SIZE;
  data->melody1Sequencer.init();

  data->snareSeq.duration = SAMPLE_RATE / 4;
  data->snareSeq.steps = 8;
  data->snareSeq.source = data->snareSteps;
  data->snareSeq.init();

  data->bassSeq.duration = SAMPLE_RATE / 4;
  data->bassSeq.steps = 8;
  data->bassSeq.source = data->bassSteps;
  data->bassSeq.init();

  data->FM = 1;
}

uint8_t step(SongData *data) {
    //printf("STEP");
    if (data->stepCounter == 0) {
      data->synthOsc.frequency = data->melody1Sequencer.output; //* data->FM;
      OSC_PROGMEM_CALCULATE_PHASE_STEP(data->synthOsc, invSampleRate);
    }

    //if (data->stepCounter == 1) {
    //  data->synthOsc2.frequency = data->melody2Sequencer.output * data->FM;
    //  OSC_CALCULATE_PHASE_STEP(data->synthOsc2, invSampleRate);
    //}

    if (data->stepCounter == 2) {
      data->bassOsc.frequency = data->relBassFreq.outputValue;
      OSC_PROGMEM_CALCULATE_PHASE_STEP(data->bassOsc, invSampleRate);
    }

    data->stepCounter++;
    if (data->stepCounter > 50)
      data->stepCounter = 0;

   // OSC_STEP(data->synthOsc2, data->triangleTable);
    OSC_PROGMEM_STEP(data->synthOsc, prog_sin);
    OSC_PROGMEM_STEP(data->bassOsc, prog_sin);
    data->relOp.inputGate = data->melody1Sequencer.gate;
    data->relOp.inputValue = data->synthOsc.output;
    data->relOp.step();

    //if (data->stepCounter & 0b10) {
      OSC_PROGMEM_STEP(data->snareOsc, prog_rand);
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

    data->melody1Sequencer.step(prog_jingle_bells);
    //data->melody2Sequencer.step();
    data->snareSeq.step();
    data->bassSeq.step();

    uint8_t instr = reverbStep(data->relOp.outputValue >> 2) >> 1;
    //int instr = /*(data->relOp2.outputValue >> 2) +*/ 
    //  data->melody1Sequencer.output <= 0 
    //  ? 0
    //  : data->relOp.outputValue >> 2;
    //int val = (data->relOp2.outputValue >> 3) + (data->relOp.outputValue >> 3) + (data->relSnare.outputValue >> 3) + (data->relBass.outputValue >> 1);
    uint8_t val =  instr + (data->relSnare.outputValue >> 2) + (data->relBass.outputValue >> 1);
    return val;
}