﻿#pragma once
#include "resource.h"

extern class invalidParameters;
extern class createWindowFailed;

namespace cmdLineCfg
{
	// Varibles
#ifdef _DEBUG
	bool isDebugMode = true;
	bool fileLogged = false;
#else
	bool isDebugMode = false;
	bool fileLogged = false;
#endif

	
	bool showHelp = false;
	int argvLogFilePathIndex = 0;


	// Functions
	bool parseCmdLine(_In_ int argc, _In_ char *argv[]);
}


HWND createEXWindow(int width, int height, bool isWindowShow);

std::wstring wsLogPath;