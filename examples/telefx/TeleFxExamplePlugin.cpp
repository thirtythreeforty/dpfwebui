/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021-2022 Luciano Iam <oss@lucianoiam.com>
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

#include "extra/PluginEx.hpp"

START_NAMESPACE_DISTRHO

class TeleFxExamplePlugin : public PluginEx
{
public:
    TeleFxExamplePlugin()
        : PluginEx(0 /*parameters*/, 0 /*programs*/, 0 /*states*/)
    {}

    ~TeleFxExamplePlugin() {}

    const char* getLabel() const override
    {
        return "TeleFX";
    }

    const char* getMaker() const override
    {
        return "Luciano Iam";
    }

    const char* getLicense() const override
    {
        return "GPLv3";
    }

    uint32_t getVersion() const override
    {
        return d_version(1, 0, 0);
    }

    int64_t getUniqueId() const override
    {
        return d_cconst('H', 'H', 't', 'f');
    }

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        (void)index;
        (void)parameter;

        // TODO
        
    }

    float getParameterValue(uint32_t index) const override
    {
        (void)index;

        // TODO

        return 0;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        (void)index;
        (void)value;

        // TODO

    }

    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        (void)inputs;
        (void)outputs;
        (void)frames;
        
        // TODO
    
    }

private:

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TeleFxExamplePlugin)

};

Plugin* createPlugin()
{
    return new TeleFxExamplePlugin;
}

END_NAMESPACE_DISTRHO
