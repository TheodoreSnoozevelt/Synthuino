#include <Sound.h>
#include <simple_song.h>
#include <stdio.h>
#include "wav.h"
#include <math.h>
#include <maybe_progmem.h>

OscValues osc1;

int main() {
    float sampleRate = SAMPLE_RATE; 
    float duration = 30;     

    int nSamples = (int)(duration*sampleRate);
    Wave mySound = makeWave((int)sampleRate,1,8);
    waveSetDuration( &mySound, duration );

    SongData song;
    setup_song(&song);

    FILE* f = fopen("test.c", "w");
    fprintf(f, "uint8_t PROGMEM prog_sin[] = { ");
    for (int x=0; x < 1024; x++) {
        fprintf(f, "%d", (int)(127.0 + sin(PI2 * x / 1024) * 127.0));
        if (x != 1023)
            fprintf(f, ", ");
    }
    fprintf(f, " }; \n");

    fprintf(f, "uint8_t PROGMEM prog_rand[] = { ");
    for (int x=0; x < 1024; x++) {
        fprintf(f, "%d", (int)rand() % 256);
        if (x != 1023)
            fprintf(f, ", ");
    }
    fprintf(f, " };\n");

    fprintf(f, "float PROGMEM prog_jingle_bells[] = { ");
    for (int x=0; x < MELODY_SIZE; x++) {
        int note = PROGMEM_GET_BYTE(melody, x);
        float val = note < 0 ? 0 : (pow(2, note / 12.0) * 660);
        fprintf(f, "%f", val);
        if (x != 1023)
            fprintf(f, ", ");
    }
    fprintf(f, " };");
    fclose(f);

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
