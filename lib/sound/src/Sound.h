

template<int T> uint8_t fastDiv(uint8_t inputValue, uint8_t amplitude) {
    if (inputValue == 0)
        return 0;

    if (amplitude == 0)
        return 0;

    int temp = inputValue << T;
    for (int j = 255; j > amplitude; j -= (1 << (8 - T))) {
        temp -= inputValue;
    }
    return temp >> T;
}

const unsigned long maxPhaseStep = 1L << 18;

struct OscValues {
    unsigned short length;
    unsigned long currentStep;
    unsigned long phaseStep;
    float frequency;
    uint8_t output;
};

inline void osc_step(OscValues *values, uint8_t *array) {
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

inline int8_t toSigned(uint8_t x) {
    return x - 127;
}

inline uint8_t fromSigned(int8_t x) {
    return x + 127;
}

template<int acc> inline int8_t scaleSigned(int8_t x, int8_t scale) {
    uint8_t val = abs(x);
    uint8_t scaled = fastDiv<acc>(val, scale);
    return signbit(x) ? -scaled : scaled;
}

class Biquad {
    private:
        float xn1;
        float xn2;
        float yn1;
        float yn2;

    public:
        float b0;
        float b1;
        float b2;
        float a0;
        float a1;
        float a2;

        float b0_scaled;
        float b1_scaled;
        float b2_scaled;
        float a1_scaled;
        float a2_scaled;

        float input;
        float output;

        void calculate_scaled() {
            b0_scaled = b0 / a0;
            b1_scaled = b1 / a0;
            b2_scaled = b2 / a0;
            a1_scaled = a1 / a0;
            a2_scaled = a2 / a0;
/*
            Serial.println(b0_scaled);
            Serial.println(b1_scaled);
            Serial.println(b2_scaled);
            Serial.println(a1_scaled);
            Serial.println(a2_scaled);
*/
        }

        void lpf(float freq, float q, float invSampleRate) {
            float omega = 2.0 * 3.14159 * freq * invSampleRate;
            float alpha = sin(omega) * 2 / q;
            b0 = (1.0 - cos(omega)) / 2.0;
            b1 = 1 - cos(omega);
            b2 = b0;
            a0 = 1 + alpha;
            a1 = -1 * cos(omega);
            a2 = 1 - alpha;
/*
            Serial.println(b0);
            Serial.println(b1);
            Serial.println(b2);
            Serial.println(a1);
            Serial.println(a2);  
*/          
        }

        void step() {
            float result = 
                b0_scaled * input 
                + b1_scaled * xn1 
                + b2_scaled * xn2
                - a1_scaled * yn1
                - a2_scaled * yn2;
            
            xn2 = xn1;
            xn1 = input;
            yn2 = yn1;
            yn1 = result;
            output = result;
        }
};

class Oscillator {
    private:
        float currentStep = 0;
        float phaseStep = 0;

    public:
        uint8_t *source;
        uint16_t length;
        float frequency;
        uint8_t output;

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

template<int T> class DelayLine {
    private:
        uint8_t values[T] = {0};
        int index = 0;

    public:   
        void insert(uint8_t value) {
            values[index++] = value;
            if (index == T)
                index = 0;
        }

        uint8_t readDelayed() {
            int readIndex = index;
            readIndex = readIndex == 0 ? T : readIndex;
            readIndex -= 1;
            return values[readIndex];
        }
};

template<int T> class Echo {
    private:
        DelayLine<T> delayLine;

    public:
        uint8_t inputValue;
        uint8_t outputValue;

        void step() {
            uint8_t lineValue = delayLine.readDelayed();
            int temp = inputValue + fastDiv<4>(lineValue, 100);
            if (temp > 255)
                outputValue = 255;
            else
                outputValue = temp;
            delayLine.insert(outputValue);
        }
};

class Reverb {
    private:
        Echo<10> echo1;
        Echo<20> echo2;
        Echo<30> echo3;
        Echo<40> echo4;
    
    public:
        uint8_t input;
        uint8_t output;

        void step() {
            echo1.inputValue = input;
            echo1.step();
            echo2.inputValue = echo1.outputValue;
            echo2.step();
            echo3.inputValue = echo2.outputValue;
            echo3.step();
            echo4.inputValue = echo3.outputValue;
            echo4.step();
            output = echo4.outputValue; 
        }
};

template<int T> class ReleaseEnvelope {
    private:
        int currentStep = 0;
        uint8_t amplitude = 0;
    public:
        uint8_t inputGate;
        uint8_t inputValue;
        int stepsPerReduction;
        uint8_t outputValue;

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

            outputValue = fastDiv<T>(inputValue, amplitude + 1);

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
        uint8_t gate = 0;

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

