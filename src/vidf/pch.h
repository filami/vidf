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


#pragma warning(push)
#pragma warning(disable : 4005)
#include <d3d9.h>
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <d3dx11.h>
#include <d3dx9.h>
#include <XInput.h>
# undef GetObject
# undef DrawText
# undef min
# undef max
#pragma warning(pop)

#include <gl/GL.H>
#include <gl/GLU.H>



#include "vidf/common/rtti.h"
#include "vidf/common/types.h"
#include "vidf/common/utils.h"
#include "vidf/common/pointer.h"
#include "vidf/profiler/timeprofiler.h"
