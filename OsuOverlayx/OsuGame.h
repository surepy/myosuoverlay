#pragma once
#include <Windows.h>
#include "Utilities.h"
#include "Signatures.h"
#include "Beatmap.h"
#include <shlObj.h>

// refactor again soon.

enum MatchScoringTypes {		// Token: 0x040039BF RID: 14783
    Score,
    // Token: 0x040039C0 RID: 14784
    Accuracy,
    // Token: 0x040039C1 RID: 14785
    Combo,
    // Token: 0x040039C2 RID: 14786
    ScoreV2
};

enum Mods
{
    // Token: 0x040039CC RID: 14796
    None = 0,
    // Token: 0x040039CD RID: 14797
    NoFail = 1,
    // Token: 0x040039CE RID: 14798
    Easy = 2,
    // Token: 0x040039CF RID: 14799
    TouchDevice = 4,
    // Token: 0x040039D0 RID: 14800
    Hidden = 8,
    // Token: 0x040039D1 RID: 14801
    HardRock = 16,
    // Token: 0x040039D2 RID: 14802
    SuddenDeath = 32,
    // Token: 0x040039D3 RID: 14803
    DoubleTime = 64,
    // Token: 0x040039D4 RID: 14804
    Relax = 128,
    // Token: 0x040039D5 RID: 14805
    HalfTime = 256,
    // Token: 0x040039D6 RID: 14806
    Nightcore = 512,
    // Token: 0x040039D7 RID: 14807
    Flashlight = 1024,
    // Token: 0x040039D8 RID: 14808
    Autoplay = 2048,
    // Token: 0x040039D9 RID: 14809
    SpunOut = 4096,
    // Token: 0x040039DA RID: 14810
    Relax2 = 8192,
    // Token: 0x040039DB RID: 14811
    Perfect = 16384,
    // Token: 0x040039DC RID: 14812
    Key4 = 32768,
    // Token: 0x040039DD RID: 14813
    Key5 = 65536,
    // Token: 0x040039DE RID: 14814
    Key6 = 131072,
    // Token: 0x040039DF RID: 14815
    Key7 = 262144,
    // Token: 0x040039E0 RID: 14816
    Key8 = 524288,
    // Token: 0x040039E1 RID: 14817
    FadeIn = 1048576,
    // Token: 0x040039E2 RID: 14818
    Random = 2097152,
    // Token: 0x040039E3 RID: 14819
    Cinema = 4194304,
    // Token: 0x040039E4 RID: 14820
    Target = 8388608,
    // Token: 0x040039E5 RID: 14821
    Key9 = 16777216,
    // Token: 0x040039E6 RID: 14822
    KeyCoop = 33554432,
    // Token: 0x040039E7 RID: 14823
    Key1 = 67108864,
    // Token: 0x040039E8 RID: 14824
    Key3 = 134217728,
    // Token: 0x040039E9 RID: 14825
    Key2 = 268435456,
    // Token: 0x040039EA RID: 14826
//    ScoreV2 = 536870912,
    // Token: 0x040039EB RID: 14827
    Mirror = 1073741824,
    // Token: 0x040039EC RID: 14828
    KeyMod = 521109504,
    // Token: 0x040039ED RID: 14829
    FreeModAllowed = 1595913403,
    // Token: 0x040039EE RID: 14830
    ScoreIncreaseMods = 1049688
};

enum PlayMode { //osu_common::PlayModes
    STANDARD = 0,
    TAIKO = 1,
    CATCH = 2,
    MANIA = 3,
    UNKNOWN
};

struct osuGame {
    DWORD dwOsuPid;
    HANDLE hOsu;

    void CheckGame()
    {
        dwOsuPid = Utilities::getProcessIDbyName(L"osu!.exe");
        hOsu = OpenProcess(PROCESS_VM_READ, false, dwOsuPid);
    };

    void UnloadGame()
    {
        dwOsuPid = 0;
        hOsu = nullptr;
        pOsuMapTime = NULL;
        pOsuFramedelay = NULL;
        pOsuGameMode = NULL;
    }

    void CheckLoaded()
    {
        DWORD exitCode = NULL;

        if (dwOsuPid == Utilities::getProcessIDbyName(L"osu!.exe") && hOsu)
            return;

        if (!GetExitCodeProcess(hOsu, &exitCode) || exitCode != STILL_ACTIVE)
        {
            bOsuLoaded = false;
            UnloadGame();
        }
    };

    void LoadGame()
    {
        if (!hOsu || bOsuLoading)
            return;

        bOsuLoading = true;

        DWORD addr;
        DWORD ptr;

        //Signatures::TIME
        // time
        ptr = Signatures::FindPattern(hOsu, Signatures::FRAME_DELAY, Signatures::FRAME_DELAY_MASK, Signatures::FRAME_DELAY_OFFSET, 0);
        if (ptr == NULL)
            OutputDebugStringW(L"fps sig not found!\n");
        else
        {
            ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
            pOsuFramedelay = addr;
        }

        ptr = Signatures::FindPattern(hOsu, Signatures::OSU_BASE, Signatures::OSU_BASE_MASK, Signatures::OSU_BASE_OFFSET, 0);
        if (ptr == NULL)
            OutputDebugStringW(L"base sig not found!\n");
        else
        {
            ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
            pBase = addr;

            /* = uses gamemode offset = */
            /* gamemode */
            ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::GAMEMODE_OFFSET), &addr, sizeof DWORD, nullptr);
            pOsuGameMode = addr;

            /* Retrys?? */
            ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::GAMEMODE_OFFSET), &addr, sizeof DWORD, nullptr);
            pTemp = addr + Signatures::RETRYS_PTR_OFFSET[0];

            /* = uses beatmap offset = */
            /* Beatmap?? */
            ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::BEATMAP_DATA_OFFSET), &addr, sizeof DWORD, nullptr);
            pBeatmapData = addr;

            /* */
            ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::BEATMAP_DATA_OFFSET), &addr, sizeof DWORD, nullptr);
            pBeatmapData = addr;
        }

        ptr = Signatures::FindPattern(hOsu, Signatures::TIME, Signatures::TIME_MASK, Signatures::TIME_OFFSET, 0);
        if (ptr == NULL)
            OutputDebugStringW(L"time sig not found!\n");
        else
        {
            ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
            pOsuMapTime = addr;
        }

        bOsuLoaded = true;
        bOsuLoading = false;
    }

    std::thread LoadGameThread()
    {
        return std::thread([this] { LoadGame(); });
    }

    /*
        c# code in c++;
    */
    void loadOsuIPC()
    {
        std::wstring osuPath;
        PWSTR osuPathtmp;
        if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, NULL, &osuPathtmp) == E_FAIL)
        {
            OutputDebugStringW(L"osu ipc load failed: appdata not found?");
            return;
        }

        osuPath = std::wstring(osuPathtmp) + L"\\";
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

    bool bOsuLoading = false;
    bool bOsuLoaded = false;

    DWORD pInput; // ?
    DWORD pOsuFramedelay;
    DWORD pBase;
    DWORD pOsuMapTime;
    DWORD pOsuGameMode;
    DWORD pMods;
    DWORD pBeatmapData;
    DWORD pTemp;

    std::chrono::milliseconds currentTime;
    std::chrono::milliseconds previousDistTime;

    beatmap currentMap;
    std::vector<std::chrono::milliseconds> clicks;

    hitobject *getCurrentHitObject() { return this->currentMap.getCurrentHitObject(); };

    std::int32_t mods;

    bool hasMod(const Mods &mod)
    {
        return (mods & mod) == mod;
    }

    double GetSecondsFromOsuTime(int time)
    {
        return (time / (hasMod(Mods::DoubleTime) || hasMod(Mods::Nightcore) ? 1.0 : 0.77) / 1000.0);
    }

    int GetActualBPM(int bpm)
    {
        return static_cast<int>(bpm * (hasMod(Mods::DoubleTime) || hasMod(Mods::Nightcore) ? 1.5 : 1));
    }

    int osuMapTime; // pointer?
    PlayMode gameMode = PlayMode::STANDARD;
    bool bOsuIngame = false;  //  idk if i will use

    DirectX::SimpleMath::Vector2 cursorLocation;
    double cursorSpeed = 0;

    bool clickx = false, clicky = false;
    int clickCounter = 0;
    int beatIndex = 0;
    bool bStreamCompanionRunning = true;
};