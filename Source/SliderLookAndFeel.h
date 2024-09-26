/*
  ==============================================================================

    SliderLookAndFeel.h
    Created: 26 Dec 2022 5:43:33pm
    Author:  Dewey

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SliderLookAndFeel : public juce::LookAndFeel_V3
{
public:
    juce::Slider::SliderLayout getSliderLayout(juce::Slider& slider) override
    {
        juce::Slider::SliderLayout layout;
        auto bounds = slider.getBounds();
    
        layout.sliderBounds = juce::Rectangle<int>(10, 425, 50, 50);
        layout.textBoxBounds = juce::Rectangle<int>(10, 475, 50, 20);

        return layout;
    }
};
