// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "client_app.h"
#include "warscef/wars_cef_shared.h"

#if CEF_ENABLE_SANDBOX
#include "include/cef_sandbox_win.h"

// The cef_sandbox.lib static library is currently built with VS2010. It may not
// link successfully with other VS versions.
#pragma comment(lib, "cef_sandbox.lib")
#endif

// Stub implementations.
std::string AppGetWorkingDirectory() {
	return std::string();
}
CefWindowHandle AppGetMainHwnd() {
	return NULL;
}

// Process entry point.
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow )
{
	//CefMainArgs main_args(argc, argv);

	HINSTANCE hinst = (HINSTANCE)GetModuleHandle(NULL);
	CefMainArgs main_args( hinst );
  
	CefRefPtr<CefApp> app(new ClientApp);

	// Execute the secondary process.
	void *sandbox_info = NULL;

#if CEF_ENABLE_SANDBOX
	// Manage the life span of the sandbox information object. This is necessary
	// for sandbox support on Windows. See cef_sandbox_win.h for complete details.
	CefScopedSandboxInfo scoped_sandbox;
	sandbox_info = scoped_sandbox.sandbox_info();
#endif

	return CefExecuteProcess(main_args, app, sandbox_info);
}
