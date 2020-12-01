#include <Arduino.h>

const unsigned long maxPhaseStep = 1L << 31;

struct OscValues {
    unsigned short length;
    unsigned long currentStep;
    unsigned long phaseStep;
    float frequency;
    byte output;
};

inline void osc_step(OscValues *values, byte *array) {
    values->currentStep += values->phaseStep;
    if (values->currentStep >= maxPhaseStep)
        values->currentStep -= maxPhaseStep;
    values->output = array[values->currentStep >> 24];
}

#define OSC_STEP(val, arr) val.currentStep += val.phaseStep; \
    if (val.currentStep >= maxPhaseStep) \
        val.currentStep -= maxPhaseStep; \
    val.output = arr[val.currentStep >> 24];

#define OSC_CALCULATE_PHASE_STEP(val, inv) val.phaseStep = inv * val.frequency * maxPhaseStep;

inline void osc_calculate_phase_step(OscValues *values, float invSampleRate) {
    values->phaseStep = invSampleRate * values->frequency * maxPhaseStep;
}

class Oscillator {
    private:
        float currentStep = 0;
        float phaseStep = 0;

    public:
        byte *source;
        uint16_t length;
        float frequency;
        byte output;

        inline void calculatePhaseStep(float invSampleRate) {
            phaseStep = length * invSampleRate * frequency;
        }

        inline void step() {
            currentStep += phaseStep;
            if (currentStep >= length)
                currentStep -= length;

            output = source[(int)currentStep];
        }
};

template<class T> class StepSequencer {
    private:
        int counter = 0;
        int currentStep = 0;
    public:
        T *source;
        T output;
        int duration;
        int steps;

        void step() {
            counter++;
            if (counter >= duration) {
                counter = 0;
                output = source[currentStep];
                currentStep++;
                if (currentStep >= steps)
                    currentStep = 0;
            }
        }
};

