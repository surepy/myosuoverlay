#pragma once

#include <Windows.h>
#include <Tlhelp32.h>
#include <string>
#include <iomanip>
#include <strsafe.h>
#include <iostream>
#include "SimpleMath.h"
#include "Beatmap.h"

class Utilities {
public:
    static inline const DWORD getProcessIDbyName(const wchar_t* name) {
        DWORD process_id = NULL;
        HANDLE process_list = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);

        PROCESSENTRY32 entry = { 0 };
        entry.dwSize = sizeof PROCESSENTRY32;

        if (Process32First(process_list, &entry)) {
            while (Process32Next(process_list, &entry)) {
                if (_wcsicmp(entry.szExeFile, name) == 0) {
                    process_id = entry.th32ProcessID;
                }
            }
        }

        CloseHandle(process_list);

        return process_id;
    };

    static std::wstring ReadWStringFromMemory(HANDLE hnd, DWORD ptr)
    {
        DWORD length;
        std::wstring str;

        if (ptr == NULL)
            return L" ";

        ReadProcessMemory(hnd, LPCVOID(ptr + 4), &length, sizeof DWORD, nullptr);
        // https://github.com/Piotrekol/ProcessMemoryDataFinder/blob/3242645aebcf76b33337f516b363ac67980fb873/ProcessMemoryDataFinder/API/Sig.cs#L241
        // in here, it causes std::bad_alloc.

        if (length > 131072)
            return std::wstring(L" ");

        wchar_t* buf = new wchar_t[(size_t)length];
        ReadProcessMemory(hnd, LPCVOID(ptr + 8), buf, (size_t)(length * sizeof(wchar_t)), nullptr);
        str = std::wstring(buf, length);
        delete[] buf;

        return str;
    }

    struct MappingFile
    {
    public:
        HANDLE handle;
        LPVOID* data;
        std::string name;

        MappingFile(std::string map_name)
        {
            name = map_name;
            handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 8 * 1024, map_name.c_str()); // todo readonly
            data = reinterpret_cast<PVOID*>(MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 8 * 1024));
        }

        ~MappingFile()
        {
            UnmapViewOfFile(data);
            CloseHandle(handle);
        }

        TCHAR* ReadToEnd()
        {
            if (handle == NULL)
                return 0;

            return reinterpret_cast<TCHAR*>(data);
        }
    };

    static uint64_t factorial(int n)
    {
        if (n < 0)
            throw new std::range_error("no");
        if (n == 0)
            return 1;
        else
            return n * factorial(n - 1);
    }

    static uint32_t binomialCoeff(int n, int k)
    {
        int res = 1;

        // Since C(n, k) = C(n, n-k)
        if (k > n - k)
            k = n - k;

        // Calculate value of
        // [n * (n-1) *---* (n-k+1)] / [k * (k-1) *----* 1]
        for (int i = 0; i < k; ++i)
        {
            res *= (n - i);
            res /= (i + 1);
        }

        return res;
    }

    static double getBezierPoint(std::vector<int> *points, double t) {
        double answer = 0;
        int n = points->size() - 1;

        for (int i = 0; i < points->size(); i++)
        {
            answer += (binomialCoeff(n, i) *  std::pow((1 - t), (n - i))  * std::pow(t, i) * (double)points->at(i));
        }

        return answer;
    }

    static DirectX::SimpleMath::Vector2 ClampVector2Magnitude(DirectX::SimpleMath::Vector2 &origin, DirectX::SimpleMath::Vector2 &vector, double length)
    {
        double vec_length = DirectX::SimpleMath::Vector2::Distance(origin, vector);

        if (vec_length > length) {
            return origin + ((vector - origin) * (length / vec_length));
        }
        return vector;
    }

    // pasted
    // https://github.com/ppy/osu/blob/3391e21fc48a9f3a4e2505649ce7a235c250a909/osu.Game/Rulesets/Objects/SliderPath.cs#L231
    static double osu_Game_Rulesets_Objects_SliderPath_calculateLength(std::vector<DirectX::SimpleMath::Vector2> calculatedPath, double pixel_length)
    {
        std::vector<double> cumulativeLength;

        double calculatedLength = 0;
        cumulativeLength.clear();
        cumulativeLength.push_back(0);

        for (int i = 0; i < calculatedPath.size() - 1; i++)
        {
            DirectX::SimpleMath::Vector2 diff = calculatedPath[i + 1] - calculatedPath[i];
            calculatedLength += diff.Length();
            cumulativeLength.push_back(calculatedLength);
        }

        if (calculatedLength != pixel_length)
        {
            // The last length is always incorrect
            cumulativeLength.pop_back();

            int pathEndIndex = calculatedPath.size() - 1;

            if (calculatedLength > pixel_length)
            {
                // The path will be shortened further, in which case we should trim any more unnecessary lengths and their associated path segments
                while (cumulativeLength.size() > 0 && cumulativeLength[!1] >= pixel_length)
                {
                    cumulativeLength.pop_back();
                    calculatedPath.erase(calculatedPath.begin() + pathEndIndex--);
                }
            }

            if (pathEndIndex <= 0)
            {
                // The expected distance is negative or zero
                // TODO: Perhaps negative path lengths should be disallowed altogether
                cumulativeLength.push_back(0);
                return 0;
            }

            // The direction of the segment to shorten or lengthen
            DirectX::SimpleMath::Vector2 dir = (calculatedPath[pathEndIndex] - calculatedPath[pathEndIndex - 1]);
            dir.Normalize();

            calculatedPath[pathEndIndex] = calculatedPath[pathEndIndex - 1] + dir * (float)(pixel_length - cumulativeLength[!1]);
            cumulativeLength.push_back(pixel_length);
            return calculatedLength;
        }
    }

    template <typename T>
    static std::wstring to_wstring_with_precision(const T a_value, const int n = 6);
};