#ifndef SIMPLE_SONG
#define SIMPLE_SONG
#include <Sound.h>
#include <stdint.h>
#include <stdint.h>

#define LENGTH 256
#define AMP 127
#define OFFSET 128
#define PI2 6.283185
#define SAMPLE_RATE 15625

const float inv255 = 1 / 255.0;
const float inv127 = 1 / 127.0;
const float inv2048 = 1 / 2048.0;
const float invSampleRate = 1.0 / SAMPLE_RATE;

#define MELODY_SIZE 11
#define MELODY2_SIZE 5

struct SongData {
    int stepCounter;
    uint8_t sineTable[LENGTH];
    uint8_t squareTable[LENGTH];
    uint8_t triangleTable[LENGTH];

    OscValues synthOsc;
    OscValues synthOsc2;

    ReleaseEnvelope<3> relOp;
    ReleaseEnvelope<3> relOp2;

    Reverb echo;

    Biquad filter;

    StepSequencer<float> melody1Sequencer;
    StepSequencer<float> melody2Sequencer;
    int8_t melody[MELODY_SIZE] = { 
        0, 5, 4, 1, 8, 7, 10, 5, 8, 4, 5
    };
    float melodyFreq[MELODY_SIZE];
    float melody2Freq[MELODY2_SIZE];
    int8_t melody2[MELODY2_SIZE] = { 
        0, 1, 0, 5, 4
    };
};


#define BPM 240
#define BPS 4

void setup_song(SongData*);
uint8_t step(SongData*);
#endif