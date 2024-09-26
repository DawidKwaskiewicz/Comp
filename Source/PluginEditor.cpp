/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "SliderLookAndFeel.h"
#include <JuceHeader.h>

//==============================================================================
Comp4AudioProcessorEditor::Comp4AudioProcessorEditor (Comp4AudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    audioProcessor.pluginWindowOpen = true;
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    Timer::startTimer(barTimerIntervalms);

    for (int i = 0; i < indicesX.size(); i++)
    {
        sliders.add(new juce::Slider(codeNames[i]));
        Comp4SetMainSlider(i);
        labels.add(new juce::Label({}, names[i]));
        labels[i]->setJustificationType(juce::Justification::centredBottom);
        if (i == 3 || i == 4 || i == 9 || i == 10 || i == 11 || i == 12)
            labels[i]->setTooltip(tooltips[i]);
        if (i > 9)
        {
            labels[i]->setBounds(335 + newSliderShiftTmp, 465 + newSliderShiftTmp + (indicesY[i] - 2) * 50, 55, 15);
        }
        else labels[i]->attachToComponent(sliders[i], false);
        this->addAndMakeVisible(labels[i]);
    }

    sliders[9]->textFromValueFunction = [](double value)
        {
            if (value == 0.0) return juce::String("Peak");
            return juce::String(value) + " ms";
        };
    sliders[9]->valueFromTextFunction = [](const juce::String& text)
        {
            if (text == "Peak") return 0.0;
            return text.getDoubleValue();
        };
    

    bUpward.setButtonText("Upward processing");
    bUpward.setToggleState(true, juce::sendNotification);
    addAndMakeVisible(&bUpward);
    bUpward.onClick = [this] {Comp4AudioProcessorEditor::onStateSwitch(); };
    bUpward.setRadioGroupId(1);

    bDownward.setButtonText("Downward processing");
    bDownward.setToggleState(false, juce::sendNotification);
    addAndMakeVisible(&bDownward);
    bDownward.onClick = [this] {Comp4AudioProcessorEditor::onStateSwitch(); };
    bDownward.setRadioGroupId(1);

    bSidechainEnable.setButtonText("Off");
    bSidechainEnable.setToggleState(false, juce::sendNotification);
    bSidechainEnable.setClickingTogglesState(true);
    bSidechainEnable.setColour(juce::TextButton::buttonOnColourId, cLabels);
    bSidechainEnable.setColour(juce::TextButton::textColourOnId, cBackground);
    bSidechainEnable.setTooltip("Enable sidechain");
    addAndMakeVisible(&bSidechainEnable);
    bSidechainEnable.onClick = [this] {Comp4AudioProcessorEditor::onStateSwitch(); };

    bSidechainMuteInput.setButtonText("M");
    bSidechainMuteInput.setToggleState(false, juce::sendNotification);
    bSidechainMuteInput.setClickingTogglesState(true);
    bSidechainMuteInput.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red);
    bSidechainMuteInput.setColour(juce::TextButton::textColourOnId, cBackground);
    bSidechainMuteInput.setTooltip("Mute main input signal");
    addAndMakeVisible(&bSidechainMuteInput);
    bSidechainMuteInput.onClick = [this] {Comp4AudioProcessorEditor::onStateSwitch(); };

    bSidechainListen.setToggleState(false, juce::sendNotification);
    bSidechainListen.setClickingTogglesState(true);
    bSidechainListen.setImages(false, true, false, juce::ImageCache::getFromMemory(BinaryData::Mute_Icon_svg_png, BinaryData::Mute_Icon_svg_pngSize),
        0.5f, juce::Colours::transparentBlack, juce::Image(), 0.5f, juce::Colours::transparentBlack, juce::Image(), 0.5f, juce::Colours::transparentBlack);
    bSidechainListen.setTooltip("Listen to sidechain input");
    addAndMakeVisible(&bSidechainListen);
    bSidechainListen.onClick = [this] {Comp4AudioProcessorEditor::onStateSwitch(); };


    for (int i = 0; i < barsCodeNames.size(); i++)
    {
        bars.add(new juce::Slider(barsCodeNames[i]));
        Comp4SetBar(bars[i], barTypes[i]);
    }
    barsPreviousValues[0] = -60.0;
    barsPreviousValues[1] = -60.0;
    barsPreviousValues[2] = -60.0;
    barsPreviousValues[3] = -60.0;
    barsPreviousValues[4] = -60.0;
    barsPreviousValues[5] = -60.0;

    Comp4RestoreParams();

    setSize(600 + newSliderShiftTmp, 600 + newSliderShiftTmp);
}

Comp4AudioProcessorEditor::~Comp4AudioProcessorEditor()
{
    Timer::stopTimer();
    audioProcessor.pluginWindowOpen = false;
}

//==============================================================================
void Comp4AudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // graph layout
    g.setColour(cGraphBackground);
    int graphSideLength = 470;
    g.fillRect(0, 0, graphSideLength, graphSideLength);

    // graph labels
    g.setColour(cGrid);
    for (int i = 1; i < 5; i++)
    {
        float tmpSide = 400 + newSliderShiftTmp;
        float tmpStep = tmpSide / 5.0f;
        g.drawLine(juce::Line<float>(juce::Point<float>(0, i * tmpStep), juce::Point<float>(tmpSide, i * tmpStep)), 1.0f);
        labelsGridX.add(new juce::Label({}, gridSteps[i - 1]));
        labelsGridX[i - 1]->setBounds(0, i * tmpStep - 15, 30, 10);
        labelsGridX[i - 1]->setColour(juce::Label::textColourId, cLabelGridText);
        labelsGridX[i - 1]->setFont(labelGridFont);
        this->addAndMakeVisible(labelsGridX[i - 1]);
        if (i != 4)
        {
            g.drawLine(juce::Line<float>(juce::Point<float>(i * tmpStep, 0), juce::Point<float>(i * tmpStep, tmpSide)), 1.0f);
            labelsGridY.add(new juce::Label({}, gridSteps[gridSteps.size() - i]));
            labelsGridY[i - 1]->setBounds(i * tmpStep - 29, tmpSide - 15, 30, 10);
            labelsGridY[i - 1]->setColour(juce::Label::textColourId, cLabelGridText);
            labelsGridY[i - 1]->setFont(labelGridFont);
            this->addAndMakeVisible(labelsGridY[i - 1]);
        }
        else
        {
            g.drawLine(juce::Line<float>(juce::Point<float>(i * tmpStep, 0), juce::Point<float>(i * tmpStep, tmpStep * 4 - 1)), 1.0f);
            labelsGridY.add(new juce::Label({}, gridSteps[gridSteps.size() - i]));
            labelsGridY[i - 1]->setBounds(i * tmpStep - 29, tmpStep * 4 - 15, 30, 10);
            labelsGridY[i - 1]->setColour(juce::Label::textColourId, cLabelGridText);
            labelsGridY[i - 1]->setFont(labelGridFont);
            this->addAndMakeVisible(labelsGridY[i - 1]);
        }
    }

    // threshold line
    g.setColour(cThreshLine);
    int threshLineHeight = std::round(-1.0 * sliders[1]->getValue() / 60.0 * graphSideLength);
    g.drawLine(juce::Line<float>(juce::Point<float>(0, threshLineHeight), juce::Point<float>(400 + newSliderShiftTmp, threshLineHeight)), 1.0f);

    // comp line
    g.saveState();
    g.setColour(cCompLine);
    compLine.clear();
    // x1, y1 - coordinates of the first point of the curve (on the bottom or left edge of the graph)
    // x2, y2 - point where the knee begins
    // x3, y3 - point where the knee ends
    // x4, y4 - the last point of the curve (on the top or right edge of the graph)
    float x1, y1, x2, y2, x3, y3, x4, y4;
    // In this instance, the thresh parameter is the amount of pixels from top of the window to the displayed threshold line.
    double thresh = -1.0 * sliders[1]->getValue() / 60.0 * (400.0 + newSliderShiftTmp);
    // In this instance, the knee parameter is translated form decibels to each side of the threshold where the knee is applied, to pixels.
    double knee = sliders[2]->getValue() / 60.0 * (400.0 + newSliderShiftTmp);
    double ratio = sliders[0]->getValue();
    x2 = 400 + newSliderShiftTmp - thresh - knee / 2;
    x3 = 400 + newSliderShiftTmp - thresh + knee / 2;

    // All coordinates are calculated from the y = ax + b formula, where the a parameter is given as a = 1/ratio for one segment (below the threshold for downward compression or above
    // the threshold for upward compression) and as a = 1 for the other segment. Both lines intersect at the threshold T (point [T, T] in carthesian coordinates). With this knowledge,
    // parameter b can be calculated, thus solving the formula.
    if (bDownward.getToggleState())
    {
        x1 = std::max(400.0 + newSliderShiftTmp - thresh - (400.0 + newSliderShiftTmp - thresh) * ratio, 0.0);
        y1 = std::min(thresh + (400.0 + newSliderShiftTmp - thresh) / ratio, 400.0 + newSliderShiftTmp);
        x4 = 400.0f + newSliderShiftTmp;
        y4 = 0.0f;
        y2 = 400 + newSliderShiftTmp - x2 / ratio - (400 + newSliderShiftTmp - thresh) * (1.0 - 1.0 / ratio);
        y3 = thresh - knee / 2.0;
    }
    else
    {
        x1 = 0.0f;
        y1 = 400.0f + newSliderShiftTmp;
        x4 = std::min(400 + newSliderShiftTmp - thresh + thresh * ratio, 400.0 + newSliderShiftTmp);
        y4 = std::max(thresh - thresh / ratio, 0.0);
        y2 = thresh + knee / 2.0;
        y3 = 400 + newSliderShiftTmp - x3 / ratio - (400 + newSliderShiftTmp - thresh) * (1.0 - 1.0 / ratio);
    }
    compLine.startNewSubPath(x1, y1);
    compLine.lineTo(x2, y2);
    if (!std::isnan(x3) && !isnan(y3))
        compLine.quadraticTo(400.0 + newSliderShiftTmp - thresh, thresh, x3, y3);
    compLine.lineTo(x4, y4);
    g.strokePath(compLine, juce::PathStrokeType(1.5f));

    g.restoreState();

    g.setColour(cBackground);
    g.fillRect(0, graphSideLength, graphSideLength, 200);
    g.fillRect(graphSideLength, 0, 200, 670);

    // bars area
    int xOffset = 2;
    int barGridLabelWidth = 25;
    int barGridLabelHeight = 10;
    double startY = 30.0;
    double endY = 580.0 + newSliderShiftTmp;
    for (int i = 0; i < bar1GridSteps.size(); i++)
    {
        bar1GridLabels.add(new juce::Label({}, bar1GridSteps[i]));
        bar1GridLabels[i]->setBounds(graphSideLength + xOffset, std::round(startY + i * (endY - startY) / (bar1GridSteps.size() - 1.0)), barGridLabelWidth, barGridLabelHeight);
        bar1GridLabels[i]->setColour(juce::Label::textColourId, cBarsGridLabels);
        bar1GridLabels[i]->setFont(barLabelGridFont);
        bar1GridLabels[i]->setJustificationType(juce::Justification::centredRight);
        this->addAndMakeVisible(bar1GridLabels[i]);

        bar2GridLabels.add(new juce::Label({}, bar2GridSteps[i]));
        bar2GridLabels[i]->setBounds(600 + newSliderShiftTmp - xOffset - barGridLabelWidth + 2, std::round(startY + i * (endY - startY) / (bar2GridSteps.size() - 1.0)), barGridLabelWidth, barGridLabelHeight);
        bar2GridLabels[i]->setColour(juce::Label::textColourId, cBarsGridLabels);
        bar2GridLabels[i]->setFont(barLabelGridFont);
        bar2GridLabels[i]->setJustificationType(juce::Justification::centredLeft);
        this->addAndMakeVisible(bar2GridLabels[i]);
    }

    g.setColour(cOutlines);
    g.drawFittedText("In", bars[0]->getX(), 10, 30, 20, juce::Justification::centred, 1);
    g.drawFittedText("Side", bars[2]->getX(), 10, 30, 20, juce::Justification::centred, 1);
    g.drawFittedText("Out", bars[4]->getX(), 10, 30, 20, juce::Justification::centred, 1);
    g.drawFittedText("Gain", bars[6]->getX(), 10, 30, 20, juce::Justification::centred, 1);

    // some labels
    g.setFont(15.0f);
    g.setColour(cLabels);
    g.drawFittedText("Sidechain bay", 280 + newSliderShiftTmp, 400 + newSliderShiftTmp, 120, 20, juce::Justification::centred, 1);


    // main layout, white
    g.setColour(cOutlines);
    g.drawLine(juce::Line<float>(juce::Point<float>(400 + newSliderShiftTmp, 0), juce::Point<float>(400 + newSliderShiftTmp, 600 + newSliderShiftTmp)), 2.0f);
    g.drawLine(juce::Line<float>(juce::Point<float>(0, 400 + newSliderShiftTmp), juce::Point<float>(400 + newSliderShiftTmp, 400 + newSliderShiftTmp)), 2.0f);
    g.drawLine(juce::Line<float>(juce::Point<float>(0, 500 + newSliderShiftTmp), juce::Point<float>(280 + newSliderShiftTmp, 500 + newSliderShiftTmp)), 2.0f);
    g.drawLine(juce::Line<float>(juce::Point<float>(280 + newSliderShiftTmp, 400 + newSliderShiftTmp), juce::Point<float>(280 + newSliderShiftTmp, 600 + newSliderShiftTmp)), 2.0f);
}

void Comp4AudioProcessorEditor::resized()
{
    for (int i = 0; i < mainSlidersCount; i++)
    {
        sliders[i]->setBounds(indicesX[i] * 70, 420 + newSliderShiftTmp + indicesY[i] * 100, 70, 75);
    }
    for (int i = mainSlidersCount; i < sliders.size(); i++)
    {
        sliders[i]->setBounds(290 + newSliderShiftTmp, 465 + newSliderShiftTmp + (indicesY[i]-2) * 50, 100, 45);
    }

    float tmpStep = (400 + newSliderShiftTmp) / 5.0f;
    bUpward.setBounds((int)std::roundf(tmpStep * 3.0f + 5), (int)std::roundf(tmpStep * 4.0f + 10), (int)std::roundf(tmpStep * 2.0f - 10), (int)std::roundf(tmpStep / 2.0f));
    bDownward.setBounds((int)std::roundf(tmpStep * 3.0f + 5), (int)std::roundf(tmpStep * 4.0f + 40), (int)std::roundf(tmpStep * 2.0f - 10), (int)std::roundf(tmpStep / 2.0f));

    int sbs = 34;
    bSidechainEnable.setBounds(286 + newSliderShiftTmp, 425 + newSliderShiftTmp, sbs, sbs);
    bSidechainListen.setBounds(323 + newSliderShiftTmp, 425 + newSliderShiftTmp, sbs, sbs);
    bSidechainMuteInput.setBounds(360 + newSliderShiftTmp, 425 + newSliderShiftTmp, sbs, sbs);

    int barWidth = 13;
    int smallBreak = 4;
    int bigBreak = 8;
    int biggerBreak = 12;
    int xOffset = 28;
    bars[0]->setBounds(400 + newSliderShiftTmp + xOffset, 30, barWidth, 560 + newSliderShiftTmp);
    bars[1]->setBounds(400 + newSliderShiftTmp + xOffset + barWidth + smallBreak, 30, barWidth, 560 + newSliderShiftTmp);
    bars[2]->setBounds(400 + newSliderShiftTmp + xOffset + barWidth * 2 + smallBreak + bigBreak, 30, barWidth, 560 + newSliderShiftTmp);
    bars[3]->setBounds(400 + newSliderShiftTmp + xOffset + barWidth * 3 + smallBreak * 2 + bigBreak, 30, barWidth, 560 + newSliderShiftTmp);
    bars[4]->setBounds(400 + newSliderShiftTmp + xOffset + barWidth * 4 + smallBreak * 2 + bigBreak * 2, 30, barWidth, 560 + newSliderShiftTmp);
    bars[5]->setBounds(400 + newSliderShiftTmp + xOffset + barWidth * 5 + smallBreak * 3 + bigBreak * 2, 30, barWidth, 560 + newSliderShiftTmp);
    bars[6]->setBounds(400 + newSliderShiftTmp + xOffset + barWidth * 6 + smallBreak * 3 + bigBreak * 2 + biggerBreak, 30, barWidth, 560 + newSliderShiftTmp);
    bars[7]->setBounds(400 + newSliderShiftTmp + xOffset + barWidth * 7 + smallBreak * 4 + bigBreak * 2 + biggerBreak, 30, barWidth, 560 + newSliderShiftTmp);
}

void Comp4AudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    audioProcessor.ratio = sliders[0]->getValue();
    audioProcessor.thresh = sliders[1]->getValue();
    audioProcessor.knee = sliders[2]->getValue();
    audioProcessor.previousInputGain = audioProcessor.inputGain;
    audioProcessor.inputGainLin = sliders[3]->getValue();
    audioProcessor.inputGain = audioProcessor.Comp4DecibelsToAmplitude(sliders[3]->getValue());
    audioProcessor.previousOutputGain = audioProcessor.outputGain;
    audioProcessor.outputGainLin = sliders[4]->getValue();
    audioProcessor.outputGain = audioProcessor.Comp4DecibelsToAmplitude(sliders[4]->getValue());
    audioProcessor.attack = sliders[5]->getValue();
    audioProcessor.release = sliders[6]->getValue();
    audioProcessor.hold = sliders[7]->getValue();
    audioProcessor.lookAhead = sliders[8]->getValue();
    audioProcessor.rmsWindowLength = sliders[9]->getValue();
    audioProcessor.previousSidechainGain = audioProcessor.sidechainGain;
    audioProcessor.sidechainGainLin = sliders[10]->getValue();
    audioProcessor.sidechainGain = audioProcessor.Comp4DecibelsToAmplitude(sliders[10]->getValue());
    audioProcessor.sidechainHP = sliders[11]->getValue();
    audioProcessor.sidechainLP = sliders[12]->getValue();
    repaint();
}
void Comp4AudioProcessorEditor::onStateSwitch()
{
    audioProcessor.downward = bDownward.getToggleState();
    audioProcessor.sidechainEnable = bSidechainEnable.getToggleState();
    audioProcessor.sidechainListen = bSidechainListen.getToggleState();
    audioProcessor.sidechainMuteInput = bSidechainMuteInput.getToggleState();
    
    Comp4ButtonOptics();
    
    repaint();
}

void Comp4AudioProcessorEditor::Comp4SetMainSlider(int index)
{
    sliders[index]->setColour(juce::Slider::textBoxBackgroundColourId, cSliderTextboxBackground);
    sliders[index]->setColour(juce::Slider::textBoxOutlineColourId, cSliderTextboxBackground);
    sliders[index]->setColour(juce::Slider::textBoxTextColourId, cSliderTextboxText);
    sliders[index]->setSliderStyle(juce::Slider::RotaryVerticalDrag);
    sliders[index]->setRange(mins[index], maxs[index], steps[index]);
    if (indicesX[index] != 5)
        sliders[index]->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 20);
    else
    {
        sliders[index]->setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 20);
    }
    if (index != 9) sliders[index]->setTextValueSuffix(suffixes[index]);
    sliders[index]->setSkewFactorFromMidPoint(mids[index]);
    sliders[index]->addListener(this);
    this->addAndMakeVisible(sliders[index]);
}

void Comp4AudioProcessorEditor::Comp4SetBar(juce::Slider* slider, int type)
{
    double min = type == 1 ? -60.0 : -30.0;
    double max = type == 1 ? 0.0 : 30.0;
    double start = type == 1 ? -60.0 : 0.0;
    slider->setPopupDisplayEnabled(true, true, this);
    slider->setColour(juce::Slider::trackColourId, cBarsFill);
    slider->setSliderStyle(juce::Slider::LinearBarVertical);
    slider->setRange(min, max, 0.01);
    slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 65, 20);
    slider->setValue(start);
    this->addAndMakeVisible(slider);
}

void Comp4AudioProcessorEditor::timerCallback()
{
    for (int i = 0; i < 6; i++)
    {
        double tmp = Comp4SignaltoRMSdb(audioProcessor.currentValues[i], i, sliders[9]->getValue() == 0);
        bars[i]->setValue(tmp);
        audioProcessor.currentValues[i].clear();
    }
    bars[6]->setValue(bars[4]->getValue() - bars[0]->getValue());
    bars[7]->setValue(bars[5]->getValue() - bars[1]->getValue());
}

double Comp4AudioProcessorEditor::Comp4SignaltoRMSdb(std::vector<double>& signal, int barIndex, bool peak)
{
    if (signal.empty()) return barsPreviousValues[barIndex];
    if (peak)
    {
        barsPreviousValues[barIndex] = Comp4GetMaxdBValue(signal);
        return barsPreviousValues[barIndex];
    }
    double sum = 0;
    int elemCount = signal.size();
    for (int i = 0; i < elemCount; i++)
    {
        sum += signal[i] * signal[i];
    }
    sum /= elemCount;
    barsPreviousValues[barIndex] = 10.0 * std::log10(sum);
    jassert(!std::isnan(barsPreviousValues[barIndex]));
    return barsPreviousValues[barIndex];
}

void Comp4AudioProcessorEditor::Comp4RestoreParams()
{
    sliders[0]->setValue(audioProcessor.ratio, juce::NotificationType::dontSendNotification);
    sliders[1]->setValue(audioProcessor.thresh, juce::NotificationType::dontSendNotification);
    sliders[2]->setValue(audioProcessor.knee, juce::NotificationType::dontSendNotification);
    sliders[3]->setValue(audioProcessor.inputGainLin, juce::NotificationType::dontSendNotification);
    sliders[4]->setValue(audioProcessor.outputGainLin, juce::NotificationType::dontSendNotification);
    sliders[5]->setValue(audioProcessor.attack, juce::NotificationType::dontSendNotification);
    sliders[6]->setValue(audioProcessor.release, juce::NotificationType::dontSendNotification);
    sliders[7]->setValue(audioProcessor.hold, juce::NotificationType::dontSendNotification);
    sliders[8]->setValue(audioProcessor.lookAhead, juce::NotificationType::dontSendNotification);
    sliders[9]->setValue(audioProcessor.rmsWindowLength, juce::NotificationType::dontSendNotification);
    sliders[10]->setValue(audioProcessor.sidechainGainLin, juce::NotificationType::dontSendNotification);
    sliders[11]->setValue(audioProcessor.sidechainHP, juce::NotificationType::dontSendNotification);
    sliders[12]->setValue(audioProcessor.sidechainLP, juce::NotificationType::dontSendNotification);
    bDownward.setToggleState(audioProcessor.downward, juce::NotificationType::dontSendNotification);
    bSidechainEnable.setToggleState(audioProcessor.sidechainEnable, false);
    bSidechainListen.setToggleState(audioProcessor.sidechainListen, false);
    bSidechainMuteInput.setToggleState(audioProcessor.sidechainMuteInput, false);
    Comp4ButtonOptics();
    repaint();
}

void Comp4AudioProcessorEditor::Comp4ButtonOptics()
{
    if (bSidechainEnable.getToggleState())
    {
        bSidechainEnable.setButtonText("On");
        bSidechainEnable.setTooltip("Disable sidechain");
    }
    else
    {
        bSidechainEnable.setButtonText("Off");
        bSidechainEnable.setTooltip("Enable sidechain");
    }

    if (bSidechainListen.getToggleState())
    {
        bSidechainListen.setImages(false, true, false, juce::ImageCache::getFromMemory(BinaryData::Speaker_Icon_svg_png, BinaryData::Speaker_Icon_svg_pngSize),
            0.5f, juce::Colours::transparentBlack, juce::Image(), 0.5f, juce::Colours::transparentBlack, juce::Image(), 0.5f, juce::Colours::transparentBlack);
        bSidechainListen.setTooltip("Mute sidechain input");
    }
    else
    {
        bSidechainListen.setImages(false, true, false, juce::ImageCache::getFromMemory(BinaryData::Mute_Icon_svg_png, BinaryData::Mute_Icon_svg_pngSize),
            0.5f, juce::Colours::transparentBlack, juce::Image(), 0.5f, juce::Colours::transparentBlack, juce::Image(), 0.5f, juce::Colours::transparentBlack);
        bSidechainListen.setTooltip("Listen to sidechain input");
    }

    if (bSidechainMuteInput.getToggleState())
    {
        bSidechainMuteInput.setTooltip("Unmute main input signal");
    }
    else
    {
        bSidechainMuteInput.setTooltip("Mute main input signal");
    }
}

double Comp4AudioProcessorEditor::Comp4GetMaxdBValue(std::vector<double>& signal)
{
    double maxValue = 0;
    double value = 0;
    for (int i = 0; i < signal.size(); i++)
    {
        value = std::abs(signal[i]);
        if (value > maxValue) maxValue = value;
    }
    return 20.0 * std::log10(maxValue);
}


