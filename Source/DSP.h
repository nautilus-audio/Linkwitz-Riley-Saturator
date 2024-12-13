
typedef struct {
    float a0, a1, a2, b1, b2;
} LRCoefficients;


struct Filter{
    
    LRCoefficients hpfCoeffs;
    LRCoefficients lpfCoeffs;
    
    
    void hpfLRCoeffs(float f_crossover, float fs)
    {
        float theta = 2 * M_PI * f_crossover / fs;
        float Wc = M_PI * f_crossover;
        float k = Wc / tan(theta);
        float d = pow(k, 2.0) + pow(Wc, 2.0) + 2.0 * k * Wc;
        
        hpfCoeffs.a0 = pow(Wc, 2.0) / d;
        hpfCoeffs.a1 = -2.0 * pow(Wc, 2.0) / d;
        hpfCoeffs.a2 = hpfCoeffs.a0;
        hpfCoeffs.b1 = (-2.0 * pow(k, 2.0) + 2.0 * pow(Wc, 2.0)) / d;
        hpfCoeffs.b2 = (-2.0 * k * Wc + pow(k, 2.0) + pow(Wc, 2.0)) / d;
    }

    void lpfLRCoeffs(float f_crossover, float fs)
    {
        float theta = 2 * M_PI * f_crossover / fs;
        float Wc = M_PI * f_crossover;
        float k = Wc / tan(theta);
        float d = pow(k, 2.0) + pow(Wc, 2.0) + 2.0 * k * Wc;
        
        lpfCoeffs.a0 = pow(Wc, 2.0) / d;
        lpfCoeffs.a1 = 2.0 * pow(Wc, 2.0) / d;
        lpfCoeffs.a2 = lpfCoeffs.a0;
        lpfCoeffs.b1 = (-2.0 * pow(k, 2.0) + 2.0 * pow(Wc, 2.0)) / d;
        lpfCoeffs.b2 = (-2.0 * k * Wc + pow(k, 2.0) + pow(Wc, 2.0)) / d;
    }

    float lowpass_filter(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2) {
        float output = a0 * input + a1 * (*state1) + a2 * (*state2);
        *state2 = *state1;
        *state1 = input - b1 * (*state1) - b2 * (*state2);
        return output;
    }

    float highpass_filter(float input, float *state1, float *state2, float a0, float a1, float a2, float b1, float b2) {
        
        float output = a0 * input + a1 * (*state1) + a2 * (*state2);
        *state2 = *state1;
        *state1 = input - b1 * (*state1) - b2 * (*state2);
        return output * (-1);
    }
};


struct DSP
{
    
    float* high_states_1;
    float* high_states_2;
    float* low_states_1;
    float* low_states_2;
    float* outputSamples;
    float* low_outputs;
    float* high_outputs;
    float* dist_lows;
    
    
    float f_crossover = 0.f; // Crossover frequency
    float initValue = 0.f;
    Filter* filters;

    float** inBuffer;
    float** outBuffer;
    float** highBand;
    float** lowBand;

    int _nMaxChannels;
    int _nMaxBlockSize;
    float _fGain_01;
    float fs;

    
    void Init() {
        inBuffer =  NULL;
        outBuffer =  NULL;
        highBand =  NULL;
        lowBand =  NULL;
        _nMaxChannels = 1;
        _nMaxBlockSize = 1;
        _fGain_01 = 1;
        
        high_states_1 =  NULL;
        high_states_2 =  NULL;
        low_states_1 =  NULL;
        low_states_2 =  NULL;
        outputSamples =  NULL;
        low_outputs =  NULL;
        high_outputs =  NULL;
        dist_lows =  NULL;
        
    } //RRS: All initializations needed for your DSP, memory allocations are allowed inside

    //RRS: Memory allocations are allowed inside
    void SetMaxBlockSize(int a_nMaxBlockSize)
    {
        if (_nMaxBlockSize != a_nMaxBlockSize)
        {
            _nMaxBlockSize = a_nMaxBlockSize;

            _ReAllocInternalBuffers(_nMaxChannels);
        }
    }
    
    void SetCrossoverFrequency(float a_nCrossoverFreq)
    {
        f_crossover = a_nCrossoverFreq;
    }
    
    void SetMaxChannels(int a_nMaxChannels)
    {
        if (_nMaxChannels != a_nMaxChannels)
            _ReAllocInternalBuffers(a_nMaxChannels);

        filters = (Filter*) malloc(a_nMaxChannels * sizeof(Filter));

        high_states_1 = (float*) malloc(_nMaxChannels);
        high_states_2 = (float*) malloc(_nMaxChannels);
        low_states_1 = (float *) malloc(_nMaxChannels);
        low_states_2 = (float *) malloc(_nMaxChannels);
        outputSamples = (float *) malloc(_nMaxChannels);
        low_outputs = (float *) malloc(_nMaxChannels);
        high_outputs = (float *) malloc(_nMaxChannels);
        dist_lows = (float *) malloc(_nMaxChannels);
    }

    void SetSampleRate(float a_fSampleRate_Hz) {
        fs = a_fSampleRate_Hz;
        
        for(int channel = 0; channel < _nMaxChannels; channel++)
        {
            filters[channel].hpfLRCoeffs(f_crossover, fs);
            filters[channel].lpfLRCoeffs(f_crossover, fs);
        }
    }

    void SetGain(float a_fGain_01) { _fGain_01 = a_fGain_01; }
    void SetSomeParam1(float a_fSomeParam1Value) {}
    void SetSomeParam2(float a_fSomeParam2Value) {}
        
    void Release() { _ReleaseInternalBuffers();
    }
    
    float ProcessSampleLow(float* readData, Filter channelFilter, int channel, int index){
        auto sample = channelFilter.lowpass_filter(readData[index], &low_states_1[channel], &low_states_2[channel], channelFilter.lpfCoeffs.a0, channelFilter.lpfCoeffs.a1, channelFilter.lpfCoeffs.a2, channelFilter.lpfCoeffs.b1, channelFilter.lpfCoeffs.b2);
        
        return sample;
    }

    float ProcessSampleHigh(float* readData, Filter channelFilter, int channel, int index){
        auto sample = channelFilter.highpass_filter(readData[index], &high_states_1[channel], &high_states_2[channel], channelFilter.hpfCoeffs.a0, channelFilter.hpfCoeffs.a1, channelFilter.hpfCoeffs.a2, channelFilter.hpfCoeffs.b1, channelFilter.hpfCoeffs.b2);
        
        return sample;
    }
    
    float* ProcessHighBand(float* inStream, float** band, Filter channelFilter, int a_nChannels, int a_nSampleCount, int channel)
    {
        for (int i = 0; i < a_nSampleCount; ++i)
        {
            high_outputs[channel] = channelFilter.highpass_filter(inStream[i], &high_states_1[channel], &high_states_2[channel], channelFilter.hpfCoeffs.a0, channelFilter.hpfCoeffs.a1, channelFilter.hpfCoeffs.a2, channelFilter.hpfCoeffs.b1, channelFilter.hpfCoeffs.b2);
            
            band[channel][i] = high_outputs[channel];
            
        }
        return band[channel];
    }
    
    float* ProcessLowBand(float* inStream, float** band, Filter channelFilter, int a_nChannels, int a_nSampleCount, int channel)
    {
        for (int i = 0; i < a_nSampleCount; ++i)
        {
            low_outputs[channel] = channelFilter.lowpass_filter(inStream[i], &low_states_1[channel], &low_states_2[channel], channelFilter.lpfCoeffs.a0, channelFilter.lpfCoeffs.a1, channelFilter.lpfCoeffs.a2, channelFilter.lpfCoeffs.b1, channelFilter.lpfCoeffs.b2);
            
            band[channel][i] = low_outputs[channel];
        }
        
        return band[channel];
    }
        
        

    void Process(float** a_vAudioBlocksInPlace, int a_nChannels, int a_nSampleCount)
    {
        for (int channel = 0; channel < a_nChannels; ++channel)
        {
            memcpy(inBuffer[channel], a_vAudioBlocksInPlace[channel], a_nSampleCount * sizeof(float));
            
            float* readData = inBuffer[channel];
            float* writeData = outBuffer[channel];
            auto monoFilter = filters[channel];
            
            // Process audio samples
            highBand[channel] = ProcessHighBand(readData, highBand, monoFilter, a_nChannels, a_nSampleCount, channel);
            lowBand[channel] = ProcessLowBand(readData, lowBand, monoFilter, a_nChannels, a_nSampleCount, channel);

            for (int i = 0; i < a_nSampleCount; ++i)
            {
                dist_lows[channel] = tubeSaturation(lowBand[channel][i], 1.f); // Apply Saturation
                writeData[i] = (highBand[channel][i] + dist_lows[channel]) * .707f;  // Sum Signals
            }
                        
            memcpy(a_vAudioBlocksInPlace[channel], outBuffer[channel], a_nSampleCount * sizeof(float));
        }
    }

    void _ReleaseInternalBuffers()
    {
        if (inBuffer)
        {
            for (int n = 0; n < _nMaxChannels; ++n)
            {
                delete[] inBuffer[n];
            }
            
            delete[] inBuffer;
        }
        
        if (outBuffer)
        {
            for (int n = 0; n < _nMaxChannels; ++n)
            {
                delete[] outBuffer[n];
            }

            delete[] outBuffer;
        }

        inBuffer =  NULL;
        outBuffer =  NULL;
        
        if (highBand)
        {
            for (int n = 0; n < _nMaxChannels; ++n)
            {
                delete[] highBand[n];
            }
            
            delete[] highBand;
        }
        
        if (lowBand)
        {
            for (int n = 0; n < _nMaxChannels; ++n)
            {
                delete[] lowBand[n];
            }

            delete[] lowBand;
        }

        highBand =  NULL;
        lowBand =  NULL;
    }

    void _ReAllocInternalBuffers(int a_nNewMaxChannels)
    {
        _ReleaseInternalBuffers();
        
        inBuffer = new float*[_nMaxChannels = a_nNewMaxChannels];
        outBuffer = new float*[_nMaxChannels = a_nNewMaxChannels];
        highBand = new float*[_nMaxChannels = a_nNewMaxChannels];
        lowBand = new float*[_nMaxChannels = a_nNewMaxChannels];
        
        for (int n = 0; n < _nMaxChannels; ++n)
        {
            inBuffer[n] = new float[_nMaxBlockSize];
            outBuffer[n] = new float[_nMaxBlockSize];
            highBand[n] = new float[_nMaxBlockSize];
            lowBand[n] = new float[_nMaxBlockSize];
        }
    }
        
    float tubeSaturation(float x, float mixAmount)
    {
        float a = mixAmount;
        float y = 0.f;

        // Soft clipping based on quadratic function
        float threshold1 = 1.0f/3.0f;
        float threshold2 = 2.0f/3.0f;
        
        if(a == 0.0f)
            y = x;
        else if(x > threshold2)
            y = 1.0f;
        else if(x > threshold1)
            y = (3.0f - ((2.0f - 3.0f*x)) *  ((2.0f - 3.0f*x)))/3.0f;
        else if(x < -threshold2)
            y = -1.0f;
        else if(x < -threshold1)
            y = -(3.0f - ((2.0f + 3.0f*x)) * ((2.0f + 3.0f*x)))/3.0f;
        else
            y = (2.0f* x);
        
        return y;
    }
};

