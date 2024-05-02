/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class Comp4AudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    Comp4AudioProcessor();
    ~Comp4AudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // GUI sliders
    double ratio, thresh, knee, inputGain, outputGain, attack, release, hold, lookAhead, rmsWindowLength, sidechainGain, sidechainHP, sidechainLP;
    // GUI booleans
    bool downward, sidechainEnable, sidechainListen, sidechainMuteInput;
    // previous gain values for each pair of channels, used for applying gain ramps to avoid clicks
    double previousInputGain, previousSidechainGain, previousOutputGain;
    // total number of samples for each timing parameter
    double attackSamples, releaseSamples, holdSamples, lookAheadSamples, rmsWindowSamples;
    // number of samples left of the total - the value depletes as the ramp applied by attack time etc. nears its end
    double attackSamplesLeft, releaseSamplesLeft, holdSamplesLeft, lookAheadSamplesLeft, rmsWindowSamplesLeft;
    // sum of squares of values in the current rms window
    double rmsSquareSum;
    // compression is engaged - last value crossed the compression threshold and attack samples (if any) have been deployed. if false, hold and then release samples are deployed and eventually compression stops
    bool compressionEngaged;
    //float prevValue;
    // need to figure out, i forgor
    double attacka, releasea;
    //double attackb, releaseb;
    // compression ratio adjusted for bezier curves (for values that fall in the knee of the curve)
    double bezierRatio;
    // compression threshold adjusted for bezier curves (for values that fall in the knee of the curve)
    double bezierThresh;
    // compression ratio adjusted for bezier curves and timing variables (attack, hold, release)
    double currentRatio;
    // compression threshold adjusted for bezier curves and timing variables (attack, hold, release)
    double currentThresh;
    //double currentValuesOld[6];
    std::vector<double> currentValues[6];
    //std::vector<std::vector<float>> currentValues;
    //std::vector<float> vec1;
    double originalValues[2];
    double sdb, sdbkey;
    float* s;
    bool windowOpen = false;

    int debugCurrentFunctionIndexEditor = 0;
    int debugCurrentFunctionIndexProcessor = 0;

    double Comp4DecibelsToAmplitude(double decibelValue);

private:
    //==============================================================================

    //double Comp4DecibelsToAmplitude(double decibelValue);
    double Comp4GetBezier(float input);
    double Comp4GetRatiod(float input);
    double Comp4GetBezierdb(double input);
    double Comp4GetRatioddb(double input);
    void Comp4UpdateBezier(double input);
    double Comp4AmplitudeToDecibels(float signal);
    void Comp4EngageCompression();
    void Comp4DisengageCompression();

    juce::dsp::LadderFilter<float> hpf;
    juce::dsp::LadderFilter<float> lpf;
    //juce::IIRFilter hpfl;
    //juce::IIRFilter hpfr;
    //juce::IIRFilter lpfl;
    //juce::IIRFilter lpfr;
    //juce::dsp::ProcessorDuplicator <juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lpf;
    //juce::dsp::ProcessorDuplicator <juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> hpf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Comp4AudioProcessor)
};
