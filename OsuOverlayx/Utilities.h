#pragma once
#include <Windows.h>
#include <Tlhelp32.h>

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
};