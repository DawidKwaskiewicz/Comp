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
    int holdSamples, lookAheadSamples, rmsWindowSamples;
    // number of samples left of the total - the value depletes as the ramp applied by attack time etc. nears its end
    int holdSamplesLeft;
    // Previous number of samples assigned to the timing parameter, stored in case there was a change mid-processing, in which case the samplesLeft value gets assigned the new samples count value.
    int holdSamplesPrevious;
    // States if the compression is currently in attack phase (currently processed value has crossed the threshold), or release phase.
    bool attackPhase;
    // Compression ratio adjusted for bezier curves (for values that fall within the knee of the curve).
    double bezierRatio;
    // Compression threshold adjusted for bezier curves (for values that fall within the knee of the curve).
    double bezierThresh;
    // Target gain reduction of a sample, adjusted further by attack, release and hold.
    double gainReduction;
    // Gain reduction of the previously processed samples (the same for L & R);
    double previousGainReduction;
    // Calculated from attack parameter implemented as time in which gain increase in upward compression, or gain reduction in downward compression, can change by 10 dB - describes the maximum per sample change.
    double maxAttackGainPerSample;
    // Calculated from release parameter implemented as time in which gain reduction in upward compression, or gain increase in downward compression, can change by 10 dB - describes the maximum per sample change.
    double maxReleaseGainPerSample;
    // Array of vectors containing samples from stereo input, sidechain and output, used for displaying said values and gain in bars on the right side of the GUI.
    std::vector<double> currentValues[6];
    // Value of currently processed samples (L&R) from the main input channel.
    float s[2];
    // Value of currently processed samples (L&R) from the main input channel converted to dBFS.
    double sdb[2];
    // Returns true if plugin window is currently open. Used for displaying data on bars.
    bool pluginWindowOpen = false;
    // Array of queues (main input & sidechain, L & R channels) containing values in the current RMS window. Used for calculating the moving average of values needed for estimating the current power of the signal, used in conjunction with timing parameters.
    std::queue<float> rmsQueues[4];
    // Array of queues storing current key input mono values modified by running average (RMS) over time described by the RMS knob.
    std::deque<float> rmsMonoBuffers[2];
    // Array of queues storing current main and sidechain (L&R) input values modified by running average (RMS) over time described by the RMS knob.
    std::deque<float> rmsStereoBuffers[4];
    // Describes if this is the first call to the processBlock() function.
    bool firstCall = true;
    // Offset to each side of current value in samples during calculating the moving average.
    int rmsOffsetInSamples;
    // Array of double ended queues storing main input values from the previous blocks. Needed for preserving the continuity of moving average calculations and look-ahead.
    std::deque<float> mainInputBuffer[2];
    // Array of double ended queues storing main input and sidechain values from the previous blocks. Needed for RMS calculations.
    std::deque<float> rmsMemoryBuffers[4];
    // Minimal latency in samples (needed for RMS window calculations and look-ahead). Used for picking one of pre-defined latency steps.
    int minimalLatencyInSamples;
    // Pre-defined latency steps in miliseconds, implemented to reduce number of reported latency changes after every small adjustment.
    int latencyStepsInMs[7] = { 1, 5, 20, 50, 100, 200, 551 };
    // Pre-defined latency steps in samples, implemented to reduce number of reported latency changes after every small adjustment.
    int latencyStepsInSamples[7];
    // Used for checking if sample rate changed and updating latency steps.
    double sampleRate;
    // Current plugin latency in samples. Equal to one of the values in latencyStepsInSamples.
    int currentLatencyInSamples;
    // Sum of squares of values in the current RMS window in main and sidechain input (stereo).
    double rmsSquareSum[4];
    // Moving RMS value of currently processed sample from the key signal converted to dBFS.
    double sdbkeyrms;
    // States if there are 4 or more input channels detected.
    bool sidechainChannelsExist;
    // Debugging variable counting calls to processBlock().
    int processBlockCallCounter = 0;

    double Comp4DecibelsToAmplitude(double decibelValue);

private:
    //==============================================================================

    void Comp4UpdateBezier(double input);
    double Comp4AmplitudeToDecibels(float signal);
    void Comp4UpdateLatencySteps();
    void Comp4UpdateLatencyCore();
    void Comp4UpdateLatency(double newMinimalLatencyInSamples);

    juce::dsp::LadderFilter<float> hpf;
    juce::dsp::LadderFilter<float> lpf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Comp4AudioProcessor)
};
