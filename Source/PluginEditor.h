/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <string>
#include <vector>

//==============================================================================
/**
*/
class Comp4AudioProcessorEditor  : public juce::AudioProcessorEditor, private juce::Slider::Listener, private juce::Timer
{
public:
    Comp4AudioProcessorEditor (Comp4AudioProcessor&);
    ~Comp4AudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    void onStateSwitch();

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Comp4AudioProcessor& audioProcessor;

    juce::TooltipWindow tooltipWindow;
    juce::ToggleButton bUpward, bDownward;
    juce::TextButton bSidechainEnable, bSidechainMuteInput;
    juce::ImageButton bSidechainListen;
    //juce::Slider sRatio, sThresh, sKnee, sMUG;
    //juce::Slider sAttack, sRelease, sHold, sLookAhead;
    //juce::Slider sSidechainGain, sSidechainHP, sSidechainLP;
    //std::vector<juce::Slider*> sliders = { &sRatio, &sThresh, &sKnee, &sMUG,
    //    &sAttack, &sRelease, &sHold, &sLookAhead,
    //    &sSidechainGain, &sSidechainHP, &sSidechainLP };
    juce::OwnedArray<juce::Slider> sliders;
    //  OLD
    //0 - sRatio
    //1 - sThresh
    //2 - sKnee
    //3 - sMUG
    //4 - sAttack
    //5 - sRelease
    //6 - sHold
    //7 - sLookAhead
    //8 - sSidechainGain
    //9 - sSidechainHP
    //10 - sSidechainLP
    //  NEW
    //0 - sRatio
    //1 - sThresh
    //2 - sKnee
    //3 - sInput!!!
    //4 - sOutput!!!
    //5 - sAttack
    //6 - sRelease
    //7 - sHold
    //8 - sLookAhead
    //9 - sRmsWindowLength!!!
    //10 - sSidechainGain
    //11 - sSidechainHP
    //12 - sSidechainLP
    std::vector<std::string> names = { "Ratio", "Threshold", "Knee", "Input", "Output", "Attack", "Release", "Hold", "Look-ahead", "RMS", "Gain", "HPF", "LPF"};
    std::vector<std::string> codeNames = { "sRatio", "sThresh", "sKnee", "sInput", "sOutput", "sAttack", "sRelease", "sHold", "sLookAhead", "sRmsWindowLength", "sSidechainGain", "sSidechainHP", "sSidechainLP"};
    std::vector<std::string> tooltips = { "Ratio", "Threshold", "Knee", "Input gain", "Output gain", "Attack", "Release", "Hold", "Look-ahead", "RMS window length",
        "Sidechain input gain", "High-pass filter cutoff frequency", "Low-pass filter cutoff frequency"};
    std::vector<int> indicesX = { 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 5, 5, 5 };
    std::vector<int> indicesY = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 3, 4 };
    std::vector<double> mins = { 0.01, -60.0, 0.0, -24.0, -24.0, 0.0, 0.0, 0.0, 0.0, 0.0, -24.0, 20.0, 20.0 };
    std::vector<double> maxs = { 30.0, 0.0, 12.0, 24.0, 24.0, 500.0, 1000.0, 1000.0, 500.0, 100.0, 24.0, 20000.0, 20000.0 };
    std::vector<double> mids = { 1.0, -12.0, 3.0, 0.0, 0.0, 10.0, 50.0, 50.0, 10.0, 1.0, 0.0, 1000.0, 1000.0 };
    std::vector<double> starts = { 1.0, 0.0, 3.0, 0.0, 0.0, 10.0, 50.0, 0.0, 0.0, 1.0, 0.0, 20.0, 20000.0 };
    std::vector<double> steps = { 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.01, 0.001, 0.01, 1.0, 1.0};
    std::vector<std::string> suffixes = { "", " dB", " dB", " dB", " dB", " ms", " ms", " ms", " ms", " ms", " dB", " Hz", " Hz"};
    int mainSlidersCount = 10;
    //std::vector<juce::Label*> labels;
    //std::vector<juce::Label*> labelsGridX;
    //std::vector<juce::Label*> labelsGridY;
    juce::OwnedArray<juce::Label> labels;
    juce::OwnedArray<juce::Label> labelsGridX;
    juce::OwnedArray<juce::Label> labelsGridY;
    std::vector<std::string> gridSteps = { "-12", "-24", "-36", "-48" };
    float labelGridFont = 13.0f;

    //juce::Slider sInputL, sInputR, sSidechainL, sSidechainR, sOutputL, sOutputR, sGainL, sGainR;
    //std::vector<juce::Slider*> bars = { &sInputL, &sInputR, &sSidechainL, &sSidechainR, &sOutputL, &sOutputR, &sGainL, &sGainR };
    //std::vector<juce::Label*> barLabels;
    //std::vector<juce::Label*> bar1GridLabels;
    //std::vector<juce::Label*> bar2GridLabels;
    juce::OwnedArray<juce::Slider> bars;
    std::vector<std::string> barsCodeNames = { "sInputL", "sInputR", "sSidechainL", "sSidechainR", "sOutputL", "sOutputR", "sGainL", "sGainR" };
    //0 - sInputL
    //1 - sInputR
    //2 - sSidechainL
    //3 - sSidechainR
    //4 - sOutputL
    //5 - sOutputR
    //6 - sGainL
    //7 - sGainR
    juce::OwnedArray<juce::Label> barLabels;
    juce::OwnedArray<juce::Label> bar1GridLabels;
    juce::OwnedArray<juce::Label> bar2GridLabels;
    std::vector<std::string> bar1GridSteps = { "0", "-12", "-24", "-36", "-48", "-60" };
    //std::vector<std::string> bar2GridSteps = { "0", "-6", "-12", "-18", "-24", "-30" };
    //std::vector<std::string> bar2GridSteps = { "15", "9", "3", "-3", "-9", "-15" };
    std::vector<std::string> bar2GridSteps = { "30", "18", "6", "-6", "-18", "-30" };
    std::vector<int> barTypes = { 1, 1, 1, 1, 1, 1, 2, 2 };
    float barLabelGridFont = 11.0f;
    int barTimerIntervalms = 40;
    double barsPreviousValues[6];

    int callbackCounter = 0;
    std::vector<int> barCountDiff;
    int newSliderShiftTmp = 70;

    juce::Path compLine;


    juce::Colour cBackground = getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId);
    juce::Colour cSliderTextboxBackground = cBackground;
    juce::Colour cSliderTextboxText = juce::Colours::white;
    juce::Colour cLabelGridText = juce::Colours::slategrey;
    juce::Colour cOutlines = juce::Colours::white;
    juce::Colour cGraphBackground = juce::Colours::black;
    juce::Colour cGrid = juce::Colours::slategrey;
    juce::Colour cLabels = juce::Colours::white;
    juce::Colour cBarsGridLabels = juce::Colours::white;
    juce::Colour cBarsFill = juce::Colours::lightgrey;
    //juce::Colour cBarsBackground = juce::Colours::white;
    juce::Colour cThreshLine = juce::Colours::red;
    juce::Colour cCompLine = juce::Colours::white;

    void sliderValueChanged(juce::Slider* slider) override;
    void timerCallback() override;
    //void Comp4SetMainSlider(juce::Slider* slider, int indexX, int indexY, double min, double max, double mid, double start, double step, std::string suffix);
    void Comp4SetMainSlider(int index);
    void Comp4SetBar(juce::Slider* slider, int type);
    double Comp4SignaltoRMSdb(std::vector<double>& signal, int barIndex, bool peak);
    double Comp4Signaltodb(double signal);
    void Comp4RestoreParams();
    void Comp4ButtonOptics();
    double Comp4GetMaxdBValue(std::vector<double>& signal);

    //int debugCurrentFunctionIndex = 0;


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Comp4AudioProcessorEditor)
};
