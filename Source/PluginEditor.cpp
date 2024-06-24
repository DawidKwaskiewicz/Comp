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
    //std::cout << "Comp4AudioProcessorEditor\n";
    //debugCurrentFunctionIndex = 1;
    p.debugCurrentFunctionIndexEditor = 1;
    audioProcessor.pluginWindowOpen = true;
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    //setSize(600, 600);

    Timer::startTimer(barTimerIntervalms);

    //Comp4SetMainSlider(&sRatio, 0, 1, 0.0, 30.0, 3.0, 1.0, "");

    //for (int i = 0; i < mainSlidersCount; i++)
    for (int i = 0; i < indicesX.size(); i++)
    {
        sliders.add(new juce::Slider(codeNames[i]));
        Comp4SetMainSlider(sliders[i], indicesX[i], indicesY[i], mins[i], maxs[i], mids[i], starts[i], steps[i], suffixes[i]);
        labels.add(new juce::Label({}, names[i]));
        labels[i]->setJustificationType(juce::Justification::centredBottom);
        if (i == 3 || i == 4 || i == 9 || i == 10 || i == 11 || i == 12)
            labels[i]->setTooltip(tooltips[i]);
        if (i > 9)
        {
            //labels[i]->setBounds(335, 450, 55, 15);
            //labels[i]->setBounds(335, 465 + (indicesY[i] - 2) * 50, 55, 15);
            labels[i]->setBounds(335 + newSliderShiftTmp, 465 + newSliderShiftTmp + (indicesY[i] - 2) * 50, 55, 15);
        }
        else labels[i]->attachToComponent(sliders[i], false);
        this->addAndMakeVisible(labels[i]);
    }
    
    //0 - sInputL
    //1 - sInputR
    //2 - sSidechainL
    //3 - sSidechainR
    //4 - sOutputL
    //5 - sOutputR
    //6 - sGainL
    //7 - sGainR

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
    //bSidechainEnable.setColour(juce::TextButton::buttonColourId, cBackground);
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


    setSize(600 + newSliderShiftTmp, 600 + newSliderShiftTmp);
}

Comp4AudioProcessorEditor::~Comp4AudioProcessorEditor()
{
    //std::cout << "~Comp4AudioProcessorEditor\n";
    //debugCurrentFunctionIndex = 2;
    //audioProcessor.debugCurrentFunctionIndexEditor = 2;
    Timer::stopTimer();
    audioProcessor.pluginWindowOpen = false;
}

//==============================================================================
void Comp4AudioProcessorEditor::paint (juce::Graphics& g)
{
    //std::cout << "paint\n";
    //debugCurrentFunctionIndex = 3;
    audioProcessor.debugCurrentFunctionIndexEditor = 3;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    // graph layout
    g.setColour(cGraphBackground);
    int graphSideLength = 470;
    g.fillRect(0, 0, graphSideLength, graphSideLength);
    //g.fillRect(0, 0, 400, 400);
    g.setColour(cGrid);
    /*for (int i = 1; i < 5; i++)
    {
        g.drawLine(juce::Line<float>(juce::Point<float>(0, i * 80), juce::Point<float>(400, i * 80)), 1.0f);
        labelsGridX.add(new juce::Label({}, gridSteps[i - 1]));
        labelsGridX[i - 1]->setBounds(0, i * 80 - 15, 30, 10);
        labelsGridX[i - 1]->setColour(juce::Label::textColourId, cLabelGridText);
        labelsGridX[i - 1]->setFont(labelGridFont);
        this->addAndMakeVisible(labelsGridX[i-1]);
        if (i != 4)
        {
            g.drawLine(juce::Line<float>(juce::Point<float>(i * 80, 0), juce::Point<float>(i * 80, 400)), 1.0f);
            labelsGridY.add(new juce::Label({}, gridSteps[gridSteps.size() - i]));
            labelsGridY[i - 1]->setBounds(i * 80 - 29, 385, 30, 10);
            labelsGridY[i - 1]->setColour(juce::Label::textColourId, cLabelGridText);
            labelsGridY[i - 1]->setFont(labelGridFont);
            this->addAndMakeVisible(labelsGridY[i-1]);
        }
        else
        {
            g.drawLine(juce::Line<float>(juce::Point<float>(i * 80, 0), juce::Point<float>(i * 80, 319)), 1.0f);
            labelsGridY.add(new juce::Label({}, gridSteps[gridSteps.size() - i]));
            labelsGridY[i - 1]->setBounds(i * 80 - 29, 305, 30, 10);
            labelsGridY[i - 1]->setColour(juce::Label::textColourId, cLabelGridText);
            labelsGridY[i - 1]->setFont(labelGridFont);
            this->addAndMakeVisible(labelsGridY[i - 1]);
        }
    }*/
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

    // bars area
    //g.setColour(cGrid);
    int xOffset = 2;
    int barGridLabelWidth = 25;
    int barGridLabelHeight = 10;
    double startY = 30.0;
    //double endY = 580.0;
    double endY = 580.0 + newSliderShiftTmp;
    /*for (int i = 0; i < bar1GridSteps.size(); i++)
    {
        bar1GridLabels.add(new juce::Label({}, bar1GridSteps[i]));
        bar1GridLabels[i]->setBounds(400 + xOffset, std::round(startY + i * (endY - startY) / (bar1GridSteps.size() - 1.0)), barGridLabelWidth, barGridLabelHeight);
        bar1GridLabels[i]->setColour(juce::Label::textColourId, cBarsGridLabels);
        bar1GridLabels[i]->setFont(barLabelGridFont);
        bar1GridLabels[i]->setJustificationType(juce::Justification::centredRight);
        this->addAndMakeVisible(bar1GridLabels[i]);

        bar2GridLabels.add(new juce::Label({}, bar2GridSteps[i]));
        bar2GridLabels[i]->setBounds(600 - xOffset - barGridLabelWidth + 2, std::round(startY + i * (endY - startY) / (bar2GridSteps.size() - 1.0)), barGridLabelWidth, barGridLabelHeight);
        bar2GridLabels[i]->setColour(juce::Label::textColourId, cBarsGridLabels);
        bar2GridLabels[i]->setFont(barLabelGridFont);
        bar2GridLabels[i]->setJustificationType(juce::Justification::centredLeft);
        this->addAndMakeVisible(bar2GridLabels[i]);
    }*/
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


    // threshold line
    g.setColour(cThreshLine);
    int threshLineHeight = std::round(-1.0 * sliders[1]->getValue() / 60.0 * graphSideLength);
    g.drawLine(juce::Line<float>(juce::Point<float>(0, threshLineHeight), juce::Point<float>(400 + newSliderShiftTmp, threshLineHeight)), 1.0f);

    // comp line
    g.saveState();
    g.reduceClipRegion(0, 0, 400 + newSliderShiftTmp, 400 + newSliderShiftTmp);
    g.setColour(cCompLine);
    compLine.clear();
    float x1, y1, x2, y2, x3, y3, x4, y4;
    //double thresh = -1.0 * sliders[1]->getValue() / 60.0 * 400.0;
    double thresh = -1.0 * sliders[1]->getValue() / 60.0 * (400.0 + newSliderShiftTmp);

    if (bDownward.getToggleState())
    {
        x1 = std::max(400.0 + newSliderShiftTmp - thresh - (400.0 + newSliderShiftTmp - thresh) * sliders[0]->getValue(), 0.0);
        y1 = std::min(thresh + (400.0 + newSliderShiftTmp - thresh) / sliders[0]->getValue(), 400.0 + newSliderShiftTmp);
        x4 = 400.0f + newSliderShiftTmp;
        y4 = 0.0f;
    }
    else
    {
        x1 = 0.0f;
        y1 = 400.0f + newSliderShiftTmp;
        x4 = std::min(400 + newSliderShiftTmp - thresh + thresh * sliders[0]->getValue(), 400.0 + newSliderShiftTmp);
        y4 = std::max(thresh - thresh / sliders[0]->getValue(), 0.0);
    }
    double knee = sliders[2]->getValue() / 60.0 * (400.0 + newSliderShiftTmp);
    x2 = 400 + newSliderShiftTmp - thresh - knee / 2;
    x3 = 400 + newSliderShiftTmp - thresh + knee / 2;
    //y2 = thresh + (thresh - x2) / thresh * (400 - thresh);
    //y3 = thresh - (x3 - thresh) / (400 - thresh) * thresh;
    //y2 = std::min(thresh + (400.0 - thresh - x2) / (400.0 - thresh - x1) * (y1 - thresh), 400.0);
    y2 = thresh + (400.0 + newSliderShiftTmp - thresh - x2) / (400.0 + newSliderShiftTmp - thresh - x1) * (y1 - thresh), 400.0 + newSliderShiftTmp;
    y3 = thresh - (x3 - (400.0 + newSliderShiftTmp - thresh)) / (x4 - (400.0 + newSliderShiftTmp - thresh)) * (thresh - y4);
    compLine.startNewSubPath(x1, y1);
    compLine.lineTo(x2, y2);
    //g.setColour(juce::Colours::blue);
    //compLine.lineTo(400.0 - thresh, thresh);
    //g.setColour(juce::Colours::green);
    //compLine.lineTo(x3, y3);
    //g.setColour(juce::Colours::yellow);
    if (!(x3 < 0.0 || x3 > 400.0 + newSliderShiftTmp || y3 < 0.0 || y3 > 400.0 + newSliderShiftTmp))
        compLine.quadraticTo(400.0 + newSliderShiftTmp - thresh, thresh, x3, y3);
    compLine.lineTo(x4, y4);
    g.strokePath(compLine, juce::PathStrokeType(1.5f));
    
    g.restoreState();

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

    //g.fillRect(0, 425, 70, 75);


}

void Comp4AudioProcessorEditor::resized()
{
    //std::cout << "resized\n";
    //debugCurrentFunctionIndex = 4;
    audioProcessor.debugCurrentFunctionIndexEditor = 4;
    /*juce::LookAndFeel_V4 lf;
    juce::Slider::SliderLayout *layout = &lf.getSliderLayout(sRatio);*/
    //SliderLookAndFeel slf;
    //sRatio.setBounds(0, 150, 300, 300);
    //sRatio.setBounds(0, 425, 70, 75); 
    //sRatio.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    //getLookAndFeel().getSliderLayout(sRatio).textBoxBounds.setX(10);
    //getLookAndFeel().getSliderLayout(sRatio).textBoxBounds.setY(450);
    //getLookAndFeel().getSliderLayout(sRatio).textBoxBounds.setHeight(50);
    /*layout->textBoxBounds.setX(10);
    layout->textBoxBounds.setY(450);
    layout->textBoxBounds.setHeight(50);
    sRatio.setLookAndFeel(&lf);*/
    //sRatio.setBounds(0, 0, 600, 600);
    //sRatio.setLookAndFeel(&slf);
    //sRatio.setBounds(0, 425, 70, 75);
    //sThresh.setBounds(120, 300, 100, 100);
    //bDownward.setBounds(230, 300, 50, 50);

    for (int i = 0; i < mainSlidersCount; i++)
    {
        sliders[i]->setBounds(indicesX[i] * 70, 420 + newSliderShiftTmp + indicesY[i] * 100, 70, 75);
    }
    for (int i = mainSlidersCount; i < sliders.size(); i++)
    {
        sliders[i]->setBounds(290 + newSliderShiftTmp, 465 + newSliderShiftTmp + (indicesY[i]-2) * 50, 100, 45);
    }

    //bUpward.setBounds(245 + newSliderShiftTmp, 335 + newSliderShiftTmp, 150, 20);
    //bDownward.setBounds(245 + newSliderShiftTmp, 365 + newSliderShiftTmp, 150, 20);
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
    //std::cout << "sliderValueChanged\n";
    //debugCurrentFunctionIndex = 5;
    audioProcessor.debugCurrentFunctionIndexEditor = 5;
    audioProcessor.ratio = sliders[0]->getValue();
    audioProcessor.thresh = sliders[1]->getValue();
    audioProcessor.knee = sliders[2]->getValue();
    audioProcessor.previousInputGain = audioProcessor.inputGain;
    audioProcessor.inputGain = audioProcessor.Comp4DecibelsToAmplitude(sliders[3]->getValue());
    audioProcessor.previousOutputGain = audioProcessor.outputGain;
    //audioProcessor.outputGain = sliders[4]->getValue();
    audioProcessor.outputGain = audioProcessor.Comp4DecibelsToAmplitude(sliders[4]->getValue());
    audioProcessor.attack = sliders[5]->getValue();
    audioProcessor.release = sliders[6]->getValue();
    audioProcessor.hold = sliders[7]->getValue();
    audioProcessor.lookAhead = sliders[8]->getValue();
    audioProcessor.rmsWindowLength = sliders[9]->getValue();
    audioProcessor.previousSidechainGain = audioProcessor.sidechainGain;
    //audioProcessor.sidechainInputGain = sliders[10]->getValue();
    audioProcessor.sidechainGain = audioProcessor.Comp4DecibelsToAmplitude(sliders[10]->getValue());
    audioProcessor.sidechainHP = sliders[11]->getValue();
    audioProcessor.sidechainLP = sliders[12]->getValue();
    //sRatio.setBounds(0, 425, 70, 75);
    // audioProcessor.Comp4UpdateLatency(std::max(audioProcessor.rmsWindowLength, audioProcessor.lookAhead));
    //audioProcessor.Comp4UpdateLatency(std::max((double)audioProcessor.rmsOffset, audioProcessor.lookAhead));
    repaint();
}
void Comp4AudioProcessorEditor::onStateSwitch()
{
    //std::cout << "onStateSwitch\n";
    //debugCurrentFunctionIndex = 6;
    audioProcessor.debugCurrentFunctionIndexEditor = 6;
    audioProcessor.downward = bDownward.getToggleState();
    audioProcessor.sidechainEnable = bSidechainEnable.getToggleState();
    if (audioProcessor.keySignalPrevious != audioProcessor.sidechainEnable)
    {
        audioProcessor.keySignalPrevious = audioProcessor.sidechainEnable;
        audioProcessor.keySignalSwitched = true;
    }
    audioProcessor.sidechainListen = bSidechainListen.getToggleState();
    audioProcessor.sidechainMuteInput = bSidechainMuteInput.getToggleState();
    
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
    repaint();
}

void Comp4AudioProcessorEditor::Comp4SetMainSlider(juce::Slider* slider, int indexX, int indexY, double min, double max, double mid, double start, double step, std::string suffix)
{
    //std::cout << "Comp4SetMainSlider\n";
    //debugCurrentFunctionIndex = 7;
    audioProcessor.debugCurrentFunctionIndexEditor = 7;
    //SliderLookAndFeel slf;
    slider->setColour(juce::Slider::textBoxBackgroundColourId, cSliderTextboxBackground);
    slider->setColour(juce::Slider::textBoxOutlineColourId, cSliderTextboxBackground);
    slider->setColour(juce::Slider::textBoxTextColourId, cSliderTextboxText);
    slider->setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider->setRange(min, max, step);
    if (indexX != 5)
        slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 65, 20);
    else
    {
        slider->setTextBoxStyle(juce::Slider::TextBoxRight, false, 65, 20);
        //slider->setTooltip(tooltips[indexY + 6]);
    }
    slider->setTextValueSuffix(suffix);
    slider->setSkewFactorFromMidPoint(mid);
    slider->setValue(start);
    slider->addListener(this);
    this->addAndMakeVisible(slider);
}

void Comp4AudioProcessorEditor::Comp4SetBar(juce::Slider* slider, int type)
{
    //std::cout << "Comp4SetBar\n";
    //debugCurrentFunctionIndex = 8;
    audioProcessor.debugCurrentFunctionIndexEditor = 8;
    double min = type == 1 ? -60.0 : -30.0;
    double max = type == 1 ? 0.0 : 30.0;
    double start = type == 1 ? -60.0 : 0.0;
    //double min = type == 1 ? 0.0 : -1.0;
    //double max = type == 1 ? 1.0 : 1.0;
    //double start = type == 1 ? 0.0 : 0.0;
    //if (type == 2) slider->setPopupDisplayEnabled(true, true, this);
    slider->setPopupDisplayEnabled(true, true, this);
    //slider->setColour(juce::Slider::backgroundColourId, cBarsBackground);
    slider->setColour(juce::Slider::trackColourId, cBarsFill);
    //slider->setColour(juce::Slider::backgroundColourId, cOutlines);
    //slider->setColour(juce::Slider::trackColourId, cOutlines);
    //slider->setColour(juce::Slider::rotarySliderFillColourId, cOutlines);
    //slider->setColour(juce::Slider::rotarySliderOutlineColourId, cOutlines);
    //slider->setColour(juce::Slider::textBoxBackgroundColourId, cOutlines);
    //slider->setColour(juce::Slider::textBoxHighlightColourId, cOutlines);
    //slider->setColour(juce::Slider::textBoxOutlineColourId, cOutlines);
    //slider->setColour(juce::Slider::textBoxTextColourId, cOutlines);
    //slider->setColour(juce::Slider::thumbColourId, cOutlines);
    slider->setSliderStyle(juce::Slider::LinearBarVertical);
    slider->setRange(min, max, 0.01);
    slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 65, 20);
    slider->setValue(start);
    //slider->addListener(this);
    this->addAndMakeVisible(slider);
}

void Comp4AudioProcessorEditor::timerCallback()
{
    callbackCounter++;
    //std::cout << "timerCallback\n";
    //debugCurrentFunctionIndex = 9;
    audioProcessor.debugCurrentFunctionIndexEditor = 9;
    for (int i = 0; i < 6; i++)
    {
        //bars[i]->setValue(Comp4Signaltodb(audioProcessor.currentValuesOld[i]));
        //bars[i]->setValue(std::abs(audioProcessor.currentValuesOld[i]));

        //bars[i]->setValue(Comp4SignaltoRMSdb(audioProcessor.currentValues[i]));

        double tmp = Comp4SignaltoRMSdb(audioProcessor.currentValues[i]);
        //jassert(!std::isnan(tmp));
        bars[i]->setValue(tmp);

        audioProcessor.currentValues[i].clear();
        //if (i != 0) barCountDiff.push_back(audioProcessor.currentValues[i - 1].size());
    }
    bars[6]->setValue(bars[4]->getValue() - bars[0]->getValue());
    bars[7]->setValue(bars[5]->getValue() - bars[1]->getValue());
}

//double Comp4AudioProcessorEditor::Comp4SignaltoRMSdb(std::vector<double>& signal)
double Comp4AudioProcessorEditor::Comp4SignaltoRMSdb(std::vector<double>& signal)
{
    //std::cout << "Comp4SignaltoRMSdb\n";
    //debugCurrentFunctionIndex = 10;
    audioProcessor.debugCurrentFunctionIndexEditor = 10;
    if (signal.empty()) return -60.0;
    double sum = 0;
    int elemCount = signal.size();
    for (int i = 0; i < elemCount; i++)
    {
        sum += signal[i] * signal[i];
    }
    sum /= elemCount;
    return 10.0 * std::log10(sum);
    //return -30.0f;
}

double Comp4AudioProcessorEditor::Comp4Signaltodb(double signal)
{
    //std::cout << "Comp4Signaltodb\n";
    //debugCurrentFunctionIndex = 11;
    audioProcessor.debugCurrentFunctionIndexEditor = 11;
    return 20.0 * std::log10(std::abs(signal));
}


