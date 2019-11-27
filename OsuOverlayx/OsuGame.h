#pragma once
#include <Windows.h>
#include "Utilities.h"
#include "Signatures.h"
#include "Beatmap.h"

enum gameMode {
    STANDARD = 0,
    TAIKO = 1,
    CATCH = 2,
    MANIA = 3,
    UNKNOWN
};

struct osuGame {
    DWORD dwOsuPid;
    HANDLE hOsu;

    void LoadGame()
    {
        dwOsuPid = Utilities::getProcessIDbyName(L"osu!.exe");
        hOsu = OpenProcess(PROCESS_VM_READ, false, dwOsuPid);

        InitMemory();
    };

    void CheckLoaded()
    {
        DWORD exitCode = NULL;

        if (dwOsuPid == Utilities::getProcessIDbyName(L"osu!.exe") && hOsu)
            return;

        if (!GetExitCodeProcess(hOsu, &exitCode) || exitCode != STILL_ACTIVE)
        {
            hOsu = nullptr;
            LoadGame();
        }
    };

    void InitMemory()
    {
        if (!hOsu)
            return;

        DWORD addr;
        DWORD ptr;

        //Signatures::TIME
        // time
        ptr = Signatures::FindPattern(hOsu, Signatures::TIME, Signatures::TIME_MASK, Signatures::TIME_OFFSET, 0);
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
        pOsuMapTime = addr;

        ptr = Signatures::FindPattern(hOsu, Signatures::FRAME_DELAY, Signatures::FRAME_DELAY_MASK, Signatures::FRAME_DELAY_OFFSET, 0);
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
        pOsuFramedelay = addr;

        ptr = Signatures::FindPattern(hOsu, Signatures::GAMEMODE_GLOBAL, Signatures::GAMEMODE_GLOBAL_MASK, Signatures::GAMEMODE_GLOBAL_OFFSET, 0);
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
        pOsuGlobalGameMode = addr;
    }

    /*
     *  StreamCompanaion shared mapfiles
     */
    std::unique_ptr<Utilities::MappingFile> mfCursor;
    std::unique_ptr<Utilities::MappingFile> mfKey;
    std::unique_ptr<Utilities::MappingFile> mfOsuPlayHits;
    std::unique_ptr<Utilities::MappingFile> mfOsuPlayPP;
    std::unique_ptr<Utilities::MappingFile> mfOsuPlayHP;
    std::unique_ptr<Utilities::MappingFile> mfKeyStat;
    std::unique_ptr<Utilities::MappingFile> mfOsuMapTime;
    std::unique_ptr<Utilities::MappingFile> mfOsuFileLoc;
    std::unique_ptr<Utilities::MappingFile> mfOsuKiaiStat;
    std::unique_ptr<Utilities::MappingFile> mfCurrentModsStr;
    std::unique_ptr<Utilities::MappingFile> mfCurrentOsuGMode;

    /*
     *  Raw osu pointers (preferred)
     */

    DWORD pOsuMapTime;
    DWORD pOsuFramedelay;
    DWORD pOsuGlobalGameMode;

    std::chrono::milliseconds currentTime;
    std::chrono::milliseconds previousDistTime;

    beatmap currentMap;
    std::vector<std::chrono::milliseconds> clicks;

    hitobject *getCurrentHitObject() { return this->currentMap.getCurrentHitObject(); };

    int osuMapTime; // pointer?
    gameMode gameMode = gameMode::STANDARD;
    bool bOsuIngame = false;  //  idk if i will use

    DirectX::SimpleMath::Vector2 cursorLocation;
    double cursorSpeed = 0;

    bool clickx = false, clicky = false;
    int clickCounter = 0;
    int beatIndex = 0;
    bool bStreamCompanionRunning = true;
};