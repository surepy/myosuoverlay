#include "pch.h"
#include "OsuGame.h"

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
    }

    ptr = Signatures::FindPattern(hOsu, Signatures::MODS, Signatures::MODS_MASK, Signatures::MODS_OFFSET, reinterpret_cast<uintptr_t>(hOsu));

    //  non-critical.
    if (ptr == NULL)
        OutputDebugStringW(L"mod sig not found!\n");
    else
    {
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
            ReadProcessMemory(hOsu, LPCVOID(pPlayData + 56), &pTemp, sizeof DWORD32, nullptr);

            ReadProcessMemory(hOsu, LPCVOID(pTemp + 138), &hit300, sizeof std::uint16_t, nullptr);
            ReadProcessMemory(hOsu, LPCVOID(pTemp + 136), &hit100, sizeof std::uint16_t, nullptr);
            ReadProcessMemory(hOsu, LPCVOID(pTemp + 140), &hit50, sizeof std::uint16_t, nullptr);
            ReadProcessMemory(hOsu, LPCVOID(pTemp + 146), &hitmiss, sizeof std::uint16_t, nullptr);

            ReadProcessMemory(hOsu, LPCVOID(pTemp + 104), &combo_max, sizeof std::uint16_t, nullptr);

            if (pMods == NULL)
            {
                ReadProcessMemory(hOsu, LPCVOID(pTemp + 28), &mods, sizeof DWORD32, nullptr);
            }

            // first offset 64
            ReadProcessMemory(hOsu, LPCVOID(pPlayData + 64), &pTemp, sizeof DWORD32, nullptr);

            // we're using playerhp instead of playerhpsmoothed because lol 
            // playerhpsmoothed is 28 and nonsmoothed is 20
            ReadProcessMemory(hOsu, LPCVOID(pTemp + 28), &player_hp, sizeof double_t, nullptr);
        
        }
    }

    if (pOsuFramedelay != NULL)
        ReadProcessMemory(hOsu, LPCVOID(pOsuFramedelay), &osu_fps, sizeof std::double_t, nullptr);


    if (pOsuGameMode != NULL)
    {
        std::int32_t osu_gmode;
        ReadProcessMemory(hOsu, LPCVOID(pOsuGameMode), &osu_gmode, sizeof std::int32_t, nullptr);
        gameMode = static_cast<PlayMode>(osu_gmode);
    }

    DWORD pBeatMapData = NULL;
    if (ppBeatmapData != NULL)
        ReadProcessMemory(hOsu, LPCVOID(ppBeatmapData), &pBeatMapData, sizeof std::int32_t, nullptr);

    // bad
    int32_t osu_time;
    ReadProcessMemory(hOsu, LPCVOID(pOsuMapTime), &osu_time, sizeof std::int32_t, nullptr);

    if (loadedMap.loaded)
    {
        try {
            //  the map is restarted. prob a better way to do this exist but /shrug
            if (osuMapTime > osu_time)
            {
                loadedMap.currentUninheritTimeIndex = 0;
                loadedMap.currentTimeIndex = 0;
                loadedMap.currentObjectIndex = 0;
                loadedMap.newComboIndex = 0;
                for (size_t &i : loadedMap.currentObjectIndex_sorted)
                {
                    i = 0;
                }
            }

            osuMapTime = osu_time;

            if (osuMapTime && osuMapTime > 0) {
                for (uint32_t i = loadedMap.currentTimeIndex; i < loadedMap.timingpoints.size() && osuMapTime >= static_cast<int>(loadedMap.timingpoints.at(i).offset); i++)
                {
                    loadedMap.currentBpm = static_cast<int>(60000 / loadedMap.timingpoints.at(i).ms_per_beat);
                    loadedMap.currentSpeed = loadedMap.timingpoints.at(i).velocity;
                    loadedMap.kiai = loadedMap.timingpoints.at(i).kiai;
                    loadedMap.currentTimeIndex = i;
                    if (!loadedMap.timingpoints.at(i).inherited)
                    {
                        loadedMap.currentUninheritTimeIndex = i;
                    }
                }
            }
            else
            {
                loadedMap.currentTimeIndex = 0;
                loadedMap.currentUninheritTimeIndex = 0;
            }

            for (uint32_t i = loadedMap.currentObjectIndex; i < loadedMap.hitobjects.size() && osuMapTime >= loadedMap.hitobjects[i].start_time; i++)
            {
                loadedMap.currentObjectIndex = i;
                if (loadedMap.hitobjects.at(i).IsNewCombo())
                {
                    loadedMap.newComboIndex = i;
                }
            }
            if (gameMode == PlayMode::MANIA)
            {
                for (int i = 0; i < 10; ++i)    // 10 because hardcoded row lol
                {
                    if (loadedMap.hitobjects_sorted[i].size() == 0)
                        break;

                    for (uint32_t a = loadedMap.currentObjectIndex_sorted[i]; a < loadedMap.hitobjects_sorted[i].size() && osuMapTime >= loadedMap.hitobjects_sorted[i].at(a).start_time; a++)
                    {
                        loadedMap.currentObjectIndex_sorted[i] = a;
                    }
                }
            }

            beatIndex = static_cast<int>((osuMapTime - static_cast<int>(loadedMap.timingpoints.at(loadedMap.currentUninheritTimeIndex).offset)) / loadedMap.timingpoints.at(loadedMap.currentUninheritTimeIndex).ms_per_beat);
        }
        catch (std::out_of_range)
        {
            for (size_t &i : loadedMap.currentObjectIndex_sorted)
            {
                i = 0;
            }
            loadedMap.currentUninheritTimeIndex = 0;
            loadedMap.currentTimeIndex = 0;
            loadedMap.currentObjectIndex = 0;
            loadedMap.newComboIndex = 0;
            bOsuIngame = false;
        }
        catch (std::invalid_argument)
        {
            for (size_t &i : loadedMap.currentObjectIndex_sorted)
            {
                i = 0;
            }
            loadedMap.currentUninheritTimeIndex = 0;
            loadedMap.currentTimeIndex = 0;
            loadedMap.currentObjectIndex = 0;
            loadedMap.newComboIndex = 0;
            bOsuIngame = false;
        }
    }
    else
    {
        // still update things that go directly to memory.
        osuMapTime = osu_time;
    }

    /////////

    if (pBeatMapData != NULL)
    {
        // offsets are https://github.com/Piotrekol/ProcessMemoryDataFinder/blob/3a48045f9686075d67c9533a06f34d98a285afaf/OsuMemoryDataProvider/OsuMemoryReader.cs#L81
        ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 196), &MemoryBeatMapID, sizeof std::int32_t, nullptr);
        ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 200), &MemoryBeatMapSetID, sizeof std::int32_t, nullptr);

        // strings

        DWORD32 ptr, length;

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 124), &ptr, sizeof DWORD32, nullptr))
        {
            MemoryMapString = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryMapString Read Fail!\n");
        }

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 140), &ptr, sizeof DWORD32, nullptr))
        {
            MemoryBeatMapFileName = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryBeatMapFileName Read Fail!\n");
        }

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 116), &ptr, sizeof DWORD32, nullptr))
        {
            MemoryBeatMapFolderName = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryBeatMapFolderName Read Fail!\n");
        }

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 108), &ptr, sizeof DWORD32, nullptr))
        {
            MemoryBeatMapMD5 = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryBeatMapMD5 Read Fail!\n");
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