//
// pch.h
// Header for standard system include files.
//

#pragma once

#include <winsdkver.h>
#define _WIN32_WINNT 0x0601
#include <SDKDDKVer.h>

// Use the C++ standard templated min/max
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <wrl/client.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <algorithm>
#include <exception>
#include <memory>
#include <stdexcept>
#include <iomanip>
#include <map>
#include <algorithm>
#include <string>
#include <memory>
#include <conio.h>
#include <cmath>
#include <math.h>
#include <chrono>
#include <vector>
#include <strsafe.h>
#include <shellapi.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include "CommonStates.h"
#include "DDSTextureLoader.h"
#include "DirectXHelpers.h"
#include "Effects.h"
#include "GamePad.h"
#include "GeometricPrimitive.h"
#include "GraphicsMemory.h"
#include "Keyboard.h"
#include "Model.h"
#include "Mouse.h"
#include "PostProcess.h"
#include "PrimitiveBatch.h"
#include "ScreenGrab.h"
#include "SimpleMath.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "VertexTypes.h"
#include "WICTextureLoader.h"
#include <wrl.h>
#include <array>
#include <wchar.h>
#include <time.h>
#include <exception>
#include <list>
#include <mutex>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <functional>
#include <malloc.h>
#include <stddef.h>
#include <stdint.h>
#include <wincodec.h>

#pragma comment(lib,"d3d11.lib")

namespace DX
{
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            // Set a breakpoint on this line to catch DirectX API errors
            throw std::exception();
        }
    }
}