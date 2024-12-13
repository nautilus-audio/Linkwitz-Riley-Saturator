
typedef float TAudioSampleType;
typedef TAudioSampleType TFloatParamType;
typedef int TIntegerParamType;

typedef struct {
    TFloatParamType a0, a1, a2, b1, b2;
} LRCoefficients;


struct Filter{
    
    LRCoefficients hpfCoeffs;
    LRCoefficients lpfCoeffs;
    
    
    void hpfLRCoeffs(TFloatParamType f_crossover, TFloatParamType fs)
    {
        TFloatParamType theta = 2 * M_PI * f_crossover / fs;
        TFloatParamType Wc = M_PI * f_crossover;
        TFloatParamType k = Wc / tan(theta);
        TFloatParamType d = pow(k, 2.0) + pow(Wc, 2.0) + 2.0 * k * Wc;
        
        hpfCoeffs.a0 = pow(Wc, 2.0) / d;
        hpfCoeffs.a1 = -2.0 * pow(Wc, 2.0) / d;
        hpfCoeffs.a2 = hpfCoeffs.a0;
        hpfCoeffs.b1 = (-2.0 * pow(k, 2.0) + 2.0 * pow(Wc, 2.0)) / d;
        hpfCoeffs.b2 = (-2.0 * k * Wc + pow(k, 2.0) + pow(Wc, 2.0)) / d;
    }

    void lpfLRCoeffs(TFloatParamType f_crossover, TFloatParamType fs)
    {
        TFloatParamType theta = 2 * M_PI * f_crossover / fs;
        TFloatParamType Wc = M_PI * f_crossover;
        TFloatParamType k = Wc / tan(theta);
        TFloatParamType d = pow(k, 2.0) + pow(Wc, 2.0) + 2.0 * k * Wc;
        
        lpfCoeffs.a0 = pow(Wc, 2.0) / d;
        lpfCoeffs.a1 = 2.0 * pow(Wc, 2.0) / d;
        lpfCoeffs.a2 = lpfCoeffs.a0;
        lpfCoeffs.b1 = (-2.0 * pow(k, 2.0) + 2.0 * pow(Wc, 2.0)) / d;
        lpfCoeffs.b2 = (-2.0 * k * Wc + pow(k, 2.0) + pow(Wc, 2.0)) / d;
    }

    TFloatParamType lowpass_filter(TFloatParamType input, TFloatParamType *state1, TFloatParamType *state2, TFloatParamType a0, TFloatParamType a1, TFloatParamType a2, TFloatParamType b1, TFloatParamType b2) {
        TFloatParamType output = a0 * input + a1 * (*state1) + a2 * (*state2);
        *state2 = *state1;
        *state1 = input - b1 * (*state1) - b2 * (*state2);
        return output;
    }

    TFloatParamType highpass_filter(TFloatParamType input, TFloatParamType *state1, TFloatParamType *state2, TFloatParamType a0, TFloatParamType a1, TFloatParamType a2, TFloatParamType b1, TFloatParamType b2) {
        
        TFloatParamType output = a0 * input + a1 * (*state1) + a2 * (*state2);
        *state2 = *state1;
        *state1 = input - b1 * (*state1) - b2 * (*state2);
        return output * (-1);
    }
};


struct DSP
{
    
    TFloatParamType* high_states_1;
    TFloatParamType* high_states_2;
    TFloatParamType* low_states_1;
    TFloatParamType* low_states_2;
    TFloatParamType* outputSamples;
    TFloatParamType* low_outputs;
    TFloatParamType* high_outputs;
    TFloatParamType* dist_lows;
    
    
    TFloatParamType f_crossover = 0.f; // Crossover frequency
    TFloatParamType initValue = 0.f;
    Filter* filters;

    TAudioSampleType** inBuffer;
    TAudioSampleType** outBuffer;
    TAudioSampleType** highBand;
    TAudioSampleType** lowBand;

    TIntegerParamType _nMaxChannels;
    TIntegerParamType _nMaxBlockSize;
    TFloatParamType _fGain_01;
    TFloatParamType fs;

    
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
    void SetMaxBlockSize(TIntegerParamType a_nMaxBlockSize)
    {
        if (_nMaxBlockSize != a_nMaxBlockSize)
        {
            _nMaxBlockSize = a_nMaxBlockSize;

            _ReAllocInternalBuffers(_nMaxChannels);
        }
    }
    
    void SetCrossoverFrequency(TFloatParamType a_nCrossoverFreq)
    {
        f_crossover = a_nCrossoverFreq;
    }
    
    //RRS: Memory allocations are allowed inside
    void SetMaxChannels(TIntegerParamType a_nMaxChannels)
    {
        if (_nMaxChannels != a_nMaxChannels)
            _ReAllocInternalBuffers(a_nMaxChannels);

        filters = (Filter*) malloc(a_nMaxChannels * sizeof(Filter));

        high_states_1 = (TFloatParamType*) malloc(_nMaxChannels);
        high_states_2 = (TFloatParamType*) malloc(_nMaxChannels);
        low_states_1 = (TFloatParamType *) malloc(_nMaxChannels);
        low_states_2 = (TFloatParamType *) malloc(_nMaxChannels);
        outputSamples = (TFloatParamType *) malloc(_nMaxChannels);
        low_outputs = (TFloatParamType *) malloc(_nMaxChannels);
        high_outputs = (TFloatParamType *) malloc(_nMaxChannels);
        dist_lows = (TFloatParamType *) malloc(_nMaxChannels);
    }

    //RRS: Sample rate is not constant, so you have to reinitialize your sample rate dependent params (such as filters coeffs) on this call from our framework
    void SetSampleRate(TFloatParamType a_fSampleRate_Hz) {
        fs = a_fSampleRate_Hz;
        
        for(TIntegerParamType channel = 0; channel < _nMaxChannels; channel++)
        {
            filters[channel].hpfLRCoeffs(f_crossover, fs);
            filters[channel].lpfLRCoeffs(f_crossover, fs);
        }
    } //RRS: Memory allocations are allowed inside

    void SetGain(TFloatParamType a_fGain_01) { _fGain_01 = a_fGain_01; } //RRS: Assertion: No memory allocations are allowed inside!
    void SetSomeParam1(TFloatParamType a_fSomeParam1Value) {} //RRS: Assertion: No memory allocations are allowed inside!
    void SetSomeParam2(TFloatParamType a_fSomeParam2Value) {} //RRS: Assertion: No memory allocations are allowed inside!
        
    void Release() { _ReleaseInternalBuffers();
        //RRS: All previously allocated memory can be deallocated here
    }
    
    float ProcessSampleLow(TFloatParamType* readData, Filter channelFilter, TIntegerParamType channel, TIntegerParamType index){
        auto sample = channelFilter.lowpass_filter(readData[index], &low_states_1[channel], &low_states_2[channel], channelFilter.lpfCoeffs.a0, channelFilter.lpfCoeffs.a1, channelFilter.lpfCoeffs.a2, channelFilter.lpfCoeffs.b1, channelFilter.lpfCoeffs.b2);
        
        return sample;
    }

    float ProcessSampleHigh(TFloatParamType* readData, Filter channelFilter, TIntegerParamType channel, TIntegerParamType index){
        auto sample = channelFilter.highpass_filter(readData[index], &high_states_1[channel], &high_states_2[channel], channelFilter.hpfCoeffs.a0, channelFilter.hpfCoeffs.a1, channelFilter.hpfCoeffs.a2, channelFilter.hpfCoeffs.b1, channelFilter.hpfCoeffs.b2);
        
        return sample;
    }
    
    TAudioSampleType* ProcessHighBand(TAudioSampleType* inStream, TAudioSampleType** band, Filter channelFilter, TIntegerParamType a_nChannels, TIntegerParamType a_nSampleCount, TIntegerParamType channel)
    {
        for (TIntegerParamType i = 0; i < a_nSampleCount; ++i)
        {
            high_outputs[channel] = channelFilter.highpass_filter(inStream[i], &high_states_1[channel], &high_states_2[channel], channelFilter.hpfCoeffs.a0, channelFilter.hpfCoeffs.a1, channelFilter.hpfCoeffs.a2, channelFilter.hpfCoeffs.b1, channelFilter.hpfCoeffs.b2);
            
            band[channel][i] = high_outputs[channel];
            
        }
        return band[channel];
    }
    
    TAudioSampleType* ProcessLowBand(TAudioSampleType* inStream, TAudioSampleType** band, Filter channelFilter, TIntegerParamType a_nChannels, TIntegerParamType a_nSampleCount, TIntegerParamType channel)
    {
        for (TIntegerParamType i = 0; i < a_nSampleCount; ++i)
        {
            low_outputs[channel] = channelFilter.lowpass_filter(inStream[i], &low_states_1[channel], &low_states_2[channel], channelFilter.lpfCoeffs.a0, channelFilter.lpfCoeffs.a1, channelFilter.lpfCoeffs.a2, channelFilter.lpfCoeffs.b1, channelFilter.lpfCoeffs.b2);
            
            band[channel][i] = low_outputs[channel];
        }
        
        return band[channel];
    }
        
        
    //RRS: Assertion: No memory allocations are allowed inside!
    void Process(TAudioSampleType** a_vAudioBlocksInPlace, TIntegerParamType a_nChannels, TIntegerParamType a_nSampleCount)
    {
        //RRS: Assertion: a_nChannels less or equal _nMaxChannels set in SetMaxChannels()
        //RRS: Assertion: a_nSampleCount less or equal _nMaxBlockSize set in SetMaxBlockSize()
        
        //RRS: Here we copy the input audio data into internal buffers, apply Gain and then copy the processed data back to the input buffers (in-place).
        //RRS: There is no need for this copy operation (and these internal buffers): it's just a demonstration of how to work with the internal buffers if you need them.

        
        for (TIntegerParamType channel = 0; channel < a_nChannels; ++channel)
        {
            memcpy(inBuffer[channel], a_vAudioBlocksInPlace[channel], a_nSampleCount * sizeof(TAudioSampleType));
            
            TFloatParamType* readData = inBuffer[channel];
            TFloatParamType* writeData = outBuffer[channel];
            auto monoFilter = filters[channel];
            
            // Process audio samples
            highBand[channel] = ProcessHighBand(readData, highBand, monoFilter, a_nChannels, a_nSampleCount, channel);
            lowBand[channel] = ProcessLowBand(readData, lowBand, monoFilter, a_nChannels, a_nSampleCount, channel);

            for (TIntegerParamType i = 0; i < a_nSampleCount; ++i)
            {
                dist_lows[channel] = tubeSaturation(lowBand[channel][i], 1.f); // Apply Saturation
                writeData[i] = (highBand[channel][i] + dist_lows[channel]) * .707f;  // Sum Signals
            }
                        
            memcpy(a_vAudioBlocksInPlace[channel], outBuffer[channel], a_nSampleCount * sizeof(TAudioSampleType));
        }
    }

    void _ReleaseInternalBuffers()
    {
        if (inBuffer)
        {
            for (TIntegerParamType n = 0; n < _nMaxChannels; ++n)
            {
                delete[] inBuffer[n];
            }
            
            delete[] inBuffer;
        }
        
        if (outBuffer)
        {
            for (TIntegerParamType n = 0; n < _nMaxChannels; ++n)
            {
                delete[] outBuffer[n];
            }

            delete[] outBuffer;
        }

        inBuffer =  NULL;
        outBuffer =  NULL;
        
        if (highBand)
        {
            for (TIntegerParamType n = 0; n < _nMaxChannels; ++n)
            {
                delete[] highBand[n];
            }
            
            delete[] highBand;
        }
        
        if (lowBand)
        {
            for (TIntegerParamType n = 0; n < _nMaxChannels; ++n)
            {
                delete[] lowBand[n];
            }

            delete[] lowBand;
        }

        highBand =  NULL;
        lowBand =  NULL;
    }

    void _ReAllocInternalBuffers(TIntegerParamType a_nNewMaxChannels)
    {
        _ReleaseInternalBuffers();
        
        inBuffer = new TAudioSampleType*[_nMaxChannels = a_nNewMaxChannels];
        outBuffer = new TAudioSampleType*[_nMaxChannels = a_nNewMaxChannels];
        highBand = new TAudioSampleType*[_nMaxChannels = a_nNewMaxChannels];
        lowBand = new TAudioSampleType*[_nMaxChannels = a_nNewMaxChannels];
        
        for (TIntegerParamType n = 0; n < _nMaxChannels; ++n)
        {
            inBuffer[n] = new TAudioSampleType[_nMaxBlockSize];
            outBuffer[n] = new TAudioSampleType[_nMaxBlockSize];
            highBand[n] = new TAudioSampleType[_nMaxBlockSize];
            lowBand[n] = new TAudioSampleType[_nMaxBlockSize];
        }
    }
        
    TFloatParamType tubeSaturation(TFloatParamType x, TFloatParamType mixAmount)
    {
        TFloatParamType a = mixAmount;
        TFloatParamType y = 0.f;

        // Soft clipping based on quadratic function
        TFloatParamType threshold1 = 1.0f/3.0f;
        TFloatParamType threshold2 = 2.0f/3.0f;
        
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

