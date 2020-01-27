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

    static int64_t factorial(int n)
    {
        if (n == 0)
            return 1;
        else
            return n * factorial(n - 1);
    }

    static double binomialCoeff(int n, int k)
    {
        return ((double)factorial(n) / (double)(factorial((n - k)) * factorial(k)));
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

    /*static DirectX::SimpleMath::Vector2 getBezierPoint()
    {
    }*/
    inline static DirectX::SimpleMath::Vector2 SliderCurveToVector2(slidercurve point)
    {
        return DirectX::SimpleMath::Vector2(point.x, point.y);
    }

    static DirectX::SimpleMath::Vector2 ClampVector2Magnitude(DirectX::SimpleMath::Vector2 &origin, DirectX::SimpleMath::Vector2 &vector, double length)
    {
        double vec_length = DirectX::SimpleMath::Vector2::Distance(origin, vector);

        if (vec_length > length) {
            return origin + ((vector - origin) * (length / vec_length));
        }
        return vector;
    }

    template <typename T>
    static std::wstring to_wstring_with_precision(const T a_value, const int n = 6);
};