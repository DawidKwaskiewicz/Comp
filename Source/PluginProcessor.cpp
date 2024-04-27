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
    sidechainInputGain = 0.0;
    sidechainHP = 20.0;
    sidechainLP = 20000.0;
    downward = false;
    sidechainEnable = false;
    sidechainListen = false;
    sidechainMuteInput = false;
    //previousInputGain = 0.0;
    //previousSidechainGain = 0.0;
    double sr = 44100.0;
    attackSamples = attack * sr * 0.001;
    releaseSamples = release * sr * 0.001;
    holdSamples = hold * sr * 0.001;
    lookAheadSamples = lookAhead * sr * 0.001;
    rmsWindowSamples = rmsWindowLength * sr * 0.001;
    attackSamplesLeft = attackSamples;
    releaseSamplesLeft = releaseSamples;
    holdSamplesLeft = holdSamples;
    lookAheadSamplesLeft = lookAheadSamples;
    rmsWindowSamplesLeft = rmsWindowSamples;
    compressionEngaged = false;
    prevValue = 0.0f;
    bezierRatio = 1.0;
    bezierThresh = 0.0;
    currentRatio = 1.0;
    currentThresh = 0.0;
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
    double outputGainLin = Comp4DecibelsToAmplitude(outputGain);
    double sidechainGainLin = Comp4DecibelsToAmplitude(sidechainInputGain);
    attackSamples = std::round(attack * getSampleRate() * 0.001);
    releaseSamples = std::round(release * getSampleRate() * 0.001);
    holdSamples = std::round(hold * getSampleRate() * 0.001);
    lookAheadSamples = std::round(lookAhead * getSampleRate() * 0.001);
    releasea = (ratio * releaseSamples - ratio) / (1.0 - ratio);


    //juce::dsp::AudioBlock<float> blockl(buffer);
    juce::dsp::AudioBlock<float> block(buffer);
    block = block.getSubsetChannelBlock(2, 2);
    lpf.setCutoffFrequencyHz(sidechainLP);
    hpf.setCutoffFrequencyHz(sidechainHP);
    hpf.process(juce::dsp::ProcessContextReplacing<float>(block));
    lpf.process(juce::dsp::ProcessContextReplacing<float>(block));

    for (int smp = 0; smp < smpNum; ++smp)
    {
        //for (int channel = 0; channel < totalNumInputChannels; ++channel)
        //{
        //    currentValues[channel].push_back(channelsData[channel][smp]);
        //    //currentValuesOld[channel] = channelsData[channel][smp];
        //}
        for (int channel = 0; channel < 2; ++channel)
        {
            channelsData[channel + 2][smp] *= sidechainGainLin;
            s = &channelsData[channel][smp];
            sdb = Comp4AmplitudeToDecibels(*s);
            sdbkey = sidechainEnable ? Comp4AmplitudeToDecibels(channelsData[channel + 2][smp]) : sdb;
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

            *s *= outputGainLin;
        }
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
void Comp4AudioProcessor::Comp4EngageCompression()
{
    debugCurrentFunctionIndexProcessor = 10;
    attackSamplesLeft = attackSamples;
    compressionEngaged = true;
    return;
}

void Comp4AudioProcessor::Comp4DisengageCompression()
{
    debugCurrentFunctionIndexProcessor = 11;
    releaseSamplesLeft = releaseSamples;
    holdSamplesLeft = holdSamples;
    compressionEngaged = false;
    return;
}

void Comp4AudioProcessor::Comp4UpdateBezier(double inputdb)
{
    debugCurrentFunctionIndexProcessor = 12;
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
    return;
}
