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
    // Linear gain values for each pair of channels, used for restoring parameters to GUI when closing and reopening a window.
    double inputGainLin, outputGainLin, sidechainGainLin;
    // total number of samples for each timing parameter
    //int attackSamples, releaseSamples, holdSamples, lookAheadSamples, rmsWindowSamples;
    int holdSamples, lookAheadSamples, rmsWindowSamples;
    //double attackSamples, releaseSamples, holdSamples, lookAheadSamples, rmsWindowSamples;
    // number of samples left of the total - the value depletes as the ramp applied by attack time etc. nears its end
    int holdSamplesLeft;
    //int attackSamplesLeft, releaseSamplesLeft, holdSamplesLeft, lookAheadSamplesLeft, rmsWindowSamplesLeft;
    //double attackSamplesLeft, releaseSamplesLeft, holdSamplesLeft, lookAheadSamplesLeft, rmsWindowSamplesLeft;
    // Previous number of samples assigned to the timing parameter, stored in case there was a change mid-processing, in which case the samplesLeft value gets assigned the new samples count value.
    int holdSamplesPrevious;
    // compression is engaged - last value crossed the compression threshold and attack samples (if any) have been deployed. if false, hold and then release samples are deployed and eventually compression stops
    //bool compressionEngaged;
    // The currently processed sample has fallen beneath the compression threshold and release samples have been deployed
    //bool compressionReleased;
    // States if the compression is currently in attack phase (currently processed value has crossed the threshold), or release phase.
    bool attackPhase;
    //float prevValue;
    // The a parameter in formula [ratio = a / (x - b)].
    // Using this formula, ratio is adjusted based on the timing parameters, changing gradually over the course of set time interval.
    // The formula y = 1 / x was chosen, because it gives equal gain reduction in each step - was the ratio to change in a linear way, gain reduction
    // would mainly take place in the first few samples, making the implementation poor.
    // Parameters a and b were introduced to be able to adjust the function so that it contains two necessary points (previous ratio and new ratio)
    // Calculated from y(x) = a / (x - b), y(-1) = previousRatio, y(attackSamples) = newRatio
    // In this case, attackSamples is the introduced latency in samples, if it's equal to 0 the ratio achieves its target value in the same sample the target value was calculated from,
    // essentially working in peak detection mode
    // The reason y(-1) = previousRatio is that the ratio was applied to the previous sample, and starting from the currently processed sample [y(0)] the ratio adjusts smoothly to eventually reach the used defined ratio.
    //double attacka, releasea;
    // The b parameter in formula [ratio = a / (x - b)].
    //double attackb, releaseb;
    // Compression ratio adjusted for bezier curves (for values that fall within the knee of the curve).
    double bezierRatio;
    // Compression threshold adjusted for bezier curves (for values that fall within the knee of the curve).
    double bezierThresh;
    // Compression ratio adjusted for bezier curves and timing variables (attack, hold, release).
    double currentRatio;
    // Compression threshold adjusted for bezier curves and timing variables (attack, hold, release).
    double currentThresh;
    // Compression ratio of the previous sample, used for smoothly adjusting the ratio during dynamic changes.
    //double previousRatio;
    // Compression threshold of the previous sample, used for smoothly adjusting the threshold during dynamic changes.
    //double previousThreshold;
    // Target gain reduction of a sample, adjusted further by attack, release and hold.
    double gainReduction;
    // Gain reduction of the previously processed samples (the same for L & R);
    double previousGainReduction;
    // Calculated from attack parameter implemented as time in which gain increase in upward compression, or gain reduction in downward compression, can change by 10 dB - describes the maximum per sample change.
    double maxAttackGainPerSample;
    // Calculated from release parameter implemented as time in which gain reduction in upward compression, or gain increase in downward compression, can change by 10 dB - describes the maximum per sample change.
    double maxReleaseGainPerSample;
    //double currentValuesOld[6];
    // Array of vectors containing samples from stereo input, sidechain and output, used for displaying said values and gain in bars on the right side of the GUI.
    std::vector<double> currentValues[6];
    //std::vector<float> currentValues[6];
    //std::vector<std::vector<float>> currentValues;
    //std::vector<float> vec1;
    //double originalValues[2];
    // Array preserving values of two samples (left and right channel) from before they are processed. The values are passed to the currentValues array if the plugin window is currently open.
    //float originalValues[2];
    // Array holding values of previous L & R samples (post-processing) in decibels.
    //double previousSampledB[2];
    // Array holding signs of previous L & R samples.
    //bool previousSamplePositive[2];
    // Difference in value between currently processed sample and previous sample (post-processing), used for applying attack, hold and release by capping this difference appropriately.
    //double previousSampleDifference;
    // Value of currently processed sample from the main input channel.
    // float* s;
    float s;
    // Value of currently processed sample from the main input channel converted to dBFS.
    double sdb;
    // Depending on sidechain being enabled, either value of currently processed sample from the main input channel or sidechain channel converted to dBFS.
    //double sdbkey;
    // Decibel value of RMS value of currently processed L & R sample from the main input.
    double sdbmean;
    // Returns true if plugin window is currently open. Used for displaying data on bars.
    bool pluginWindowOpen = false;
    // Array of queues (L & R channels) containing values in the current RMS window. Used for calculating the moving average of values needed for estimating the current power of the signal, used in conjunction with timing parameters.
    std::queue<float> rmsQueues[2];
    // Returns true if sidechain was enabled/disabled since last checked (during the processing of previous block).
    bool keySignalSwitched = true;
    // Stores the previous value of key signal. True - main input, false - sidechain.
    bool keySignalPrevious = true;
    // Offset to each side of current value in miliseconds during calculating the moving average.
    //int rmsOffsetInMs;
    // Offset to each side of current value in samples during calculating the moving average.
    int rmsOffsetInSamples;
    // Array of double ended queues storing the values from the previous blocks. Needed for preserving the continuity of moving average calculations and look-ahead.
    std::deque<float> memoryBuffer[4];
    // Minimal latency in miliseconds (needed for RMS window calculations and look-ahead). Used for picking one of pre-defined latency steps.
    //double minimalLatencyInMs;
    // Minimal latency in samples (needed for RMS window calculations and look-ahead). Used for picking one of pre-defined latency steps.
    int minimalLatencyInSamples;
    // Pre-defined latency steps in miliseconds, implemented to reduce number of reported latency changes after every small adjustment.
    int latencyStepsInMs[7] = { 1, 5, 20, 50, 100, 200, 500 };
    // Pre-defined latency steps in samples, implemented to reduce number of reported latency changes after every small adjustment.
    int latencyStepsInSamples[7];
    // Used for checking if sample rate changed and updating latency steps.
    double sampleRate;
    // Current plugin latency in samples. Equal to one of the values in latencyStepsInSamples.
    int currentLatencyInSamples;
    // Sum of squares of values in the current RMS window in the key signal.
    double rmsSquareSum;
    // Moving RMS value of currently processed sample from the key signal converted to dBFS.
    double sdbkeyrms;
    // States if there are 4 or more input channels detected.
    bool sidechainChannelsExist;

    int debugCurrentFunctionIndexEditor = 0;
    int debugCurrentFunctionIndexProcessor = 0;

    double Comp4DecibelsToAmplitude(double decibelValue);
    //void Comp4UpdateLatency(double newMinimalLatencyInMs);

private:
    //==============================================================================

    //double Comp4DecibelsToAmplitude(double decibelValue);
    double Comp4GetBezier(float input);
    double Comp4GetRatiod(float input);
    double Comp4GetBezierdb(double input);
    double Comp4GetRatioddb(double input);
    void Comp4UpdateBezier(double input);
    double Comp4AmplitudeToDecibels(float signal);
    //void Comp4EngageCompression();
    //void Comp4DisengageCompression();
    void clear(std::queue<int>&);
    // void Comp4UpdateLatencySteps(double sampleRate);
    // void Comp4UpdateLatency(double sampleRate, double newMinimalLatencyInMs);
    void Comp4UpdateLatencySteps();
    void Comp4UpdateLatencyCore();
    //void Comp4UpdateLatencyMs(double newMinimalLatencyInMs);
    void Comp4UpdateLatency(double newMinimalLatencyInSamples);
    //void Comp4UnreleaseCompression();

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
