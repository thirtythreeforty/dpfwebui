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

#ifndef MAC_WEB_UI_HPP
#define MAC_WEB_UI_HPP

#include "../WebUIBase.hpp"
#include "CocoaWebView.hpp"

START_NAMESPACE_DISTRHO

class MacWebUI : public WebUIBase
{
public:
    MacWebUI(uint baseWidth = 0, uint baseHeight = 0,
        uint32_t backgroundColor = 0xffffffff, bool startLoading = true);
    virtual ~MacWebUI();

    void openSystemWebBrowser(String& url) override;

protected:
    uintptr_t createStandaloneWindow() override;
    void      processStandaloneEvents() override;

private:
    uintptr_t fWindow;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacWebUI)

};

END_NAMESPACE_DISTRHO

#endif  // MAC_WEB_UI_HPP
