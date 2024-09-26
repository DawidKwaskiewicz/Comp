/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Comp4AudioProcessor::Comp4AudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    ratio = 1.0;
    thresh = 0.0;
    knee = 3.0;
    inputGain = 0.0;
    outputGain = 0.0;
    attack = 10.0;
    release = 50.0;
    hold = 0.0;
    lookAhead = 0.0;
    rmsWindowLength = 1.0;
    sidechainGain = 0.0;
    sidechainHP = 20.0;
    sidechainLP = 20000.0;
    downward = false;
    sidechainEnable = false;
    sidechainListen = false;
    sidechainMuteInput = false;
    previousInputGain = 0.0;
    previousSidechainGain = 0.0;
    previousOutputGain = 0.0;
    inputGainLin = 0.0;
    outputGainLin = 0.0;
    sidechainGainLin = 0.0;
    sampleRate = 48000.0;
    holdSamples = std::round(hold * sampleRate * 0.001);
    lookAheadSamples = std::round(lookAhead * sampleRate * 0.001);
    rmsWindowSamples = std::round(rmsWindowLength * sampleRate * 0.001);
    if (rmsWindowSamples % 2 == 0) rmsWindowSamples++;
    rmsOffsetInSamples = (rmsWindowSamples - 1) / 2;
    holdSamplesLeft = holdSamples;
    holdSamplesPrevious = holdSamples;
    gainReduction = 0.0;
    previousGainReduction = 0.0;
    maxAttackGainPerSample = 10.0 / attack / sampleRate * 1000.0;
    maxReleaseGainPerSample = 10.0 / release / sampleRate * 1000.0;
    bezierRatio = 1.0;
    bezierThresh = 0.0;
    rmsSquareSum[0] = 0.0;
    rmsSquareSum[1] = 0.0;
    rmsSquareSum[2] = 0.0;
    rmsSquareSum[3] = 0.0;
    sdbkeyrms = 0.0;
    attackPhase = false;
    pluginWindowOpen = false;
    firstCall = true;
    minimalLatencyInSamples = 0;
    currentLatencyInSamples = 0;
    latencyStepsInMs[0] = 1;
    latencyStepsInMs[1] = 5;
    latencyStepsInMs[2] = 20;
    latencyStepsInMs[3] = 50;
    latencyStepsInMs[4] = 100;
    latencyStepsInMs[5] = 200;
    latencyStepsInMs[6] = 551;
    Comp4UpdateLatencySteps();
    Comp4UpdateLatency(rmsOffsetInSamples + lookAheadSamples);
}

Comp4AudioProcessor::~Comp4AudioProcessor()
{
}

//==============================================================================
const juce::String Comp4AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Comp4AudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Comp4AudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Comp4AudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Comp4AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Comp4AudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Comp4AudioProcessor::getCurrentProgram()
{
    return 0;
}

void Comp4AudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Comp4AudioProcessor::getProgramName (int index)
{
    return {};
}

void Comp4AudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Comp4AudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::dsp::ProcessSpec psp;
    psp.sampleRate = sampleRate;
    psp.numChannels = getMainBusNumOutputChannels();
    psp.maximumBlockSize = samplesPerBlock;

    hpf.prepare(psp);
    hpf.reset();
    hpf.setMode(juce::dsp::LadderFilterMode::HPF24);
    hpf.setCutoffFrequencyHz(sidechainHP);
    hpf.setResonance(0.7f);
    hpf.setDrive(1.0f);
    hpf.setEnabled(true);

    lpf.prepare(psp);
    lpf.reset();
    lpf.setMode(juce::dsp::LadderFilterMode::LPF24);
    lpf.setCutoffFrequencyHz(sidechainLP);
    lpf.setResonance(0.7f);
    lpf.setDrive(1.0f);
    lpf.setEnabled(true);
}

void Comp4AudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Comp4AudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Comp4AudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlockCallCounter++;
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto smpNum = buffer.getNumSamples();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    auto* channelsData = buffer.getArrayOfWritePointers();

    if (totalNumInputChannels >= 4)
    {
        sidechainChannelsExist = true;
        totalNumInputChannels = 4;
    }
    else
    {
        sidechainChannelsExist = false;
        totalNumInputChannels = 2;
    }

    if (previousInputGain != inputGain || inputGain != 0)
    {
        buffer.applyGainRamp(0, 0, buffer.getNumSamples(), previousInputGain, inputGain);
        buffer.applyGainRamp(1, 0, buffer.getNumSamples(), previousInputGain, inputGain);
        previousInputGain = inputGain;
    }
    if (sidechainChannelsExist && (previousSidechainGain != sidechainGain || sidechainGain != 0))
    {
        buffer.applyGainRamp(2, 0, buffer.getNumSamples(), previousSidechainGain, sidechainGain);
        buffer.applyGainRamp(3, 0, buffer.getNumSamples(), previousSidechainGain, sidechainGain);
        previousSidechainGain = sidechainGain;
    }
    double newSampleRate = getSampleRate();
    if (newSampleRate != sampleRate)
    {
        sampleRate = newSampleRate;
        Comp4UpdateLatencySteps();
    }

    // Processing the sidechain input with high-pass and low-pass filters.
    if (sidechainChannelsExist)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        block = block.getSubsetChannelBlock(2, 2);
        lpf.setCutoffFrequencyHz(sidechainLP);
        hpf.setCutoffFrequencyHz(sidechainHP);
        hpf.process(juce::dsp::ProcessContextReplacing<float>(block));
        lpf.process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    // Calculating each timing parameter from ms to samples, as the sample rate may have changed and its accurate retrieval is only possible in processBlock().
    holdSamples = std::round(hold * sampleRate * 0.001);
    if (holdSamples != holdSamplesPrevious)
    {
        holdSamplesLeft = holdSamples;
        holdSamplesPrevious = holdSamples;
    }
    maxAttackGainPerSample = 10.0 / attack / sampleRate * 1000.0;
    maxReleaseGainPerSample = 10.0 / release / sampleRate * 1000.0;
    lookAheadSamples = std::round(lookAhead * sampleRate * 0.001);
    rmsWindowSamples = std::round(rmsWindowLength * sampleRate * 0.001);
    if (rmsWindowSamples % 2 == 0) rmsWindowSamples++;
    rmsOffsetInSamples = (rmsWindowSamples - 1) / 2;
    Comp4UpdateLatency(rmsOffsetInSamples + lookAheadSamples);

    // Initiating rmsQueues by filling them with 0's.
    if (firstCall)
    {
        firstCall = false;
        for (int channel = 0; channel < totalNumInputChannels; channel++)
        {
            rmsQueues[channel] = std::queue<float>();
            for (int sample = 0; sample < rmsWindowSamples; sample++)
                rmsQueues[channel].push(0);
        }
    }

    // Passing the samples from buffer to the processing buffer and RMS memory buffer.
    for (int sample = 0; sample < smpNum; sample++)
    {
        for (int channel = 0; channel < 2; channel++)
            mainInputBuffer[channel].push_back(channelsData[channel][sample]);
        for (int channel = 0; channel < totalNumInputChannels; channel++)
            rmsMemoryBuffers[channel].push_back(channelsData[channel][sample]);
    }
    // Filling the RMS buffers.
    float rmsValueToRemove, rmsValueToAdd;
    while (rmsMemoryBuffers[0].size() > currentLatencyInSamples)
    {
        double rmsCalculatedValue;
        float valueSquared;
        for (int channel = 0; channel < totalNumInputChannels; channel++)
        {
            if (rmsQueues[channel].size() >= rmsWindowSamples)
            {
                rmsValueToRemove = rmsQueues[channel].front();
                valueSquared = rmsValueToRemove * rmsValueToRemove;
                rmsSquareSum[channel] -= valueSquared;
                rmsQueues[channel].pop();
            }
            rmsValueToAdd = rmsMemoryBuffers[channel][rmsOffsetInSamples];
            valueSquared = rmsValueToAdd * rmsValueToAdd;
            if (valueSquared < 0)
            {
                valueSquared = 0;
                rmsValueToAdd = 0;
            }
            rmsQueues[channel].push(rmsValueToAdd);
            rmsSquareSum[channel] += valueSquared;
            if (rmsSquareSum[channel] < 0) rmsSquareSum[channel] = 0;
            rmsMemoryBuffers[channel].pop_front();
            rmsCalculatedValue = std::sqrt(rmsSquareSum[channel] / rmsWindowSamples);
            rmsStereoBuffers[channel].push_back(rmsCalculatedValue);
        }
        rmsCalculatedValue = std::sqrt((rmsSquareSum[0] + rmsSquareSum[1]) / rmsWindowSamples / 2.0);
        jassert(!std::isnan(rmsCalculatedValue));
        rmsMonoBuffers[0].push_back(rmsCalculatedValue);
        rmsCalculatedValue = std::sqrt((rmsSquareSum[2] + rmsSquareSum[3]) / rmsWindowSamples / 2.0);
        jassert(!std::isnan(rmsCalculatedValue));
        rmsMonoBuffers[1].push_back(rmsCalculatedValue);
    }
    
    // If RMS window parameter decreased since last call to processBlock(), excessive old samples need to be erased.
    while (rmsQueues[0].size() > rmsWindowSamples)
        for (int channel = 0; channel < totalNumInputChannels; channel++)
        {
            rmsValueToRemove = rmsQueues[channel].front();
            rmsSquareSum[channel] -= rmsValueToRemove * rmsValueToRemove;
            rmsQueues[channel].pop();
        }

    // If latency exceeds currently accumulated samples, buffer is cleared and returned, as there is not enough samples to begin processing yet.
    if (mainInputBuffer[0].size() <= currentLatencyInSamples)
    {
        buffer.clear();
        return;
    }


    // Total amount of samples that are able to be processed (accumulated samples minus latency in samples).
    int samplesToProcess = mainInputBuffer[0].size() - currentLatencyInSamples;
    // In case the latency was reduced since last call to processBlock, older samples are removed until samples count in mainInputBuffer is equal to sum of samples in buffer and currentLatencyInSamples.
    for (int sample = 0; sample < samplesToProcess - smpNum; sample++)
    {
        for (int channel = 0; channel < 2; channel++)
        {
            mainInputBuffer[channel].pop_front();
            rmsMonoBuffers[channel].pop_front();
        }
        for (int channel = 0; channel < totalNumInputChannels; channel++)
        {
            rmsStereoBuffers[channel].pop_front();
        }
    }
    // On the other hand, if samples count in memoryBuffer is smaller than sum of samples in buffer and currentLatencyInSamples
    // (this is the case if this is the first processBlock call with enough samples to start processing), then 0's are used to fill the beginning of the buffer.
    int startingSample = smpNum - mainInputBuffer[0].size() + currentLatencyInSamples;
    jassert(startingSample >= 0);
    for (int sample = 0; sample < startingSample; sample++)
        for (int channel = 0; channel < totalNumInputChannels; channel++)
            channelsData[channel][sample] = 0;

    for (int smp = startingSample; smp < smpNum; ++smp)
    {
        for (int channel = 0; channel < 2; ++channel)
        {
            s[channel] = mainInputBuffer[channel].front();
            sdb[channel] = Comp4AmplitudeToDecibels(s[channel]);
            sdbkeyrms = Comp4AmplitudeToDecibels(rmsMonoBuffers[sidechainEnable && sidechainChannelsExist].front());
        }

        for (int input = 0; input < 2; input++)
            rmsMonoBuffers[input].pop_front();

        if (ratio != 1.0 && thresh != 0.0)
        {
            if (!downward)
            {
                if (sdbkeyrms >= thresh + knee / 2.0)
                {
                    gainReduction = sdbkeyrms - (thresh + (sdbkeyrms - thresh) / ratio);
                    attackPhase = true;

                }
                else if (sdbkeyrms > thresh - knee / 2.0)
                {
                    Comp4UpdateBezier(sdbkeyrms);
                    gainReduction = sdbkeyrms - (bezierThresh + (sdbkeyrms - bezierThresh) / bezierRatio);
                    attackPhase = true;
                }
                else
                {
                    gainReduction = 0;
                    attackPhase = false;
                }
            }
            else
            {
                if (sdbkeyrms <= thresh - knee / 2.0)
                {
                    gainReduction = sdbkeyrms - (thresh - (thresh - sdbkeyrms) / ratio);
                    attackPhase = true;
                }
                else if (sdbkeyrms < thresh + knee / 2.0)
                {
                    Comp4UpdateBezier(sdbkeyrms);
                    gainReduction = sdbkeyrms - (bezierThresh - (bezierThresh - sdbkeyrms) / bezierRatio);
                    attackPhase = true;
                }
                else
                {
                    gainReduction = 0;
                    attackPhase = false;
                }
            }
        }
        else
        {
            gainReduction = 0;
            attackPhase = false;
        }
        jassert(!std::isnan(gainReduction) && !std::isinf(gainReduction));

        if (attackPhase && std::abs(gainReduction - previousGainReduction) > maxAttackGainPerSample)
        {
            if (gainReduction > previousGainReduction) gainReduction = previousGainReduction + maxAttackGainPerSample;
            else gainReduction = previousGainReduction - maxAttackGainPerSample;
            holdSamplesLeft = holdSamples;
        }
        else if (!attackPhase && holdSamplesLeft > 0)
        {
            gainReduction = previousGainReduction;
            holdSamplesLeft--;
        }
        else if (!attackPhase && std::abs(gainReduction - previousGainReduction) > maxReleaseGainPerSample)
        {
            if (gainReduction > previousGainReduction) gainReduction = previousGainReduction + maxReleaseGainPerSample;
            else gainReduction = previousGainReduction - maxReleaseGainPerSample;
        }
        previousGainReduction = gainReduction;

        for (int channel = 0; channel < 2; ++channel)
        {
            s[channel] = s[channel] >= 0 ? Comp4DecibelsToAmplitude(sdb[channel] - gainReduction) : -1.0 * Comp4DecibelsToAmplitude(sdb[channel] - gainReduction);
            jassert(!std::isnan(s[channel]));
            if (pluginWindowOpen)
            {
                currentValues[channel].push_back(rmsStereoBuffers[channel].front());
                if (sidechainChannelsExist) currentValues[channel + 2].push_back(rmsStereoBuffers[channel + 2].front());
                currentValues[channel + 4].push_back(s[channel]);
            }
            if (sidechainChannelsExist)
            {
                channelsData[channel][smp] = !sidechainMuteInput * s[channel] + sidechainListen * mainInputBuffer[channel + 2].front();
            }
            else
            {
                channelsData[channel][smp] = !sidechainMuteInput * s[channel];
            }
            mainInputBuffer[channel].pop_front();
        }
        for (int channel = 0; channel < totalNumInputChannels; channel++)
        {
            rmsStereoBuffers[channel].pop_front();
        }
    }

    if (previousOutputGain != outputGain || outputGain != 0)
    {
        buffer.applyGainRamp(0, 0, buffer.getNumSamples(), previousOutputGain, outputGain);
        buffer.applyGainRamp(1, 0, buffer.getNumSamples(), previousOutputGain, outputGain);
        previousOutputGain = outputGain;
    }
}

//==============================================================================
bool Comp4AudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Comp4AudioProcessor::createEditor()
{
    return new Comp4AudioProcessorEditor (*this);
}

//==============================================================================
void Comp4AudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Comp4AudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Comp4AudioProcessor();
}

double Comp4AudioProcessor::Comp4DecibelsToAmplitude(double decibelValue)
{
    return std::pow(10.0, decibelValue / 20.0);
}


double Comp4AudioProcessor::Comp4AmplitudeToDecibels(float signal)
{
    return 20.0 * std::log10(std::abs(signal));
}

void Comp4AudioProcessor::Comp4UpdateBezier(double inputdb)
{
    double kneehalf = knee / 2.0;
    // X coordinates of three given points of the bezier curve being the knee of the compression graph
    double x1 = thresh - kneehalf;
    double x2 = thresh;
    double x3 = thresh + kneehalf;
    jassert(inputdb >= x1 && inputdb <= x3);
    // Calculated from Bezier curve formula for x coordinate after solving for t
    double t = (thresh - kneehalf - inputdb) / (-2.0 * kneehalf);
    // Y coordinates of three given points
    double y1, y2, y3, y;
    y2 = thresh;
    if (!downward)
    {
        y1 = thresh - kneehalf;
        y3 = y2 + kneehalf / ratio;
    }
    else
    {
        y3 = thresh + kneehalf;
        y1 = y2 - kneehalf / ratio;
    }

    // Y coordinate of the curve for x = inputdb
    y = (1 - t) * (1 - t) * y1 + 2 * (1 - t) * t * y2 + t * t * y3;
    double xprim = 2.0 * (1.0 - t) * (x2 - x1) + 2.0 * t * (x3 - x2);
    double yprim = 2.0 * (1.0 - t) * (y2 - y1) + 2.0 * t * (y3 - y2);
    // Dividing the derivatives gives us the slope of the tangent line in point (inputdb, y), which is by definition the (instantaneous) compression ratio
    // (the compression graph no longer consists only of straight lines, and because of that both the ratio and the threshold change dynamically in the region of the knee)
    double beziera = yprim / xprim;
    bezierRatio = 1 / beziera;
    // Coefficient b in formula of the tangent line (y = ax + b), needed for calculating the instantaneous threshold
    // (as the values below the chosen threshold are now being processed (half of the knee in x-axis), the threshold for those values needs to be adjusted)
    // Calculated by knowing the a parameter and one point [inputdb, y]
    double bezierb = y - beziera * inputdb;
    // Using both coefficients (a = bezierRatio, b = bezierb) we calculate the y coordinate of the point in which the tangent line intersects the y = x line (the uncompressed part of the signal)
    // That coordinate is the isntantaneous threshold
    bezierThresh = bezierb / (1.0 - beziera);
    jassert((bezierRatio >= ratio && bezierRatio <= 1) || (bezierRatio <= ratio && bezierRatio >= 1));
    jassert((bezierThresh >= x1 && bezierThresh <= x2 && bezierThresh <= inputdb && !downward) || (bezierThresh >= x2 && bezierThresh <= x3 && bezierThresh >= inputdb && downward));
    return;
}
void Comp4AudioProcessor::Comp4UpdateLatencySteps()
{
    for (int i = 0; i < 7; i++) latencyStepsInSamples[i] = std::ceil(sampleRate * latencyStepsInMs[i] * 0.001);
    Comp4UpdateLatencyCore();
}
void Comp4AudioProcessor::Comp4UpdateLatencyCore()
{
    //double minimalLatencyInSamples = std::ceil(minimalLatencyInMs * sampleRate * 0.001);
    for (int i = 0; i < 7; i++)
    {
        if (minimalLatencyInSamples <= latencyStepsInSamples[i])
        {
            currentLatencyInSamples = latencyStepsInSamples[i];
            setLatencySamples(currentLatencyInSamples);
            return;
        }
    }
    throw std::invalid_argument("Requested latency out of range of predicted values");
}
void Comp4AudioProcessor::Comp4UpdateLatency(double newMinimalLatencyInSamples)
{
    if (newMinimalLatencyInSamples == minimalLatencyInSamples) return;
    minimalLatencyInSamples = newMinimalLatencyInSamples;
    Comp4UpdateLatencyCore();
}
