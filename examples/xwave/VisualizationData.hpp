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

#include <atomic>
#include <chrono>
#include <vector>

#include "WebUI.hpp"

#ifndef VISUALIZATION_DATA_HPP
#define VISUALIZATION_DATA_HPP

#define SAMPLE_BUFFER_SIZE (HIPHOP_SHARED_MEMORY_SIZE - /* this */1024)

#define RT_SAFE

START_NAMESPACE_DISTRHO

constexpr double kFrequencyLocal   = 30.0;
constexpr double kFrequencyNetwork = 10.0;

class VisualizationData
{
public:
    VisualizationData()
        : fSendTimeLocal(0)
        , fSendTimeNetwork(0)
        , fReadPosLocal(0)
        , fReadPosNetwork(0)
        , fWritePos(0)
        , fBusy(false)
    {}

    ~VisualizationData()
    {
        // Block until addSamples() calls fBusy.clear(), to make sure deletion
        // does not happen while Plugin::run() is writing to the buffer.
        while (fBusy.test_and_set()) {};
    }

    RT_SAFE bool addSamples(const float** inputs, uint32_t frames)
    {
        if (fBusy.test_and_set()) {
            return false;
        }

        size_t i = fWritePos;
        size_t j = 0;

        while (j < frames) {
            float k = inputs[0][j] + inputs[1][j] / 2.f;
            k = 1.f + ((k < -1.f) ? -1.f : ((k > 1.f) ? 1.f : k));

            fSamplesIn[i] = static_cast<uint8_t>(255.f * k - 255.f);

            i++;
            j++;

            if (i == sizeof(fSamplesIn)) {
                i = 0;
            }
        }

        fWritePos = i;

        fBusy.clear();

        return true;
    }

    void send(WebUI& ui)
    {
        // TODO : define kDestinationEmbeddedWebView to target the embedded
        //        web view from UI::callback() . Detect the embedded web view
        //        client in WebServer.cpp by inspecting HTTP headers.

        double now = static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        ) / 1000.0;

        if ((now - fSendTimeLocal) >= (1.0 / kFrequencyLocal)) {
            fSendTimeLocal = now;

            Variant visData = Variant::createObject({
                { "samples", getSamples(fReadPosLocal) }
            });

            ui.getWebView().postMessage(visData); // bypass network
        }

        if ((now - fSendTimeNetwork) >= (1.0 / kFrequencyNetwork)) {
            fSendTimeNetwork = now;

            Variant visData = Variant::createObject({
                { "samples", getSamples(fReadPosNetwork) }
            });

            // ui.js : XWaveExampleUI.onVisualizationData()
            ui.callback("onVisualizationData", Variant::createArray({ visData }));
        }
    }

private:
    typedef std::vector<uint8_t> SampleVector;
    typedef std::atomic_flag     AtomicFlag;
    typedef std::atomic<size_t>  AtomicSize;

    const SampleVector& getSamples(AtomicSize& readPos)
    {
        size_t rpos = readPos;
        size_t wpos = fWritePos;

        if (wpos > rpos) {
            // [--R>>>W--]
            fSamplesOut.assign(fSamplesIn + rpos, fSamplesIn + wpos);
        } else {
            // [>>W---R>>]
            fSamplesOut.assign(fSamplesIn + rpos, fSamplesIn + sizeof(fSamplesIn) - 1);
            fSamplesOut.insert(fSamplesOut.end(), fSamplesIn, fSamplesIn + wpos);
        }

        readPos = wpos;

        return fSamplesOut;
    }

    uint8_t      fSamplesIn[SAMPLE_BUFFER_SIZE];
    SampleVector fSamplesOut;

    double fSendTimeLocal;
    double fSendTimeNetwork;

    AtomicSize fReadPosLocal;
    AtomicSize fReadPosNetwork;
    AtomicSize fWritePos;
    AtomicFlag fBusy;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VisualizationData)
};

END_NAMESPACE_DISTRHO

#endif // VISUALIZATION_DATA_HPP
