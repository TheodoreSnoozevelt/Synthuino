#ifndef SIMPLE_SONG
#define SIMPLE_SONG
#include <Sound.h>
#include <stdint.h>
#include <stdint.h>


#define LENGTH (1 << OSC_SIZE)
#define AMP 127
#define OFFSET 128
#define PI2 6.283185
#define CLOCK_DIVIDER 4
#define SAMPLE_RATE (16000000 / (256 * (CLOCK_DIVIDER + 1)))

const float invSampleRate = 1.0 / SAMPLE_RATE;

#define MELODY_SIZE 11
#define MELODY2_SIZE 5

struct SongData {
    int stepCounter;
    uint8_t sineTable[LENGTH];
    uint8_t triangleTable[LENGTH];
    uint8_t noiseTable[LENGTH];

    OscValues synthOsc;
    OscValues synthOsc2;
    OscValues snareOsc;
    OscValues bassOsc;

    ReleaseEnvelope<3> relOp;
    ReleaseEnvelope<3> relOp2;
    ReleaseEnvelope<3> relSnare;
    ReleaseEnvelope<3> relBass;
    ReleaseEnvelope<3> relBassFreq;

    StepSequencer<float> melody1Sequencer;
    StepSequencer<float> melody2Sequencer;
    
    StepSequencer<uint8_t> snareSeq;
    StepSequencer<uint8_t> bassSeq;

    int8_t melody[MELODY_SIZE] = { 
        0, 5, 4, 1, 8, 7, 10, 5, 8, 4, 5
    };
    float melodyFreq[MELODY_SIZE];
    float melody2Freq[MELODY2_SIZE];
    int8_t melody2[MELODY2_SIZE] = { 
        0, 1, 0, 5, 4
    };

    uint8_t snareSteps[8] = {
        0, 0, 0, 0, 255, 0, 0, 255
    };

    uint8_t bassSteps[8] = {
        255, 0, 0, 255, 0, 0, 255, 0
    };
};


#define BPM 240
#define BPS 4

void setup_song(SongData*);
uint8_t step(SongData*);
#endif