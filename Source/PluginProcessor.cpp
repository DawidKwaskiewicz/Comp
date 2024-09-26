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
    //vec1.push_back(1.0f);
    //currentValues.push_back(vec1);
    debugCurrentFunctionIndexProcessor = 1;

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
    //attackSamples = std::round(attack * sampleRate * 0.001);
    //releaseSamples = std::round(release * sampleRate * 0.001);
    holdSamples = std::round(hold * sampleRate * 0.001);
    lookAheadSamples = std::round(lookAhead * sampleRate * 0.001);
    rmsWindowSamples = std::round(rmsWindowLength * sampleRate * 0.001);
    if (rmsWindowSamples % 2 == 0) rmsWindowSamples++;
    rmsOffsetInSamples = (rmsWindowSamples - 1) / 2;
    //rmsOffsetInSamples = std::round(rmsOffsetInMs * sampleRate * 0.001);
    //attackSamplesLeft = attackSamples;
    //releaseSamplesLeft = releaseSamples;
    holdSamplesLeft = holdSamples;
    holdSamplesPrevious = holdSamples;
    //lookAheadSamplesLeft = lookAheadSamples;
    //rmsWindowSamplesLeft = rmsWindowSamples;
    gainReduction = 0.0;
    previousGainReduction = 0.0;
    // maxAttackGainPerSample = 10.0 / attack / sampleRate * 0.001;
    // maxReleaseGainPerSample = 10.0 / release / sampleRate * 0.001;
    maxAttackGainPerSample = 10.0 / attack / sampleRate * 1000.0;
    maxReleaseGainPerSample = 10.0 / release / sampleRate * 1000.0;
    //compressionEngaged = false;
    //prevValue = 0.0f;
    bezierRatio = 1.0;
    bezierThresh = 0.0;
    currentRatio = 1.0;
    currentThresh = 0.0;
    rmsSquareSum[0] = 0.0;
    rmsSquareSum[1] = 0.0;
    rmsSquareSum[2] = 0.0;
    rmsSquareSum[3] = 0.0;
    sdbkeyrms = 0.0;
    attackPhase = false;
    //originalValues[0] = 0.0f;
    //originalValues[1] = 0.0f;
    pluginWindowOpen = false;
    keySignalSwitched = true;
    keySignalPrevious = true;
    firstCall = true;
    // Comp4UpdateLatencySteps(oldSampleRate);
    // Comp4UpdateLatency(oldSampleRate, std::max(rmsWindowLength, lookAhead));
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
    //Comp4UpdateLatency(std::max(rmsWindowLength, lookAhead));
    //Comp4UpdateLatency(std::max(rmsOffsetInSamples, lookAheadSamples));
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
    debugCurrentFunctionIndexProcessor = 2;
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::dsp::ProcessSpec psp;
    psp.sampleRate = sampleRate;
    psp.numChannels = getMainBusNumOutputChannels();
    psp.maximumBlockSize = samplesPerBlock;

    /*lpf.prepare(psp);
    lpf.reset();
    hpf.prepare(psp);
    hpf.reset();*/

    //lpfl = new juce::IIRFilter(juce::IIRCoefficients::makeLowPass(sampleRate, sidechainLP));

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



    //hpfl = new juce::IIRFilter(juce::IIRCoefficients::makeHighPass(sampleRate, sidechainHP));
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
    debugCurrentFunctionIndexProcessor = 3;
    processBlockCallCounter++;
    //juce::ScopedNoDenormals noDenormals;
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

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    /*for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel, 0);
    }*/

    auto* channelsData = buffer.getArrayOfWritePointers();

    //double xknee1 = Comp4DecibelsToAmplitude(thresh - knee / 2.0);
    //double xknee2 = Comp4DecibelsToAmplitude(thresh + knee / 2.0);
    //double inputGainLin = Comp4DecibelsToAmplitude(inputGain);

    sidechainChannelsExist = totalNumInputChannels >= 4;
    if (sidechainChannelsExist) totalNumInputChannels = 4;
    else totalNumInputChannels = 2;

    if (previousInputGain != inputGain || inputGain != 0)
    {
        buffer.applyGainRamp(0, 0, buffer.getNumSamples(), previousInputGain, inputGain);
        buffer.applyGainRamp(1, 0, buffer.getNumSamples(), previousInputGain, inputGain);
        previousInputGain = inputGain;
    }
    //double sidechainGainLin = Comp4DecibelsToAmplitude(sidechainInputGain);
    if (sidechainChannelsExist && (previousSidechainGain != sidechainGain || sidechainGain != 0))
    {
        buffer.applyGainRamp(2, 0, buffer.getNumSamples(), previousSidechainGain, sidechainGain);
        buffer.applyGainRamp(3, 0, buffer.getNumSamples(), previousSidechainGain, sidechainGain);
        previousSidechainGain = sidechainGain;
    }
    //double outputGainLin = Comp4DecibelsToAmplitude(outputGain);
    double newSampleRate = getSampleRate();
    if (newSampleRate != sampleRate)
    {
        sampleRate = newSampleRate;
        Comp4UpdateLatencySteps();
    }
    // attackSamples = std::round(attack * sampleRate * 0.001);
    // releaseSamples = std::round(release * sampleRate * 0.001);
    // holdSamples = std::round(hold * sampleRate * 0.001);
    // lookAheadSamples = std::round(lookAhead * sampleRate * 0.001);
    // rmsWindowSamples = std::round(rmsWindowLength * sampleRate * 0.001);
    // releasea = (ratio * releaseSamples - ratio) / (1.0 - ratio);
    //double sampleRate = getSampleRate();

    // Processing the sidechain input with high-pass and low-pass filters.
    //juce::dsp::AudioBlock<float> blockl(buffer);
    if (sidechainChannelsExist)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        block = block.getSubsetChannelBlock(2, 2);
        lpf.setCutoffFrequencyHz(sidechainLP);
        hpf.setCutoffFrequencyHz(sidechainHP);
        hpf.process(juce::dsp::ProcessContextReplacing<float>(block));
        lpf.process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    //attackSamples = std::round(attack * sampleRate * 0.001);
    //releaseSamples = std::round(release * sampleRate * 0.001);
    // Calculating each timing parameter from ms to samples, as the sample rate may have changed and its accurate retrieval is only possible in processBlock
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
    //Comp4UpdateLatency(std::max(rmsOffsetInSamples, lookAheadSamples));
    Comp4UpdateLatency(rmsOffsetInSamples + lookAheadSamples);
    // releasea = (ratio * releaseSamples - ratio) / (1.0 - ratio);
    //maxAttackGainPerSample = 10.0 / attack / sampleRate * 0.001;
    //maxReleaseGainPerSample = 10.0 / release / sampleRate * 0.001;

    //// If sidechain was enabled/disabled since the previous call to processBlock, rms queues are replaced with 0's, since values inside them are no longer relevant. Also called during the first execution.
    //if (keySignalSwitched)
    //{
    //    keySignalSwitched = false;
    //    rmsSquareSum = 0;
    //    for (int channel = 0; channel < 2; channel++)
    //    {
    //        rmsQueues[channel] = std::queue<float>();
    //        for (int sample = 0; sample < rmsWindowSamples; sample++)
    //            rmsQueues[channel].push(0);
    //    }
    //}

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
    //int channelOffset = (sidechainEnable && sidechainChannelsExist) * 2;
    //for (int sample = 0; sample < rmsMemoryBuffers[0].size() - rmsOffsetInSamples; sample++)
    while (rmsMemoryBuffers[0].size() > currentLatencyInSamples)
    {
        double rmsCalculatedValue;
        float valueSquared;
        for (int channel = 0; channel < totalNumInputChannels; channel++)
        {
            //int inputNum = static_cast<int>(std::floorf(channel / 2.0));
            if (rmsQueues[channel].size() >= rmsWindowSamples)
            {
                rmsValueToRemove = rmsQueues[channel].front();
                valueSquared = rmsValueToRemove * rmsValueToRemove;
                //if (valueSquared >= 0) rmsSquareSum[channel] -= valueSquared;
                //rmsSquareSum[channel] -= rmsValueToRemove * rmsValueToRemove;
                rmsSquareSum[channel] -= valueSquared;
                //jassert(rmsSquareSum[channel] >= 0);
                rmsQueues[channel].pop();
            }
            rmsValueToAdd = rmsMemoryBuffers[channel][rmsOffsetInSamples];
            //jassert(rmsValueToAdd == 0);
            valueSquared = rmsValueToAdd * rmsValueToAdd;
            if (valueSquared < 0)
            {
                valueSquared = 0;
                rmsValueToAdd = 0;
            }
            rmsQueues[channel].push(rmsValueToAdd);
            //rmsSquareSum[channel] += rmsValueToAdd * rmsValueToAdd;
            rmsSquareSum[channel] += valueSquared;
            //jassert(rmsSquareSum[channel] >= 0);
            if (rmsSquareSum[channel] < 0) rmsSquareSum[channel] = 0;
            rmsMemoryBuffers[channel].pop_front();
            //rmsMemoryBuffer[channel + 2].pop_front();
            rmsCalculatedValue = std::sqrt(rmsSquareSum[channel] / rmsWindowSamples);
            //jassert(!std::isnan(rmsCalculatedValue));
            //rmsStereoBuffers[channel].push_back(std::sqrt(rmsSquareSum[channel] / rmsWindowSamples));
            rmsStereoBuffers[channel].push_back(rmsCalculatedValue);
        }
        rmsCalculatedValue = std::sqrt((rmsSquareSum[0] + rmsSquareSum[1]) / rmsWindowSamples / 2.0);
        jassert(!std::isnan(rmsCalculatedValue));
        rmsMonoBuffers[0].push_back(rmsCalculatedValue);
        rmsCalculatedValue = std::sqrt((rmsSquareSum[2] + rmsSquareSum[3]) / rmsWindowSamples / 2.0);
        jassert(!std::isnan(rmsCalculatedValue));
        rmsMonoBuffers[1].push_back(rmsCalculatedValue);
        //rmsMonoBuffers[0].push_back(std::sqrt((rmsSquareSum[0] + rmsSquareSum[1]) / rmsWindowSamples / 2.0));
        //rmsMonoBuffers[1].push_back(std::sqrt((rmsSquareSum[2] + rmsSquareSum[3]) / rmsWindowSamples / 2.0));
    }
        //for (int channel = 0; channel < 2; channel++)
        //{
        //    rmsValueToRemove = rmsQueues[channel].front();
        //    rmsSquareSum -= rmsValueToRemove * rmsValueToRemove;
        //    rmsQueues[channel].pop();
        //    rmsValueToAdd = rmsMemoryBuffer[channel][rmsOffsetInSamples];
        //    rmsQueues[channel].push(rmsValueToAdd);
        //    rmsSquareSum += rmsValueToAdd * rmsValueToAdd;
        //    rmsBuffer[channel].push_back(std::sqrt(rmsSquareSum / rmsWindowSamples / 2.0));
        //}
    
    // If RMS window parameter decreased since last call to processBlock(), excessive old samples need to be erased.
    //while (rmsMonoBuffers[0].size() > rmsWindowSamples)
    //{
    //    for (int channel = 0; channel < totalNumInputChannels; channel++)
    //    {
    //        rmsValueToRemove = rmsQueues[channel].front();
    //        rmsSquareSum[channel] -= rmsValueToRemove * rmsValueToRemove;
    //        rmsQueues[channel].pop();
    //        //rmsMemoryBuffers[channel].pop_front();
    //        rmsStereoBuffers[channel].pop_front();
    //    }
    //    for (int input = 0; input <= sidechainChannelsExist; input++)
    //    {
    //        rmsMonoBuffers[input].pop_front();
    //    }
    //}
    while (rmsQueues[0].size() > rmsWindowSamples)
        for (int channel = 0; channel < totalNumInputChannels; channel++)
        {
            rmsValueToRemove = rmsQueues[channel].front();
            rmsSquareSum[channel] -= rmsValueToRemove * rmsValueToRemove;
            rmsQueues[channel].pop();
        }
    //!!!!!DODAWAĆ WARTOŚCI DO QUEUE JAK JEST ZA MAŁO!!!!!????

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
            //rmsMemoryBuffer[channel].pop_front();
            //rmsMemoryBuffer[channel + 2].pop_front();
            rmsMonoBuffers[channel].pop_front();
        }
        for (int channel = 0; channel < totalNumInputChannels; channel++)
        {
            //rmsMemoryBuffers[channel].pop_front();
            //rmsValueToRemove = rmsQueues[channel].front();
            //rmsSquareSum[channel] -= rmsValueToRemove * rmsValueToRemove;
            //rmsQueues[channel].pop();
            rmsStereoBuffers[channel].pop_front();
        }
    }
    //for (int sample = 0; sample < samplesToProcess + rmsOffsetInSamples - smpNum; sample++)
    //{
    //    for (int channel = 0; channel < totalNumInputChannels; channel++)
    //    {
    //        rmsMemoryBuffer[channel].pop_front();
    //        rmsValueToRemove = rmsQueues[channel].front();
    //        rmsSquareSum -= rmsValueToRemove * rmsValueToRemove;
    //        rmsQueues[channel].pop();
    //    }
    //}
    // On the other hand, if samples count in memoryBuffer is smaller than sum of samples in buffer and currentLatencyInSamples
    // (this is the case if this is the first processBlock call with enough samples to start processing), then 0's are used to fill the beginning of the buffer.
    int startingSample = smpNum - mainInputBuffer[0].size() + currentLatencyInSamples;
    jassert(startingSample >= 0);
    for (int sample = 0; sample < startingSample; sample++)
        for (int channel = 0; channel < totalNumInputChannels; channel++)
            channelsData[channel][sample] = 0;
    
    //if (keySignalSwitched)
    //{
    //    keySignalSwitched = false;
    //    int channelOffset = sidechainEnable ? 2 : 0;
    //    int channel;
    //    for (int keySignalChannel = 0; keySignalChannel < 2; keySignalChannel++)
    //    {
    //        rmsQueues[keySignalChannel] = std::queue<float>();
    //        rmsQueues[keySignalChannel].push(0);
    //        channel = keySignalChannel + channelOffset;
    //        for (int keySignalSample = -rmsOffset; keySignalSample < rmsOffset; keySignalSample++)
    //        {
    //            rmsQueues[keySignalChannel].push(channelsData[channel][std::abs(keySignalSample)]);
    //        }
    //    }
    //}

    //if (keySignalSwitched)
    //{
    //    keySignalSwitched = false;
    //    int channelOffset = sidechainEnable ? 2 : 0;
    //    int channel;
    //    for (int keySignalChannel = 0; keySignalChannel < 2; keySignalChannel++)
    //    {
    //        rmsQueues[keySignalChannel] = std::queue<float>();
    //        channel = keySignalChannel + channelOffset;
    //        for (int sample = samplesToProcess - 1; sample < rmsWindowSamples; sample++)
    //            rmsQueues[keySignalChannel].push(0);
    //        for (int sample = 0; sample < samplesToProcess; sample++)
    //            rmsQueues[keySignalChannel].push(memoryBuffer[keySignalChannel + channelOffset].front());
    //}

    //// If sidechain was enabled/disabled since the previous call to processBlock, rms queues are replaced with 0's, since values inside them are no longer relevant.
    //if (keySignalSwitched)
    //{
    //    keySignalSwitched = false;
    //    rmsSquareSum = 0;
    //    for (int channel = 0; channel < 2; channel++)
    //    {
    //        rmsQueues[channel] = std::queue<float>();
    //        for (int sample = 0; sample < rmsWindowSamples; sample++)
    //            rmsQueues[channel].push(0);
    //    }
    //}

    for (int smp = startingSample; smp < smpNum; ++smp)
    {
        //for (int channel = 0; channel < totalNumInputChannels; ++channel)
        //{
        //    currentValues[channel].push_back(channelsData[channel][smp]);
        //    //currentValuesOld[channel] = channelsData[channel][smp];
        //}
        for (int channel = 0; channel < 2; ++channel)
        {
            //float rmsValueToRemove, rmsValueToAdd;
            //rmsValueToRemove = rmsQueues[channel].front();
            //rmsSquareSum -= rmsValueToRemove * rmsValueToRemove;
            //rmsQueues[channel].pop();

            //channelsData[channel + 2][smp] *= sidechainGainLin;
            s[channel] = mainInputBuffer[channel].front();
            //s[channel] = mainInputBuffer[channel].front();
            sdb[channel] = Comp4AmplitudeToDecibels(s[channel]);
            //int rmsNewValueIndex = smp + rmsOffsetInSamples;
            //sdbkey = sidechainEnable ? Comp4AmplitudeToDecibels(channelsData[channel + 2][smp]) : sdb;

            //if (sidechainEnable)
            //{
            //    //sdbkey = Comp4AmplitudeToDecibels(memoryBuffer[channel + 2].front());
            //    //int tmp = smp + rmsOffset;
            //    //rmsQueues[channel].push(memoryBuffer[channel + 2][tmp]);
            //    //rmsValueToAdd = memoryBuffer[channel + 2][rmsNewValueIndex];
            //    if (sidechainChannelsExist) rmsValueToAdd = mainInputBuffer[channel + 2][rmsOffsetInSamples];
            //    else rmsValueToAdd = 0;
            //    // rmsQueues[channel].push(rmsValueToAdd);
            //    // if (smp + rmsOffset < smpNum) rmsQueues[channel].push(memoryBuffer[channel + 2][smp + rmsOffset]);
            //    // else rmsQueues[channel].push(memoryBuffer[channel + 2][smpNum * 2 - smp - rmsOffset]);
            //}
            //else
            //{
            //    //sdbkey = sdb;
            //    //rmsValueToAdd = memoryBuffer[channel][rmsNewValueIndex];
            //    rmsValueToAdd = mainInputBuffer[channel][rmsOffsetInSamples];
            //    // rmsQueues[channel].push(rmsValueToAdd);
            //    // if (smp + rmsOffset < smpNum) rmsQueues[channel].push(memoryBuffer[channel][smp + rmsOffset]);
            //    // else rmsQueues[channel].push(memoryBuffer[channel][smpNum * 2 - smp - rmsOffset]);
            //}

            //rmsQueues[channel].push(rmsValueToAdd);
            //rmsSquareSum += rmsValueToAdd * rmsValueToAdd;

            //double sabs = sidechainEnable ? std::abs(channelsData[channel+2][smp]) : std::abs(*s);
            //double newValuedb = 0;
            //originalValues[channel] = s;
            //if (smp % 100 == 0) DBG(attackSamplesLeft);

            //sdbkeyrms = Comp4AmplitudeToDecibels(std::sqrt(rmsSquareSum / rmsWindowSamples / 2.0));
            sdbkeyrms = Comp4AmplitudeToDecibels(rmsMonoBuffers[sidechainEnable && sidechainChannelsExist].front());
            //if (!sidechainEnable || !sidechainChannelsExist) sdbkeyrms = Comp4AmplitudeToDecibels((rmsBuffer[0].front() + rmsBuffer[1].front() / 2.0));
            //else sdbkeyrms = Comp4AmplitudeToDecibels((rmsBuffer[2].front() + rmsBuffer[3].front() / 2.0));
        }
        //sdbmean = Comp4AmplitudeToDecibels(std::sqrt(memoryBuffer[0].front() * memoryBuffer[0].front() + memoryBuffer[1].front() * memoryBuffer[1].front()));

        for (int input = 0; input < 2; input++)
            rmsMonoBuffers[input].pop_front();

        //if (ratio != 1.0 && thresh != 0.0 && !std::isinf(sdbmean))
        if (ratio != 1.0 && thresh != 0.0)
        {
            if (!downward)
            {
                if (sdbkeyrms >= thresh + knee / 2.0)
                {
                    //gainReduction = sdbmean - (thresh + (sdbmean - thresh) / ratio);
                    gainReduction = sdbkeyrms - (thresh + (sdbkeyrms - thresh) / ratio);
                    attackPhase = true;

                }
                else if (sdbkeyrms > thresh - knee / 2.0)
                {
                    Comp4UpdateBezier(sdbkeyrms);
                    //gainReduction = sdbmean - (bezierThresh + (sdbmean - bezierThresh) / bezierRatio);
                    gainReduction = sdbkeyrms - (bezierThresh + (sdbkeyrms - bezierThresh) / bezierRatio);
                    attackPhase = true;
                }
                else
                {
                    gainReduction = 0;
                    attackPhase = false;
                }

                // if (gainReduction - previousGainReduction > maxAttackGainPerSample)
                // {
                //     gainReduction = previousGainReduction + maxAttackGainPerSample;
                //     holdSamplesLeft = holdSamples;
                // }
                // else if (gainReduction - previousGainReduction < 0 && holdSamplesLeft > 0)
                // {
                //     gainReduction = previousGainReduction;
                //     holdSamplesLeft--;
                // }
                // else if (gainReduction - previousGainReduction < maxReleaseGainPerSample * -1.0)
                // {
                //     //gainReduction = -1.0 * previousGainReduction[channel] - maxReleaseGainPerSample;
                //     gainReduction = previousGainReduction - maxReleaseGainPerSample;
                // }
            }
            else
            {
                if (sdbkeyrms <= thresh - knee / 2.0)
                {
                    //gainReduction = sdbmean - (thresh - (thresh - sdbmean) / ratio);
                    gainReduction = sdbkeyrms - (thresh - (thresh - sdbkeyrms) / ratio);
                    attackPhase = true;
                }
                else if (sdbkeyrms < thresh + knee / 2.0)
                {
                    Comp4UpdateBezier(sdbkeyrms);
                    //gainReduction = sdbmean - (bezierThresh - (bezierThresh - sdbmean) / bezierRatio);
                    gainReduction = sdbkeyrms - (bezierThresh - (bezierThresh - sdbkeyrms) / bezierRatio);
                    attackPhase = true;
                }
                else
                {
                    gainReduction = 0;
                    attackPhase = false;
                }

                // if (previousGainReduction - gainReduction > maxAttackGainPerSample)
                // {
                //     gainReduction = previousGainReduction - maxAttackGainPerSample;
                //     holdSamplesLeft = holdSamples;
                // }
                // else if (previousGainReduction - gainReduction < 0 && holdSamplesLeft > 0)
                // {
                //     gainReduction = previousGainReduction;
                //     holdSamplesLeft--;
                // }
                // else if (previousGainReduction - gainReduction < maxReleaseGainPerSample * -1.0)
                // {
                //     gainReduction = previousGainReduction + maxReleaseGainPerSample;
                // }
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
        //jassert(gainReduction > -20);
        previousGainReduction = gainReduction;

        //s = s >= 0 ? Comp4DecibelsToAmplitude(currentThresh + (sdb - currentThresh) / currentRatio) :
        //    -1.0 * Comp4DecibelsToAmplitude(currentThresh + (sdb - currentThresh) / currentRatio);
        //s = s >= 0 ? Comp4DecibelsToAmplitude(sdb - gainReduction) : -1.0 * Comp4DecibelsToAmplitude(sdb - gainReduction);
        //previousSamplePositive[channel] = s >= 0;
        //previousSampledB[channel] = sdb - gainReduction;
        //s = Comp4DecibelsToAmplitude(previousSampledB[channel]);
        //if (!previousSamplePositive) s *= -1.0;

        // if (ratio != 1.0)
        // {
        //     if (!downward)
        //     {
        //         if (sdbkeyrms >= thresh + knee / 2.0)
        //         {
        //             if (compressionReleased) Comp4UnreleaseCompression();
        //             if (!compressionEngaged) Comp4EngageCompression();
        //             currentThresh = thresh;
        //             if (attackSamplesLeft > 0)
        //             {
        //                 // attackb = (attackSamples * ratio + previousRatio) / (ratio - previousRatio);
        //                 // attacka = -1.0 * ratio * (attackb + 1.0);
        //                 attacka = (attackSamples * ratio + 1.0) / (ratio - 1.0);
        //                 attackb = -1.0 * attacka - 1;
        //                 currentRatio = attacka / (attackSamples - attackSamplesLeft - attackb);
        //                 attackSamplesLeft--;
        //             }
        //             else currentRatio = ratio;
        // 
        //         }
        //         else if (sdbkeyrms > thresh - knee / 2.0)
        //         {
        //             if (compressionReleased) Comp4UnreleaseCompression();
        //             if (!compressionEngaged) Comp4EngageCompression();
        //             Comp4UpdateBezier(sdbkeyrms);
        //             currentThresh = bezierThresh;
        //             if (attackSamplesLeft > 0)
        //             {
        //                 // attackb = (attackSamples * bezierRatio + previousRatio) / (bezierRatio - previousRatio);
        //                 // attacka = -1.0 * bezierRatio * (attackb + 1.0);
        //                 attacka = (attackSamples * bezierRatio + 1.0) / (bezierRatio - 1.0);
        //                 attackb = -1.0 * attacka - 1;
        //                 currentRatio = attacka / (attackSamples - attackSamplesLeft - attackb);
        //                 attackSamplesLeft--;
        //             }
        //             else currentRatio = bezierRatio;
        //         }
        //         else //currentRatio = 1.0;
        //         {
        //             if (compressionEngaged)
        //             {
        //                 compressionReleased = true;
        //                 attackSamplesLeft = attackSamples;
        //                 if (holdSamplesLeft > 0) holdSamplesLeft--;
        //                 else if (releaseSamplesLeft > 0)
        //                 {
        //                     // releaseb = (releaseSamples * ratio + previousRatio) / (ratio - previousRatio);
        //                     // releasea = -1.0 * ratio * (attackb + 1.0);
        //                     releasea = (releaseSamples * ratio + 1.0) / (ratio - 1.0);
        //                     releaseb = -1.0 * releasea - 1;
        //                     currentRatio = releasea / (releaseSamplesLeft - releaseb);
        //                     releaseSamplesLeft--;
        //                 }
        //                 else Comp4DisengageCompression();
        //             }
        //             else currentRatio = 1.0;
        //             //!!!!!ZAMIAST UŻYWAĆ FUNKCJI DO WYLICZANIA ODPOWIEDNIEGO RATIO PO PROSTU LICZYĆ LINIOWO DOCELOWĄ REDUKCJĘ I SKALOWAĆ PRZEZ UPŁYNIĘTE PRÓBKI?!!!!!!!!!!
        //         }
        //     }
        //     else
        //     {
        //         if (sdbkeyrms <= thresh - knee / 2.0)
        //         {
        //             currentThresh = thresh;
        //             if (!compressionEngaged) Comp4EngageCompression();
        //             if (attackSamplesLeft > 0)
        //             {
        //                 attacka = (ratio * attackSamples - ratio) / (1 - ratio);
        //                 currentRatio = attacka / (attackSamples - attackSamplesLeft + attacka);
        //                 attackSamplesLeft--;
        //             }
        //             else currentRatio = ratio;
        //         }
        //         //else if (sdbkey < thresh + knee / 2.0)
        //         else if (sdbkeyrms < thresh + knee / 2.0)
        //         {
        //             currentThresh = bezierThresh;
        //             if (!compressionEngaged) Comp4EngageCompression();
        //             if (attackSamplesLeft > 0)
        //             {
        //                 Comp4UpdateBezier(sdb);
        //                 //Comp4UpdateBezier(sdbkey);
        //                 attacka = (bezierRatio * attackSamples - bezierRatio) / (1 - bezierRatio);
        //                 currentRatio = attacka / (attackSamples - attackSamplesLeft + attacka);
        //                 attackSamplesLeft--;
        //             }
        //             else currentRatio = bezierRatio;
        //         }
        //         else //currentRatio = 1.0;
        //         {
        //             if (compressionEngaged) Comp4DisengageCompression();
        //             if (holdSamplesLeft > 0) holdSamplesLeft--;
        //             else if (releaseSamplesLeft > 0)
        //             {
        //                 currentRatio = releasea / (releaseSamplesLeft + releasea);
        //                 releaseSamplesLeft--;
        //             }
        //             else currentRatio = 1.0;
        //         }
        //     }
        // }
        // 
        // s = s >= 0 ? Comp4DecibelsToAmplitude(currentThresh + (sdb - currentThresh) / currentRatio) :
        //     -1.0 * Comp4DecibelsToAmplitude(currentThresh + (sdb - currentThresh) / currentRatio);

        // if (windowOpen)
        // {
        //     currentValues[channel].push_back(originalValues[channel]);
        //     currentValues[channel + 2].push_back(channelsData[channel + 2][smp]);
        //     currentValues[channel + 4].push_back(channelsData[channel][smp]);
        // }
        for (int channel = 0; channel < 2; ++channel)
        {
            //if (lookAhead > 0)
            //    s[channel] = s[channel] >= 0 ? Comp4DecibelsToAmplitude(std::max(sdb[channel] - gainReduction), lookAheadValues[channel].f) : -1.0 * Comp4DecibelsToAmplitude(sdb[channel] - gainReduction);
            //else
            s[channel] = s[channel] >= 0 ? Comp4DecibelsToAmplitude(sdb[channel] - gainReduction) : -1.0 * Comp4DecibelsToAmplitude(sdb[channel] - gainReduction);
            jassert(!std::isnan(s[channel]));
            if (pluginWindowOpen)
            {
                //currentValues[channel].push_back(originalValues[channel]);
                //jassert(memoryBuffer[channel].front() < 0.1);
                //currentValues[channel].push_back(mainInputBuffer[channel].front());
                //if (sidechainChannelsExist) currentValues[channel + 2].push_back(mainInputBuffer[channel + 2].front());
                //currentValues[channel + 4].push_back(s[channel]);
                //currentValues[channel].push_back(mainInputBuffer[channel].front());
                //if (sidechainChannelsExist) currentValues[channel + 2].push_back(mainInputBuffer[channel + 2].front());
                currentValues[channel].push_back(rmsStereoBuffers[channel].front());
                if (sidechainChannelsExist) currentValues[channel + 2].push_back(rmsStereoBuffers[channel + 2].front());
                currentValues[channel + 4].push_back(s[channel]);
            }

            //if (smp % 100 == 0) DBG(currentValues[channel][smp] - originalValues[channel]);
            //if (smp % 100 == 0) DBG(channel);

            //currentValues[channel + 6].push_back(channelsData[channel][smp] - originalValues[channel]);
            //if (sidechainMuteInput) channelsData[channel][smp] = 0;
            //if (sidechainListen) channelsData[channel][smp] += channelsData[channel + 2][smp];

            //currentValuesOld[channel] = originalValues[channel];
            //currentValuesOld[channel + 2] = channelsData[channel + 2][smp];
            //currentValuesOld[channel + 4] = channelsData[channel][smp];
            //currentValuesOld[channel + 6] = std::abs(currentValuesOld[channel + 4]) - std::abs(currentValuesOld[channel]);
            // if (sidechainMuteInput) channelsData[channel][smp] = 0;
            // if (sidechainListen) channelsData[channel][smp] += channelsData[channel + 2][smp];
            // if (sidechainMuteInput) memoryBuffer[channel].front() = 0;
            // if (sidechainListen) memoryBuffer[channel].front() += memoryBuffer[channel + 2].front();

            // if (!sidechainMuteInput && !sidechainListen) channelsData[channel][smp] = memoryBuffer[channel].front();
            // else if (sidechainMuteInput && !sidechainListen) channelsData[channel][smp] = 0;
            // else if (!sidechainMuteInput && sidechainListen) channelsData[channel][smp] = memoryBuffer[channel].front() + memoryBuffer[channel + 2].front();
            // else channelsData[channel][smp] = memoryBuffer[channel + 2].front();

            //channelsData[channel][smp] = memoryBuffer[channel].front();
            //channelsData[channel][smp] = !sidechainMuteInput * memoryBuffer[channel].front() + sidechainListen * memoryBuffer[channel + 2].front();
            if (sidechainChannelsExist)
            {
                channelsData[channel][smp] = !sidechainMuteInput * s[channel] + sidechainListen * mainInputBuffer[channel + 2].front();
                //mainInputBuffer[channel].pop_front();
            }
            else
            {
                channelsData[channel][smp] = !sidechainMuteInput * s[channel];
                //mainInputBuffer[channel].pop_front();
            }
            mainInputBuffer[channel].pop_front();

            //*s *= outputGainLin;
        }
        for (int channel = 0; channel < totalNumInputChannels; channel++)
        {
            rmsStereoBuffers[channel].pop_front();
        }
    }

    /* for (int smp = 0; smp < smpNum; ++smp)
    {
        //for (int channel = 0; channel < totalNumInputChannels; ++channel)
        //{
        //    currentValues[channel].push_back(channelsData[channel][smp]);
        //    //currentValuesOld[channel] = channelsData[channel][smp];
        //}
        for (int channel = 0; channel < 2; ++channel)
        {
            rmsQueues[channel].pop();
            //channelsData[channel + 2][smp] *= sidechainGainLin;
            s = &channelsData[channel][smp];
            sdb = Comp4AmplitudeToDecibels(*s);
            //sdbkey = sidechainEnable ? Comp4AmplitudeToDecibels(channelsData[channel + 2][smp]) : sdb;
            if (sidechainEnable)
            {
                sdbkey = Comp4AmplitudeToDecibels(channelsData[channel + 2][smp]);
                if (smp + rmsOffset < smpNum) rmsQueues[channel].push(channelsData[channel + 2][smp + rmsOffset]);
                else rmsQueues[channel].push(channelsData[channel + 2][smpNum * 2 - smp - rmsOffset]);
            }
            else
            {
                sdbkey = sdb;
                if (smp + rmsOffset < smpNum) rmsQueues[channel].push(channelsData[channel][smp + rmsOffset]);
                else rmsQueues[channel].push(channelsData[channel][smpNum * 2 - smp - rmsOffset]);
            }
            //double sabs = sidechainEnable ? std::abs(channelsData[channel+2][smp]) : std::abs(*s);
            //double newValuedb = 0;
            originalValues[channel] = *s;
            //if (smp % 100 == 0) DBG(attackSamplesLeft);


            if (ratio != 1.0)
            {
                if (!downward)
                {
                    if (sdbkey >= thresh + knee / 2.0)
                    {
                        currentThresh = thresh;
                        if (!compressionEngaged) Comp4EngageCompression();
                        if (attackSamplesLeft > 0)
                        {
                            attacka = (ratio * attackSamples - ratio) / (1.0 - ratio);
                            currentRatio = attacka / (attackSamples - attackSamplesLeft + attacka);
                            attackSamplesLeft--;
                        }
                        else currentRatio = ratio;

                    }
                    else if (sdbkey > thresh - knee / 2.0)
                    {
                        //newValuedb = Comp4GetBezierdb(sdb);
                        currentThresh = bezierThresh;
                        if (!compressionEngaged) Comp4EngageCompression();
                        if (attackSamplesLeft > 0)
                        {
                            Comp4UpdateBezier(sdb);
                            //Comp4UpdateBezier(sdbkey);
                            attacka = (bezierRatio * attackSamples - bezierRatio) / (1.0 - bezierRatio);
                            currentRatio = attacka / (attackSamples - attackSamplesLeft + attacka);
                            attackSamplesLeft--;
                        }
                        else currentRatio = bezierRatio;
                    }
                    else //currentRatio = 1.0;
                    {
                        if (compressionEngaged)
                        {
                            if (holdSamplesLeft > 0) holdSamplesLeft--;
                            else if (releaseSamplesLeft > 0)
                            {
                                currentRatio = releasea / (releaseSamplesLeft + releasea);
                                releaseSamplesLeft--;
                            }
                            else Comp4DisengageCompression();
                        }
                        else currentRatio = 1.0;
                    }
                }
                else
                {
                    if (sdbkey <= thresh - knee / 2.0)
                    {
                        currentThresh = thresh;
                        //newValuedb = Comp4GetRatioddb(sdb);
                        if (!compressionEngaged) Comp4EngageCompression();
                        if (attackSamplesLeft > 0)
                        {
                            attacka = (ratio * attackSamples - ratio) / (1 - ratio);
                            currentRatio = attacka / (attackSamples - attackSamplesLeft + attacka);
                            attackSamplesLeft--;
                        }
                        else currentRatio = ratio;
                    }
                    else if (sdbkey < thresh + knee / 2.0)
                    {
                        currentThresh = bezierThresh;
                        if (!compressionEngaged) Comp4EngageCompression();
                        if (attackSamplesLeft > 0)
                        {
                            Comp4UpdateBezier(sdb);
                            //Comp4UpdateBezier(sdbkey);
                            attacka = (bezierRatio * attackSamples - bezierRatio) / (1 - bezierRatio);
                            currentRatio = attacka / (attackSamples - attackSamplesLeft + attacka);
                            attackSamplesLeft--;
                        }
                        else currentRatio = bezierRatio;
                    }
                    else //currentRatio = 1.0;
                    {
                        if (compressionEngaged) Comp4DisengageCompression();
                        if (holdSamplesLeft > 0) holdSamplesLeft--;
                        else if (releaseSamplesLeft > 0)
                        {
                            currentRatio = releasea / (releaseSamplesLeft + releasea);
                            releaseSamplesLeft--;
                        }
                        else currentRatio = 1.0;
                    }
                }
            }

            *s = *s >= 0 ? Comp4DecibelsToAmplitude(currentThresh + (sdb - currentThresh) / currentRatio) :
                -1.0 * Comp4DecibelsToAmplitude(currentThresh + (sdb - currentThresh) / currentRatio);

            if (windowOpen)
            {
                currentValues[channel].push_back(originalValues[channel]);
                currentValues[channel + 2].push_back(channelsData[channel + 2][smp]);
                currentValues[channel + 4].push_back(channelsData[channel][smp]);
            }

            //if (smp % 100 == 0) DBG(currentValues[channel][smp] - originalValues[channel]);
            //if (smp % 100 == 0) DBG(channel);

            //currentValues[channel + 6].push_back(channelsData[channel][smp] - originalValues[channel]);
            //if (sidechainMuteInput) channelsData[channel][smp] = 0;
            //if (sidechainListen) channelsData[channel][smp] += channelsData[channel + 2][smp];

            //currentValuesOld[channel] = originalValues[channel];
            //currentValuesOld[channel + 2] = channelsData[channel + 2][smp];
            //currentValuesOld[channel + 4] = channelsData[channel][smp];
            //currentValuesOld[channel + 6] = std::abs(currentValuesOld[channel + 4]) - std::abs(currentValuesOld[channel]);
            if (sidechainMuteInput) channelsData[channel][smp] = 0;
            if (sidechainListen) channelsData[channel][smp] += channelsData[channel + 2][smp];

            //*s *= outputGainLin;
        }
    } */

    if (previousOutputGain != outputGain || outputGain != 0)
    {
        buffer.applyGainRamp(0, 0, buffer.getNumSamples(), previousOutputGain, outputGain);
        buffer.applyGainRamp(1, 0, buffer.getNumSamples(), previousOutputGain, outputGain);
        previousOutputGain = outputGain;
    }

    //*hpf.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(getSampleRate(), sidechainHP);
    //*lpf.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), sidechainLP);
    ////int bufferSamples = buffer.getNumSamples();
    //hpf.process(buffer.getWritePointer(2));
    //hpf.process(buffer.getWritePointer(3));
    //lpf.process(buffer.getWritePointer(2));
    //lpf.process(buffer.getWritePointer(3));
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
    debugCurrentFunctionIndexProcessor = 4;
    return std::pow(10.0, decibelValue / 20.0);
}

double Comp4AudioProcessor::Comp4GetBezier(float input)
{
    debugCurrentFunctionIndexProcessor = 5;
    double t = (thresh - knee - Comp4AmplitudeToDecibels(input)) / (-2.0 * knee);
    double y1, y2, y3, y;
    y2 = thresh;
    if (!downward)
    {
        y1 = thresh - knee;
        y3 = y2 + knee / ratio;
    }
    else
    {
        y3 = thresh + knee;
        y1 = y2 - knee / ratio;
    }
    y = (1 - t) * (1 - t) * y1 + 2 * (1 - t) * t * y2 + t * t * y3;

    if (input >= 0.0f) return std::pow(10.0, y / 20.0);
    return -1.0 * std::pow(10.0, y / 20.0);
}

double Comp4AudioProcessor::Comp4GetRatiod(float input)
{
    debugCurrentFunctionIndexProcessor = 6;
    if (input >= 0.0f) return std::pow(10.0, (thresh + (Comp4AmplitudeToDecibels(input) - thresh) / ratio) / 20.0);
    return -1.0f * std::pow(10.0, (thresh + (Comp4AmplitudeToDecibels(input) - thresh) / ratio) / 20.0);
}

double Comp4AudioProcessor::Comp4GetBezierdb(double inputdb)
{
    debugCurrentFunctionIndexProcessor = 7;
    double x1 = thresh - knee;
    double x2 = thresh;
    double x3 = thresh + knee;
    double t = (thresh - knee - inputdb) / (-2.0 * knee);
    double y1, y2, y3, y;
    y2 = thresh;
    if (!downward)
    {
        y1 = thresh - knee;
        y3 = y2 + knee / ratio;
    }
    else
    {
        y3 = thresh + knee;
        y1 = y2 - knee / ratio;
    }

    y = (1 - t) * (1 - t) * y1 + 2 * (1 - t) * t * y2 + t * t * y3;
    double xprim = 2.0 * (1.0 - t) * (x2 - x1) + 2.0 * t * (x3 - x2);
    double yprim = 2.0 * (1.0 - t) * (y2 - y1) + 2.0 * t * (y3 - y2);
    bezierRatio = yprim / xprim;
    double bezierb = y - bezierRatio * inputdb;
    bezierThresh = (-1.0 * bezierb) / (bezierRatio - 1);


    return y;
}

double Comp4AudioProcessor::Comp4GetRatioddb(double inputdb)
{
    debugCurrentFunctionIndexProcessor = 8;
    return thresh + (inputdb - thresh) / ratio;
}

double Comp4AudioProcessor::Comp4AmplitudeToDecibels(float signal)
{
    debugCurrentFunctionIndexProcessor = 9;
    return 20.0 * std::log10(std::abs(signal));
}
//void Comp4AudioProcessor::Comp4EngageCompression()
//{
//    debugCurrentFunctionIndexProcessor = 10;
//    attackSamplesLeft = attackSamples;
//    compressionEngaged = true;
//    return;
//}
//
//void Comp4AudioProcessor::Comp4DisengageCompression()
//{
//    debugCurrentFunctionIndexProcessor = 11;
//    releaseSamplesLeft = releaseSamples;
//    holdSamplesLeft = holdSamples;
//    compressionEngaged = false;
//    return;
//}
//
//void Comp4AudioProcessor::Comp4UnreleaseCompression()
//{
//    releaseSamplesLeft = releaseSamples;
//    holdSamplesLeft = holdSamples;
//    compressionReleased = false;
//    return;
//}

void Comp4AudioProcessor::Comp4UpdateBezier(double inputdb)
{
    debugCurrentFunctionIndexProcessor = 12;
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
void Comp4AudioProcessor::clear(std::queue<int>& q)
{
    std::queue<int> empty;
    std::swap(q, empty);
}
void Comp4AudioProcessor::Comp4UpdateLatencySteps()
{
    for (int i = 0; i < 7; i++) latencyStepsInSamples[i] = std::ceil(sampleRate * latencyStepsInMs[i] * 0.001);
    Comp4UpdateLatencyCore();
}
//void Comp4AudioProcessor::Comp4UpdateLatency(double newMinimalLatencyInMs)
//{
//    if (newMinimalLatencyInMs == minimalLatencyInMs) return;
//    minimalLatencyInMs = newMinimalLatencyInMs;
//    Comp4UpdateLatencyCore();
//}
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
//std::vector<float> Comp4GetLookAhead(std::deque<float> memoryBuffer[], int lookAheadSamples, double maxAttackGainPerSample)
//{
//
//}
void Comp4AudioProcessor::Comp4GetLookAhead()
{
    jassert(lookAheadSamples <= mainInputBuffer[0].size());
    double lookRmsSquareSum = 0;
    int channel = 0;
    double prevValue = -std::numeric_limits<double>::infinity();
    double currentValue = 0;
    if (sidechainEnable) channel = 2;
    if (lookAheadValues.empty())
    {
        for (int i = lookAheadSamples - 1; i > lookAheadSamples - 1 - rmsOffsetInSamples; i--)
        {
            //lookRmsSquareSum += memoryBuffer[channel][lookAheadSamples - i] * memoryBuffer[channel][lookAheadSamples - i] + memoryBuffer[channel + 1][lookAheadSamples - i] * memoryBuffer[channel + 1][lookAheadSamples - i];
            lookRmsSquareSum += mainInputBuffer[channel][i] * mainInputBuffer[channel][i] + mainInputBuffer[channel + 1][i] * mainInputBuffer[channel + 1][i];
        }
        //for (int i = 0; i < lookAheadSamples; i++)
        for (int i = lookAheadSamples - 1; i >= 0; i--)
        {
            lookRmsSquareSum += mainInputBuffer[channel][i] * mainInputBuffer[channel][i] + mainInputBuffer[channel + 1][i] * mainInputBuffer[channel + 1][i];
            //if (i == lookAheadSamples - 1) lookAheadValues.push_back(std::sqrt(lookRmsSquareSum));
            //else lookAheadValues.push_back(std::max(std::sqrt(lookRmsSquareSum), 
            //lookAheadValues.push_back(std::max(prevValue - maxAttackGainPerSample, std::sqrt(lookRmsSquareSum / rmsWindowSamples / 2.0)));
            currentValue = Comp4AmplitudeToDecibels(std::sqrt(lookRmsSquareSum / rmsWindowSamples / 2.0));
            //if (prevValue - maxAttackGainPerSample > currentValue) currentValue = prevValue - maxAttackGainPerSample;
            //currentValue = std::max(currentValue, prevValue - maxAttackGainPerSample);
            if (prevValue - maxAttackGainPerSample > currentValue)
            {
                lookAheadValues.push_back(maxAttackGainPerSample);
                currentValue = prevValue - maxAttackGainPerSample;
            }
            else lookAheadValues.push_back(0);
            prevValue = currentValue;
            //PRZEANALIZOWAĆ DLA DOWNWARD I EXPANDERÓW
        }
    }
    else
    {

    }
    //PRZEANALIZOWAĆ DLA ZMIENIAJĄCEGO SIĘ OPÓŹNIENIA
}
