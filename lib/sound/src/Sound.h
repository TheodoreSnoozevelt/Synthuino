#include <Arduino.h>

const unsigned long maxPhaseStep = 1L << 18;

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
    values->output = array[values->currentStep >> 10];
}

#define OSC_STEP(val, arr) val.currentStep += val.phaseStep; \
    if (val.currentStep >= maxPhaseStep) \
        val.currentStep -= maxPhaseStep; \
    val.output = arr[val.currentStep >> 10];

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

template<int T> class ReleaseEnvelope {
    private:
        int currentStep = 0;
        byte amplitude = 0;
    public:
        byte inputGate;
        byte inputValue;
        int stepsPerReduction;
        byte outputValue;

        void step() {
            if (inputGate > 0) {
                amplitude = 255;
                currentStep = 0;
                return;
            }

            if (amplitude > 0) {
                currentStep++;
                if (currentStep == stepsPerReduction) {
                    currentStep = 0;
                    amplitude--;
                }
            }

            int temp = inputValue << T;
            for (int j = 255; j > amplitude; j -= (1 << (8 - T))) {
                temp -= inputValue;
            }
            outputValue = temp >> T;
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
        byte gate = 0;

        void init() {
            output = source[0];
            gate = 1;
        }

        void step() {
            counter++;
            gate = 0;
            if (counter >= duration) {
                counter = 0;
                output = source[currentStep];
                gate = 1;
                currentStep++;
                if (currentStep >= steps)
                    currentStep = 0;
            }
        }
};

