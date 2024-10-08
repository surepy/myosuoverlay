#pragma once
#include "Utilities.h"
#include "Beatmap.h"

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

enum class PlayMode { //osu_common::PlayModes
    STANDARD = 0,
    TAIKO = 1,
    CATCH = 2,
    MANIA = 3,
    UNKNOWN
};

class osuGame {
private:
    // pid of osu!.exe
    DWORD dwOsuPid = 0;

    // handle to osu memory.
    HANDLE hOsu = nullptr;

    // where are the song files?
    std::wstring SongFolderLocation = L"C:\\Users\\sleepy\\AppData\\Local\\osu!\\Songs\\";
public:
    /// <summary>
    /// get osu's process id and handle.
    /// </summary>
    inline void LoadGameWnd()
    {
        dwOsuPid = Utilities::getProcessIDbyName(L"osu!.exe");
        hOsu = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_LIMITED_INFORMATION, false, dwOsuPid);
    }

    void UnloadGame();
    void CheckMap();

    void CheckLoaded();

    void LoadGame();

    std::thread LoadGameThread() { return std::thread([this] { LoadGame(); }); }

    // is osu loading or loaded?
    std::shared_ptr<bool> bOsuLoading = std::make_unique<bool>(false);
    std::shared_ptr<bool> bOsuLoaded = std::make_unique<bool>(false);

    /*
     *  StreamCompanaion shared mapfiles (MOSTLY UNUSED)
    */
    // std::unique_ptr<Utilities::MappingFile> mfOsuPlayPP;

    /*
     *  Raw osu pointers (preferred)
     */
private:
    DWORD pInput = NULL; // ?
    DWORD pOsuFramedelay = NULL;
    DWORD pBase = NULL;
    DWORD pOsuMapTime = NULL;
    DWORD pOsuGameMode = NULL;
    DWORD pMods = NULL;
    DWORD ppBeatmapData = NULL;
    DWORD pRetries = NULL;
    DWORD ppPlayData = NULL;

    // what the fuck is this 
    DWORD pTemp;

public:
    // osu frame time (memory)
    std::double_t osu_fps;
    // osu frame time, previous memory read.
    std::double_t osu_fps_previous;

    // "smoothed" fps update.
    std::double_t osu_fps_avg;
    std::chrono::milliseconds osu_fps_avg_last_update;

    // detected performance issue data
    std::double_t last_performance_issue_fps;
    std::double_t last_performance_issue_fps_previous;
    std::chrono::milliseconds last_performance_issue;

    // osu map time (memory, delayed)
    int osuMapTime = 0;

    // gamemode (memory)
    PlayMode gameMode = PlayMode::STANDARD;

    // gameplay stats
    bool bOsuIngame = false;

    // gameplay stuff (memory)
    std::int32_t mods = 0;
    double player_hp;
    std::vector<int> hit_errors;
    std::uint16_t hit300g;
    std::uint16_t hit300;
    std::uint16_t hit200;
    std::uint16_t hit100;
    std::uint16_t hit50;
    std::uint16_t hitmiss;
    std::uint16_t combo_max = 0;

    // Beatmap data (memory)
    int MemoryBeatMapID = 0;
    int MemoryBeatMapSetID = 0;
    std::wstring MemoryMapString;
    std::wstring MemoryBeatMapFileName = L" ";
    std::wstring MemoryBeatMapFolderName = L" ";
    std::wstring MemoryBeatMapMD5 = L" ";

    // beatmap data (non-memory)
    int beatIndex = 0;

    // if map load fails:
    bool load_failed = false;
    std::wstring load_fail_reason = L" ";

    // map is loaded.
    beatmap loadedMap; 

    // loadedmap shortcuts
    hitobject *getCurrentHitObject() { return this->loadedMap.getCurrentHitObject(); };

    bool hasMod(const Mods &mod)
    {
        return (mods & mod) == mod;
    }

    double GetMiliSecondsFromOsuTime(int time)
    {
        return time / (hasMod(Mods::DoubleTime) || hasMod(Mods::Nightcore) ? 1.0 : 0.77);
    }

    double GetSecondsFromOsuTime(int time)
    {
        return (GetMiliSecondsFromOsuTime(time) / 1000.0);
    }

    int GetActualBPM()
    {
        return static_cast<int>(this->loadedMap.getCurrentTimingPoint()->getBPM() * (hasMod(Mods::DoubleTime) || hasMod(Mods::Nightcore) ? 1.5 : 1));
    }

    int GetActualBPM(int bpm)
    {
        return static_cast<int>(bpm * (hasMod(Mods::DoubleTime) || hasMod(Mods::Nightcore) ? 1.5 : 1));
    }

    DirectX::XMVECTORF32 GetBeatColorGarbage() {
        switch (this->beatIndex % 4)
        {
        case 1:
            return DirectX::Colors::AliceBlue;
        case 2:
            return DirectX::Colors::LightBlue;
        case 3:
            return DirectX::Colors::AliceBlue;
        case 0:
        default:
            return DirectX::Colors::Yellow;
        }
    }


    std::wstring GetFolderActual()
    {
        return SongFolderLocation + MemoryBeatMapFolderName + L"\\" + MemoryBeatMapFileName;
    }

    double getAR_Real_Unclamped(double AR)
    {
        double ar_multiplier = 1;

        if (hasMod(Mods::HardRock))
        {
            ar_multiplier *= 1.4;
        }
        else if (hasMod(Mods::Easy))
        {
            ar_multiplier *= 0.5;
        }

        return AR * ar_multiplier;
    }

    inline double getAR_Real(double AR)
    {
        return std::clamp(getAR_Real_Unclamped(AR), -5.0, hasMod(Mods::DoubleTime) || hasMod(Mods::Nightcore) ? 11.0 : 10.0);
    }

    int GetARDelay(double AR)
    {
        int delay = 450;

        double ar = 10 - getAR_Real(AR);

        if (ar <= 5)
            delay += (ar - 1) * 150;
        else if (ar > 5)
            delay = 1200 + (ar - 5) * 120;

        return std::clamp(delay, 300, 2400);
    }

    void readMemory();
    void readMemoryOnlyFps();
};