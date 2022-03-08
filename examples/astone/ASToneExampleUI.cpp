/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "DistrhoUI.hpp"
#include "EventHandlers.hpp"

#include "ui/Blendish.hpp"

START_NAMESPACE_DISTRHO

class ASToneExampleUI : public UI, public KnobEventHandler::Callback
{
public:
    ASToneExampleUI()
        : UI(240 /*width*/, 160 /*height*/)
        , fBlendish(this)
        , fKnob(&fBlendish)
    {
        double k = getScaleFactor();
        setSize(k * getWidth(), k * getHeight());

        glClearColor(0.9f, 0.9f, 0.9f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        fBlendish.setSize(k * getWidth(), k * getHeight());

        fKnob.setAbsoluteX((k * getWidth() - fKnob.getWidth()) / 2);
        fKnob.setAbsoluteY((k * getHeight() - fKnob.getHeight()) / 3);
        fKnob.setRange(220.f, 880.f);
        fKnob.setValue(440.f);
        fKnob.setUnit("Hz");
        fKnob.setCallback(this);
    }

    ~ASToneExampleUI() {}

    void knobDragStarted(SubWidget*) override {}
    void knobDragFinished(SubWidget*) override {}

    void knobValueChanged(SubWidget*, float value) override
    {
        setParameterValue(0, value);
    }

protected:
    void onDisplay() override {}

    void parameterChanged(uint32_t index, float value) override
    {
        switch (index) {
            case 0:
                fKnob.setValue(value);
                break;
        }
    }

private:
    BlendishSubWidgetSharedContext fBlendish;
    BlendishKnob fKnob;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ASToneExampleUI)

};

UI* createUI()
{
    return new ASToneExampleUI;
}

END_NAMESPACE_DISTRHO
