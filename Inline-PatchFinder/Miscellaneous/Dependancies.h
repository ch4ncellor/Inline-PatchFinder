#pragma once

#include <iostream>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <tchar.h>
#include <list>
#include <random>
#include <sstream>
#include <ratio>
#include <algorithm>
#include <vector>
#include <fstream>
#include <stdio.h>
#include <filesystem>
#include <TlHelp32.h>
#include <Psapi.h>
#include <comdef.h> 
#include <winnt.h>

#include "../Utilities/Utilities.h"

#include <Zydis/Zydis.h>
#include <Zycore/Zycore.h>
#include <Zycore/Format.h>
#include <Zycore/API/Memory.h>
#include <Zycore/LibC.h>

#define LOG( s, ... ) { \
	printf_s( (s), __VA_ARGS__); \
}

#define PAUSE_SYSTEM_CMD( b ) { \
	system(("pause")); \
	if (b) \
		return 0; \
}