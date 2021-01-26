#ifndef PCH_H
#define PCH_H

#define NOMINMAX

#include <windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <assert.h>

#include <vector>
#include <list>
#include <memory>
#include <string>
#include <stdexcept>
#include <stdlib.h>
#include <map>
#include <array>
#include <functional>
#include <sstream>
#include <set>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <limits>
#include <random>

#define JASSERT(x) assert(x)
#define JMESSAGE(x) MessageBoxA(0, x, "", MB_OK)

using int8 = char;
using uint8 = unsigned char;
using int16 = short;
using uint16 = unsigned short;
using int32 = int;
using uint32 = unsigned int;
using int64 = __int64;
using uint64 = unsigned __int64;

using tchar = wchar_t;

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

const unsigned int SM_WIDTH = 1024;
const unsigned int SM_HEIGHT = 1024;

const unsigned int SM_ARRAY_WIDTH = 512;
const unsigned int SM_ARRAY_HEIGHT = 512;

const unsigned int SM_LINKED_LIST_WIDTH = SM_WIDTH;
const unsigned int SM_LINKED_LIST_HEIGHT = SM_HEIGHT;

const unsigned int LUMINANCE_WIDTH = 512;
const unsigned int LUMINANCE_HEIGHT = 512;

constexpr int NUM_CASCADES = 3;
constexpr int NUM_FRUSTUM_CORNERS = 8;

#define FORCEINLINE __forceinline

enum class EMouseButtonType
{
	LEFT = 0,
	MIDDLE,
	RIGHT,
	MAX
};

extern std::map<int, bool> g_KeyState;
extern std::map<EMouseButtonType, bool> g_MouseState;
extern float g_timeDeltaSecond;

//////////////////////////////////////////////////////////////////////////
// https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
template <class T>
inline void hash_combine(std::size_t& seed, const T& v)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

#include "jRHIType.h"
#include "jPerformanceProfile.h"

#define TRUE_PER_MS(WaitMS)\
[waitMS = WaitMS]() -> bool\
{\
	static int64 lastTick = GetTickCount64();\
	int64 currentTick = GetTickCount64();\
	if (currentTick - lastTick >= waitMS)\
	{\
		lastTick = currentTick;\
		return true;\
	}\
	return false;\
}()

// imgui
#include "IMGUI/imgui.h"
#include "IMGUI/imgui_impl_glfw.h"
#include "IMGUI/imgui_impl_opengl3.h"


#include "jShadowAppProperties.h"

#define DEBUG_OUTPUT_ON 0
//#define DEBUG_OUTPUT_LEVEL 0	// show all
//#define DEBUG_OUTPUT_LEVEL 1	// show mid priority
#define DEBUG_OUTPUT_LEVEL 2	// show high priority

#define USE_TW 1

#endif //PCH_H
