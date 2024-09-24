#include "pch.h"
#include "OsuGame.h"
#include <Windows.h>
#include "Signatures.h"
#include <shlObj.h>
#include <algorithm>

void osuGame::UnloadGame()
{
    dwOsuPid = 0;
    hOsu = nullptr;
    pBase = NULL;
    pOsuMapTime = NULL;
    pOsuFramedelay = NULL;
    pOsuGameMode = NULL;
}

void osuGame::LoadGame()
{
    if (!hOsu)
        return;

    DWORD32 addr;
    DWORD32 ptr;

    ptr = Signatures::FindPattern(hOsu, Signatures::FRAME_DELAY, Signatures::FRAME_DELAY_MASK, Signatures::FRAME_DELAY_OFFSET, reinterpret_cast<uintptr_t>(hOsu));
    
    //  non-critical.
    if (ptr == NULL)
        OutputDebugStringW(L"fps sig not found!\n");
    else
    {
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD32, nullptr);
        pOsuFramedelay = addr;

        // for init
        ReadProcessMemory(hOsu, LPCVOID(pOsuFramedelay), &osu_fps, sizeof std::double_t, nullptr);
    }

    ptr = Signatures::FindPattern(hOsu, Signatures::MODS, Signatures::MODS_MASK, Signatures::MODS_OFFSET, reinterpret_cast<uintptr_t>(hOsu));

    //  non-critical.
    if (ptr == NULL)
        OutputDebugStringW(L"mod sig not found!\n");
    else
    {
        //https://github.com/Piotrekol/ProcessMemoryDataFinder/blob/master/OsuMemoryDataProvider/OsuMemoryReader.cs#L74 
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD32, nullptr);
        pMods = addr;
    }

    ptr = Signatures::FindPattern(hOsu, Signatures::OSU_BASE, Signatures::OSU_BASE_MASK, Signatures::OSU_BASE_OFFSET, reinterpret_cast<uintptr_t>(hOsu));
    
    //  critical.
    if (ptr == NULL)
    {
        OutputDebugStringW(L"base sig not found!!!! this is catastrophic! exiting!\n");
        exit(1);
    }
    else
    {
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD32, nullptr);
        pBase = ptr;

        /* = uses gamemode offset = */
        /* gamemode */
        ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::GAMEMODE_OFFSET), &addr, sizeof DWORD32, nullptr);
        pOsuGameMode = addr;

        /* Retrys?? */
        ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::GAMEMODE_OFFSET), &addr, sizeof DWORD32, nullptr);
        pRetries = addr + Signatures::RETRYS_PTR_OFFSET[0];

        /* = uses beatmap offset = */
        /* Beatmap?? */
        ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::BEATMAP_DATA_OFFSET), &addr, sizeof DWORD32, nullptr);
        ppBeatmapData = addr;

        ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::TIME_OFFSET), &addr, sizeof DWORD32, nullptr);
        pOsuMapTime = addr + Signatures::TIME_PTR_OFFSET[0];
    }

    // critical.
    ptr = Signatures::FindPattern(hOsu, Signatures::PLAYDATA, Signatures::PLAYDATA_MASK, Signatures::PLAYDATA_OFFSET, 0);
    if (ptr == NULL)
    {
        OutputDebugStringW(L"playdata sig not found! exiting!\n");
        exit(1);
    }   
    else
    {
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
        ppPlayData = addr;
    }

    *bOsuLoaded = true;
    *bOsuLoading = false;

    OutputDebugStringW(L"LoadGame: Exit\n");
}

// https://github.com/Piotrekol/ProcessMemoryDataFinder/blob/master/OsuMemoryDataProvider/OsuMemoryReader.cs
// https://github.com/Piotrekol/ProcessMemoryDataFinder/blob/master/OsuMemoryDataProvider/StructuredOsuMemoryReader.cs
void osuGame::readMemory()
{
    if (!bOsuLoaded)
        return;

    if (pMods != NULL)
        ReadProcessMemory(hOsu, LPCVOID(pMods), &mods, sizeof std::int32_t, nullptr);

    DWORD32 pPlayData = NULL;
    if (ppPlayData != NULL)
    {
        ReadProcessMemory(hOsu, LPCVOID(ppPlayData), &pPlayData, sizeof DWORD32, nullptr);
        bOsuIngame = (pPlayData != NULL);

        DWORD32 pTemp = NULL;
        if (bOsuIngame) 
        {
            // first offset 56 stuff goes here
            // https://github.com/Piotrekol/ProcessMemoryDataFinder/blob/1b48309f87449d9269d289278e41618e32b0213d/OsuMemoryDataProvider/OsuMemoryModels/Abstract/MainPlayerScore.cs
            ReadProcessMemory(hOsu, LPCVOID(pPlayData + 56), &pTemp, sizeof DWORD32, nullptr); // ie 0x38

            ReadProcessMemory(hOsu, LPCVOID(pTemp + 138), &hit300, sizeof std::uint16_t, nullptr);
            ReadProcessMemory(hOsu, LPCVOID(pTemp + 136), &hit100, sizeof std::uint16_t, nullptr);
            ReadProcessMemory(hOsu, LPCVOID(pTemp + 140), &hit50, sizeof std::uint16_t, nullptr);
            ReadProcessMemory(hOsu, LPCVOID(pTemp + 142), &hit300g, sizeof std::uint16_t, nullptr);
            ReadProcessMemory(hOsu, LPCVOID(pTemp + 144), &hit200, sizeof std::uint16_t, nullptr);

            ReadProcessMemory(hOsu, LPCVOID(pTemp + 146), &hitmiss, sizeof std::uint16_t, nullptr);

            ReadProcessMemory(hOsu, LPCVOID(pTemp + 104), &combo_max, sizeof std::uint16_t, nullptr);

            if (pMods == NULL)
            {
                ReadProcessMemory(hOsu, LPCVOID(pTemp + 28), &mods, sizeof DWORD32, nullptr);
            }

            // fixme: shove this into a function pls
            // stolen from https://github.com/Piotrekol/ProcessMemoryDataFinder/blob/95030bba9c5e2de0667c4ae95e6e6b1fbbde3d5c/ProcessMemoryDataFinder/ObjectReader.cs#L132
            // the structure of the array seems to be implemented in a way where
            // [???, 4 bytes] [data ptr, 4 bytes???] [???, 4 bytes] [size, 4 bytes]

            // and where data ptr leads to [???, 4 bytes] [???, 4 bytes that goes 32 -> 64 -> 128(...)] [items, 4 bytes, repeated]

            // "baseAddress"
            DWORD pHitErrorList;
            // read the ptr of list
            ReadProcessMemory(hOsu, LPCVOID(pTemp + 0x38), &pHitErrorList, sizeof DWORD, nullptr);

            // get our list size
            std::int32_t list_size;
            ReadProcessMemory(hOsu, LPCVOID(pHitErrorList + (sizeof std::int32_t * 3)), &list_size, sizeof std::int32_t, nullptr);

            if (list_size > 0) {
                DWORD hit_error_ptr;
                ReadProcessMemory(hOsu, LPCVOID(pHitErrorList + (sizeof std::int32_t)), &hit_error_ptr, sizeof std::int32_t, nullptr);

                std::int32_t* hit_error_buf = new std::int32_t[(size_t)list_size];
                // we skip 8 bytes, i don't know why.
                ReadProcessMemory(hOsu, LPCVOID(hit_error_ptr + 2 * sizeof std::int32_t), hit_error_buf, sizeof std::int32_t * list_size, nullptr);

                // copy into our vector
                hit_errors.clear(); // idk how hard this hits performance, but idk.
                hit_errors.insert(hit_errors.begin(), &hit_error_buf[0], &hit_error_buf[list_size]);

                delete[] hit_error_buf;
            }
            else if (hit_errors.size() > 0 && list_size < 1) {
                hit_errors.clear();
            }

            // first offset 64
            ReadProcessMemory(hOsu, LPCVOID(pPlayData + 0x40), &pTemp, sizeof DWORD32, nullptr);

            // we're using playerhp instead of playerhpsmoothed because lol 
            // playerhpsmoothed is 28 and nonsmoothed is 20
            ReadProcessMemory(hOsu, LPCVOID(pTemp + 28), &player_hp, sizeof double_t, nullptr); // ie 0x1C
        }
    }

    this->readMemoryOnlyFps();

    if (pOsuGameMode != NULL)
    {
        std::int32_t osu_gmode;
        ReadProcessMemory(hOsu, LPCVOID(pOsuGameMode), &osu_gmode, sizeof std::int32_t, nullptr);
        gameMode = static_cast<PlayMode>(osu_gmode);
    }

    DWORD pBeatMapData = NULL;
    if (ppBeatmapData != NULL)
        ReadProcessMemory(hOsu, LPCVOID(ppBeatmapData), &pBeatMapData, sizeof std::int32_t, nullptr);

    // get our osu map time.
    int32_t osu_time;
    ReadProcessMemory(hOsu, LPCVOID(pOsuMapTime), &osu_time, sizeof std::int32_t, nullptr);

    if (loadedMap.loaded)
    {
        try {
            //  the map is restarted. prob a better way to do this exist but /shrug
            if (osuMapTime > osu_time)
            {
                loadedMap.resetIndexes();
            }

            // update our map time
            osuMapTime = osu_time;

            // set our indexes for map timingpoint.
            for (uint32_t i = loadedMap.currentTimeIndex; i < loadedMap.timingpoints.size() && osuMapTime >= loadedMap.getTimingPoint(i)->offset; i++)
            {
                loadedMap.currentBpm = loadedMap.getTimingPoint(i)->getBPM();
                loadedMap.currentSpeed = loadedMap.getTimingPoint(i)->velocity;
                loadedMap.kiai = loadedMap.getTimingPoint(i)->kiai;

                // update our current timingpoint.
                loadedMap.currentTimeIndex = i;

                // if this is not an inherited timing point, update our uninherited time index.
                if (!loadedMap.getTimingPoint(i)->inherited)
                {
                    loadedMap.currentUninheritTimeIndex = i;
                }
            }

            // set our indexes for general objects in the vector.
            for (uint32_t i = loadedMap.currentObjectIndex; i < loadedMap.hitobjects.size() && osuMapTime >= loadedMap.getHitObject(i)->start_time; i++)
            {
                // update our current hitobject index.
                loadedMap.currentObjectIndex = i;

                // if note is new combo, update that index too.
                if (loadedMap.getHitObject(i)->IsNewCombo())
                {
                    loadedMap.newComboIndex = i;
                }
            }

            // update our mania index.
            if (gameMode == PlayMode::MANIA)
            {
                for (size_t row = 0; row < loadedMap.CircleSize; ++row)    
                {
                    // if there's absolutely no notes, ignore
                    if (loadedMap.hitobjects_sorted[row].size() == 0)
                        continue;

                    for (uint32_t i = loadedMap.currentObjectIndex_sorted[row]; i < loadedMap.hitobjects_sorted[row].size() && osuMapTime >= loadedMap.getHitObject(row, i)->start_time; i++)
                    {
                        loadedMap.currentObjectIndex_sorted[row] = i;
                    }
                }
            }

            // get our... bpm beat index?
            beatIndex = static_cast<int>((osuMapTime - loadedMap.getCurrentUninheritedTimingPoint()->offset) / loadedMap.getCurrentUninheritedTimingPoint()->ms_per_beat);
        }
        catch (std::out_of_range)
        {
            loadedMap.resetIndexes();
            bOsuIngame = false;
        }
        catch (std::invalid_argument)
        {
            loadedMap.resetIndexes();
            bOsuIngame = false;
        }
    }
    else
    {
        // still update things that go directly to memory.
        osuMapTime = osu_time;
    }

    /////////

    // https://github.com/Piotrekol/ProcessMemoryDataFinder/blob/master/OsuMemoryDataProvider/OsuMemoryModels/Direct/CurrentBeatmap.cs
    if (pBeatMapData != NULL)
    {
        ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 0xCC), &MemoryBeatMapID, sizeof std::int32_t, nullptr);
        ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 0xD0), &MemoryBeatMapSetID, sizeof std::int32_t, nullptr);

        // strings

        DWORD32 ptr, length;

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 0x80), &ptr, sizeof DWORD32, nullptr))
        {
            MemoryMapString = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryMapString Read Fail!\n");
        }

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 0x78), &ptr, sizeof DWORD32, nullptr))
        {
            MemoryBeatMapFolderName = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryBeatMapFolderName Read Fail!\n");
        }

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 0x94), &ptr, sizeof DWORD32, nullptr))
        {
            MemoryBeatMapFileName = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryBeatMapFileName Read Fail!\n");
        }

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 0x6C), &ptr, sizeof DWORD32, nullptr))
        {
            MemoryBeatMapMD5 = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryBeatMapMD5 Read Fail!\n");
        }
    }
}

void osuGame::readMemoryOnlyFps()
{
    if (!*bOsuLoaded) {
        return;
    }

    if (pOsuFramedelay != NULL) {

        std::chrono::milliseconds epoch = Utilities::time_since_epoch();
        osu_fps_previous = osu_fps;

        ReadProcessMemory(hOsu, LPCVOID(pOsuFramedelay), &osu_fps, sizeof std::double_t, nullptr);

        if (osu_fps - osu_fps_previous > 5.0) {
            last_performance_issue = epoch;
            last_performance_issue_fps_previous = 1000.0 / osu_fps_previous;
            last_performance_issue_fps = 1000.0 / osu_fps;
        }

        if (epoch - osu_fps_avg_last_update > std::chrono::milliseconds(500)) {
            osu_fps_avg_last_update = epoch;
            osu_fps_avg = 1000.0 / osu_fps;
        }
    }
}

void osuGame::CheckMap()
{
    if (!*bOsuLoaded)
        return;

    if (loadedMap.MD5_Hash != MemoryBeatMapMD5)
    {
        //  invalid beatmapfile, skip.
        //  this is validated twice but like i care.
        std::wifstream beatmap_file(GetFolderActual(), std::ifstream::in);

        if (!beatmap_file.good()) {
            OutputDebugStringW(L"GetFolderActual() is invalid! skipping.\n");
            return;
        }

        OutputDebugStringW(L"Loading mapset: ");
        OutputDebugStringW(MemoryBeatMapMD5.c_str());
        OutputDebugStringW(L" ");
        OutputDebugStringW(MemoryBeatMapFileName.c_str());
        OutputDebugStringW(L"\n");
        loadedMap.Unload();

        try
        {
            if (!loadedMap.Parse(GetFolderActual(), MemoryBeatMapMD5))
            {
                loadedMap.Unload();
                OutputDebugStringW(L"Load failed!\n");
                return;
            }
            loadedMap.loaded = true;
            OutputDebugStringW(L"Loaded!\n");
        }
        catch (std::out_of_range *e )
        {
            load_failed = true;
            load_fail_reason = L"out-of-range!";
            OutputDebugStringW(L"Load failed!\n");
            loadedMap.Unload();
        }
        catch (std::out_of_range &e)
        {
            load_failed = true;
            load_fail_reason = L"out-of-range!";
            OutputDebugStringW(L"Load failed!\n");
            loadedMap.Unload();
        }
        catch (std::invalid_argument *e)
        {
            load_failed = true;
            load_fail_reason = L"Invalid Sliders in map!";
            OutputDebugStringW(L"Load failed! - Invalid sliders\n");
            loadedMap.Unload();
        }
        catch (std::exception *e)
        {
            load_failed = true;
            load_fail_reason = L"unknown!";
            OutputDebugStringW(L"Load failed! : ALL EXCEPTION IDK WTF THIS IS \n");
            OutputDebugStringA(e->what());
            loadedMap.Unload();
        }
        catch (std::exception &e)
        {
            load_failed = true;
            load_fail_reason = L"unknown!";
            OutputDebugStringW(L"Load failed! : ALL EXCEPTION IDK WTF THIS IS \n");
            OutputDebugStringA(e.what());
            loadedMap.Unload();
        }
    }
}

void osuGame::CheckLoaded()
{
    if (*bOsuLoading)
    {
        return;
    }

    if (hOsu != nullptr) {
        DWORD exitCode = NULL;
        bool fn = !GetExitCodeProcess(hOsu, &exitCode);
        if (fn || exitCode != STILL_ACTIVE)
        {
            *bOsuLoaded = false;
            UnloadGame();
        }
    }
    else
    {
        if (!*bOsuLoading)
        {
            LoadGameWnd();

            if (hOsu != NULL)
            {
                *bOsuLoading = true;
                std::thread a = LoadGameThread();
                a.detach();
            }
        }
        else
        {
            UnloadGame();
        }
        return;
    }
}