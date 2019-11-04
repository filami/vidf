#include <cassert>
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iostream>
#include <limits>
#include <queue>
#include <cassert>
#include <set>
#include <memory>
#include <thread>
#include <future>
#include <chrono>
#include <functional>
#include <queue>
#include <stack>
#include <array>
#include <unordered_map>
#include <random>
#include <ctime>
#include <deque>
#include <utility>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define _WIN32_WINDOWS 0x0500
#define _WIN32_WINNT 0x0400
#define WINVER 0x0500
#include <windows.h>
# undef GetObject
# undef DrawText
# undef min
# undef max
# undef GetCurrentDirectory
# undef SetCurrentDirectory

#include <gl/GL.H>
#include <gl/GLU.H>

#include <d3d11_3.h>
#include <D3Dcompiler.h>

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

namespace vidf
{
	using namespace std;
}

#include "vidf/common/rtti.h"
#include "vidf/common/types.h"
#include "vidf/common/utils.h"
#include "vidf/common/pointer.h"
#include "vidf/platform/logger.h"
#include "vidf/profiler/timeprofiler.h"

#include "yasli/Archive.h"