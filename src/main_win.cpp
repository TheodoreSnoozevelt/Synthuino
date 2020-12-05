#include <Sound.h>
#include <simple_song.h>
#include <stdio.h>
#include "wav.h"


OscValues osc1;

int main() {
    float sampleRate = SAMPLE_RATE; 
    float duration = 10;     

    int nSamples = (int)(duration*sampleRate);
    Wave mySound = makeWave((int)sampleRate,1,8);
    waveSetDuration( &mySound, duration );

    SongData song;
    setup_song(&song);
    int i;
    float frameData[1];
    for(i=0; i<nSamples; i+=1 ){
        char val = step(&song);
        frameData[0] = ((float)val - 128) / 128.0;
        //printf("val: %d , sample: %f\n", val, frameData[0]);
        waveAddSample( &mySound, frameData );
    }

    // Write it to a file and clean up when done
    waveToFile( &mySound, "output.wav");
    waveDestroy( &mySound );
    return 0;
}
