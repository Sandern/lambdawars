#ifndef RENDER_BROWSER_HELPERS_H
#define RENDER_BROWSER_HELPERS_H
#ifdef _WIN32
#pragma once
#endif

#include "include/cef_v8.h"

class RenderBrowser;

void V8ValueListToListValue( RenderBrowser *pBrowser, const CefV8ValueList& arguments, CefRefPtr<CefListValue> args );
CefRefPtr<CefV8Value> DictionaryValueToV8Value( RenderBrowser *pBrowser, const CefRefPtr<CefDictionaryValue> args, const CefString &key );
CefRefPtr<CefV8Value> ListValueToV8Value( RenderBrowser *pBrowser, const CefRefPtr<CefListValue> args, int idx );
void ListValueToV8ValueList( RenderBrowser *pBrowser, const CefRefPtr<CefListValue> args, CefV8ValueList& arguments );

#endif // RENDER_BROWSER_HELPERS_H