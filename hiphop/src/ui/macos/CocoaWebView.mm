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

#import <Foundation/Foundation.h>
#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>

#include "CocoaWebView.hpp"

#define UNPACK_RGBA_NORMALIZED(c,t)  ( c >> 24)               / (t)(255), \
                                     ((c & 0x00ff0000) >> 16) / (t)(255), \
                                     ((c & 0x0000ff00) >> 8 ) / (t)(255), \
                                     ( c & 0x000000ff)        / (t)(255)

// Avoid symbol name collisions
#define OBJC_INTERFACE_NAME_HELPER_1(INAME, SEP, SUFFIX) INAME ## SEP ## SUFFIX
#define OBJC_INTERFACE_NAME_HELPER_2(INAME, SUFFIX) OBJC_INTERFACE_NAME_HELPER_1(INAME, _, SUFFIX)
#define OBJC_INTERFACE_NAME(INAME) OBJC_INTERFACE_NAME_HELPER_2(INAME, HIPHOP_PROJECT_ID_HASH)

#define DistrhoWebView         OBJC_INTERFACE_NAME(DistrhoWebView)
#define DistrhoWebViewDelegate OBJC_INTERFACE_NAME(DistrhoWebViewDelegate)

#define fNsBackground ((NSView *)fBackground)
#define fNsWebView    ((DistrhoWebView *)fWebView)
#define fNsDelegate   ((DistrhoWebViewDelegate *)fDelegate)

#define JS_POST_MESSAGE_SHIM "window.host.postMessage = (args) => window.webkit.messageHandlers.host.postMessage(args);"

USE_NAMESPACE_DISTRHO

@interface DistrhoWebView: WKWebView
@property (readonly, nonatomic) CocoaWebView* cppView;
@property (readonly, nonatomic) NSView* pluginRootView;
@end

@interface DistrhoWebViewDelegate: NSObject<WKNavigationDelegate, WKUIDelegate, WKScriptMessageHandler>
@property (assign, nonatomic) CocoaWebView *cppView;
@end

CocoaWebView::CocoaWebView()
{
    fBackground = [[NSView alloc] initWithFrame:CGRectZero];
    fNsBackground.autoresizesSubviews = YES;

    fWebView = [[DistrhoWebView alloc] initWithFrame:CGRectZero];
    fNsWebView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
    [fNsBackground addSubview:fNsWebView];

    fDelegate = [[DistrhoWebViewDelegate alloc] init];
    fNsDelegate.cppView = this;
    fNsWebView.navigationDelegate = fNsDelegate;
    fNsWebView.UIDelegate = fNsDelegate;
    [fNsWebView.configuration.userContentController addScriptMessageHandler:fNsDelegate name:@"host"];

    // EventTarget() constructor is unavailable on Safari < 14 (2020-09-16)
    // https://developer.mozilla.org/en-US/docs/Web/API/EventTarget/EventTarget
    // https://github.com/ungap/event-target
    String js = String(
#include "ui/macos/polyfill.js.inc"
    );
    injectScript(js);

    injectHostObjectScripts();

    js = String(JS_POST_MESSAGE_SHIM);
    injectScript(js);
}

CocoaWebView::~CocoaWebView()
{
    [fNsDelegate release];
    [fNsWebView removeFromSuperview];
    [fNsWebView release];
    [fNsBackground removeFromSuperview];
    [fNsBackground release];
}

void CocoaWebView::realize()
{
    [(NSView *)getParent() addSubview:fNsBackground];

    onSize(getWidth(), getHeight());
    
    @try {
        if ([fNsBackground respondsToSelector:@selector(setBackgroundColor:)]) {
            CGFloat c[] = { UNPACK_RGBA_NORMALIZED(getBackgroundColor(), CGFloat) };
            NSColor* color = [NSColor colorWithRed:c[0] green:c[1] blue:c[2] alpha:c[3]];
            [fNsBackground setValue:color forKey:@"backgroundColor"];
        }

        if ([fNsWebView respondsToSelector:@selector(_setDrawsBackground:)]) {
            NSNumber *no = [[NSNumber alloc] initWithBool:NO];
            [fNsWebView setValue:no forKey:@"drawsBackground"];
            [no release];
        }
    } @catch (NSException *e) {
        NSLog(@"Could not set background color");
    }
}

void CocoaWebView::navigate(String& url)
{
    NSString *urlStr = [[NSString alloc] initWithCString:url encoding:NSUTF8StringEncoding];
    NSURL *urlObj = [NSURL URLWithString:[urlStr stringByAddingPercentEncodingWithAllowedCharacters:
        [NSCharacterSet URLFragmentAllowedCharacterSet]]];
    if ([urlObj.scheme isEqual:@"file"]) {
        NSURL *urlBase = [urlObj URLByDeletingLastPathComponent];
        [fNsWebView loadFileURL:urlObj allowingReadAccessToURL:urlBase];
    } else {
        [fNsWebView loadRequest:[NSURLRequest requestWithURL:urlObj]];
    }
    [urlStr release];
}

void CocoaWebView::runScript(String& source)
{
    NSString *js = [[NSString alloc] initWithCString:source encoding:NSUTF8StringEncoding];
    [fNsWebView evaluateJavaScript:js completionHandler: nil];
    [js release];
}

void CocoaWebView::injectScript(String& source)
{
    NSString *js = [[NSString alloc] initWithCString:source encoding:NSUTF8StringEncoding];
    WKUserScript *script = [[WKUserScript alloc] initWithSource:js
        injectionTime:WKUserScriptInjectionTimeAtDocumentStart forMainFrameOnly:YES];
    [fNsWebView.configuration.userContentController addUserScript:script];
    [script release];
    [js release];
}

void CocoaWebView::onSize(uint width, uint height)
{
    CGRect frame;
    frame.size.width = (CGFloat)width;
    frame.size.height = (CGFloat)height;
    frame = [fNsBackground.window convertRectFromBacking:frame]; // convert DPF coords. to AppKit
    frame.origin = fNsBackground.frame.origin; // keep position, for example REAPER sets it
    fNsBackground.frame = frame;
}

@implementation DistrhoWebView

- (CocoaWebView *)cppView
{
    return ((DistrhoWebViewDelegate *)self.navigationDelegate).cppView;
}

- (NSView *)pluginRootView
{
    return self.superview.superview;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)event
{
    // Allow the web view to immediately process clicks when the plugin window
    // is unfocused, otherwise the first click is swallowed to focus web view.
    (void)event;
    return YES;
}

- (BOOL)performKeyEquivalent:(NSEvent *)event
{
    // Make the web view ignore key shortcuts like Cmd+Q and Cmd+H
    (void)event;
    return NO;
}

// Passthrough keyboard events targeting the web view to the plugin root view
// whenever the keyboard focus is not requested. Focus must be switched on/off
// on demand by the UI JS code. This allows for example to play the virtual
// keyboard on Live while the web view UI is open and no text input is required.

- (void)keyDown:(NSEvent *)event
{
    if (self.cppView->getKeyboardFocus()) {
        [super keyDown:event];
    } else {
        [self.pluginRootView keyDown:event];
    }
}

- (void)keyUp:(NSEvent *)event
{
    if (self.cppView->getKeyboardFocus()) {
        [super keyUp:event];
    } else {
        [self.pluginRootView keyUp:event];
    }
}

- (void)flagsChanged:(NSEvent *)event
{
    if (self.cppView->getKeyboardFocus()) {
        [super flagsChanged:event];
    } else {
        [self.pluginRootView flagsChanged:event];
    }
}

@end

@implementation DistrhoWebViewDelegate

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
    (void)webView;
    (void)navigation;
    self.cppView->didFinishNavigation();
}

- (void)webView:(WKWebView *)webView
        runOpenPanelWithParameters:(WKOpenPanelParameters *)parameters 
        initiatedByFrame:(WKFrameInfo *)frame
        completionHandler:(void (^)(NSArray<NSURL *> *URLs))completionHandler
{
    NSOpenPanel* openPanel = [[NSOpenPanel alloc] init];
    openPanel.canChooseFiles = YES;

    [openPanel beginWithCompletionHandler:^(NSModalResponse result) {
        if (result == NSModalResponseOK) {
            completionHandler(openPanel.URLs);
        } else if (result == NSModalResponseCancel) {
            completionHandler(nil);
        }
    }];

    [openPanel release];
}

- (void)userContentController:(WKUserContentController *)controller didReceiveScriptMessage:(WKScriptMessage *)message
{
    (void)controller;
    
    if (![message.body isKindOfClass:[NSArray class]]) {
        return;
    }

    // Avoid clashing with macOS WebKit class JSValue by specifying namespace
    JSValue::array args;

    for (id objcArg : (NSArray *)message.body) {
        if (CFGetTypeID(objcArg) == CFBooleanGetTypeID()) {
            args.push_back(DISTRHO::JSValue(static_cast<bool>([objcArg boolValue])));
        } else if ([objcArg isKindOfClass:[NSNumber class]]) {
            args.push_back(DISTRHO::JSValue([objcArg doubleValue]));
        } else if ([objcArg isKindOfClass:[NSString class]]) {
            args.push_back(DISTRHO::JSValue([objcArg cStringUsingEncoding:NSUTF8StringEncoding]));
        } else {
            args.push_back(DISTRHO::JSValue()); // null
        }
    }

    self.cppView->didReceiveScriptMessage(args);
}

@end
