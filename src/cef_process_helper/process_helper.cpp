// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that can
// be found in the LICENSE file.

#include "client_app.h"

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
	void* sandbox_info = NULL;
	return CefExecuteProcess(main_args, app, sandbox_info);
}
