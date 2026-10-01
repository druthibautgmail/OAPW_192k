#ifndef RACE_DSP_ENGINE_H
#define RACE_DSP_ENGINE_H

#include <vector>
#include <mutex>
#include <array>
#include <complex>

// 64-Bit Float für interne Berechnungen
using audio_sample_t = double;

class BiquadFilter {
private:
    audio_sample_t b0, b1, b2, a1, a2;
    audio_sample_t z1, z2; 
    audio_sample_t freq, q, gainDb;
    int sampleRate;

public:
    BiquadFilter();
    void setParameters(audio_sample_t frequency, audio_sample_t qFactor, audio_sample_t gainDb, int sampleRate = 192000);
    audio_sample_t process(audio_sample_t in);
    
    audio_sample_t getFreq() const { return freq; }
    audio_sample_t getQ() const { return q; }
    audio_sample_t getGain() const { return gainDb; }
};

class IIRFilter {
private:
    std::vector<double> ac;
    std::vector<double> bc;
    std::vector<double> x;
    std::vector<double> y;
    int order;

public:
    IIRFilter(const std::vector<double>& a, const std::vector<double>& b);
    double process(double input);
};

class RACEDspEngine {
private:
    audio_sample_t delayUs;           
    audio_sample_t attenuationDb;     
    audio_sample_t attenuationLinear; 
    
    // Fractional Delay State
    size_t intDelay;
    double fraction;
    size_t bufferSize;
    
    audio_sample_t centerP;
    bool freqLimitRACE;
    audio_sample_t volume; 
    bool raceEnabled;
    bool filtersEnabled;
    bool eqEnabled; 

    std::vector<float> spectrumBuffer;
    int spectrumIndex;

    std::vector<audio_sample_t> delayBufferL;
    std::vector<audio_sample_t> delayBufferR;
    size_t writeIndex;

    IIRFilter lpfL, lpfR, hpfL, hpfR;

    std::array<BiquadFilter, 3> eqL;
    std::array<BiquadFilter, 3> eqR;

    std::mutex dspMutex;

    // Hermite Interpolation Funktion
    inline audio_sample_t hermite(double frac, audio_sample_t y0, audio_sample_t y1, audio_sample_t y2, audio_sample_t y3);

public:
    RACEDspEngine(audio_sample_t initialDelayUs, audio_sample_t initialAttenuationDb, audio_sample_t initialCenterP, bool initialFreqLimit);
    ~RACEDspEngine();

    void setParameters(audio_sample_t newDelayUs, audio_sample_t newAttenuationDb, audio_sample_t newCenterP, bool newFreqLimit);
    void setVolume(audio_sample_t newVolume); 
    void setRaceEnabled(bool enabled);
    void setFiltersEnabled(bool enabled);
    
    void setEqEnabled(bool enabled);
    void setEqBand(int bandIndex, audio_sample_t freq, audio_sample_t q, audio_sample_t gainDb);

    // Getter Signaturen bleiben unverändert für die Web-API
    float getDelayUs();
    float getAttenuationDb();
    float getCenterP();
    bool getFreqLimit();
    float getVolume();
    bool getRaceEnabled();
    bool getFiltersEnabled();
    bool getEqEnabled();
    float getEqFreq(int bandIndex);
    float getEqQ(int bandIndex);
    float getEqGain(int bandIndex);
    std::vector<float> getSpectrumBands();

    // Nimmt nun Double-Vektoren für 64-Bit I/O
    void processSamples(std::vector<audio_sample_t>& interleavedSamples);
};

#endif
