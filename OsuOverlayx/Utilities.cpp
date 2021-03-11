#include "pch.h"
#include "Utilities.h"
#include <Tlhelp32.h>
#include <string>
#include <iomanip>
#include <strsafe.h>
#include <iostream>
#include "Beatmap.h"

/// <summary>
/// mapping file (string),
/// a way to share string across programs, pretty helpful.
/// </summary>
struct Utilities::MappingFile
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

std::wstring Utilities::s2ws(const std::string& str)
{
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

const DWORD Utilities::getProcessIDbyName(const wchar_t* name) {
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
}

std::wstring Utilities::ReadWStringFromMemory(HANDLE hnd, DWORD ptr) {
    DWORD length;
    std::wstring str;

    if (ptr == NULL)
        return L" ";

    ReadProcessMemory(hnd, LPCVOID(ptr + 4), &length, sizeof DWORD, nullptr);
    // https://github.com/Piotrekol/ProcessMemoryDataFinder/blob/3242645aebcf76b33337f516b363ac67980fb873/ProcessMemoryDataFinder/API/Sig.cs#L241
    // in here, it causes std::bad_alloc.

    if (length > 131072)
    {
        OutputDebugString(L"ReadWStringFromMemory: length over 131072?\n");
        return std::wstring(L" ");
    }

    wchar_t* buf = new wchar_t[(size_t)length];
    ReadProcessMemory(hnd, LPCVOID(ptr + 8), buf, (size_t)(length * sizeof(wchar_t)), nullptr);
    str = std::wstring(buf, length);
    delete[] buf;

    return str;
}

/// <summary>
/// calculates the position between p1 and p2 depending on completion.
/// </summary>
/// <param name="p1">first position</param>
/// <param name="p2">second position</param>
/// <param name="completion">number between 0 ~ 1 inclusive.</param>
/// <returns></returns>
DirectX::SimpleMath::Vector2 Utilities::object_completion(const DirectX::SimpleMath::Vector2& p1, const DirectX::SimpleMath::Vector2& p2, double completion)
{
    return p1 + ((p2 - p1) * completion);
}

/// <summary>
/// how much % have we gone from start_time (0.0) to end_time (1.0)?
/// </summary>
/// <param name="osu_time">osu's current time.</param>
/// <param name="start_time"></param>
/// <param name="end_time"></param>
/// <returns></returns>
double Utilities::time_completion_percent(double osu_time, double start_time, double end_time)
{
    return std::clamp(1.0 + ((osu_time - end_time) / (end_time - start_time)), 0.0, 1.0);
}

double Utilities::osu_Game_Rulesets_Objects_SliderPath_calculateLength(std::vector<DirectX::SimpleMath::Vector2> calculatedPath, double pixel_length) {
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

std::wstring Utilities::to_wstring_with_precision(const double a_value, const int n)
{
    std::wstringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

