/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2023 Luciano Iam <oss@lucianoiam.com>
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

#include "WebUI.hpp"
#include "VisualizationData.hpp"

class WaveExampleUI : public WebUI
{
public:
    WaveExampleUI()
        : WebUI(640 /*width*/, 256 /*height*/, "#000" /*background*/)
        , fVisData(nullptr)
    {}

    ~WaveExampleUI()
    {
        // VisualizationData destructor is synchronized to the addSamples()
        // method, to ensure deletion does not happen during Plugin::run().

        if (fVisData != nullptr) {
            fVisData->~VisualizationData();
        }

        // At this point Plugin::run() has finished, it is now safe to let the
        // WebUI destructor dispose of shared memory. Safety depends on the
        // assumption that DPF UI->Plugin notifications are synchronous, or that
        // they happen before the next render cycle.
    }

    void sharedMemoryCreated(uint8_t* ptr) override
    {
        fVisData =  new(ptr) VisualizationData();
    }

    void uiIdle() override
    {
        WebUI::uiIdle();

        if (fVisData != nullptr) {
            fVisData->send(*this);
        }
    }

private:
    VisualizationData* fVisData;

};

UI* DISTRHO::createUI()
{
    return new WaveExampleUI;
}
