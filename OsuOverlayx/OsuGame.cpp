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

    DWORD addr;
    DWORD ptr;

    ptr = Signatures::FindPattern(hOsu, Signatures::FRAME_DELAY, Signatures::FRAME_DELAY_MASK, Signatures::FRAME_DELAY_OFFSET, 0);
    if (ptr == NULL)
        OutputDebugStringW(L"fps sig not found!\n");
    else
    {
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
        pOsuFramedelay = addr;
    }

    ptr = Signatures::FindPattern(hOsu, Signatures::MODS, Signatures::MODS_MASK, Signatures::MODS_OFFSET, 0);
    if (ptr == NULL)
        OutputDebugStringW(L"mod sig not found!\n");
    else
    {
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
        pMods = addr;
    }

    ptr = Signatures::FindPattern(hOsu, Signatures::OSU_BASE, Signatures::OSU_BASE_MASK, Signatures::OSU_BASE_OFFSET, 0);
    if (ptr == NULL)
        OutputDebugStringW(L"base sig not found!\n");
    else
    {
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
        pBase = ptr;

        /* = uses gamemode offset = */
        /* gamemode */
        ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::GAMEMODE_OFFSET), &addr, sizeof DWORD, nullptr);
        pOsuGameMode = addr;

        /* Retrys?? */
        ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::GAMEMODE_OFFSET), &addr, sizeof DWORD, nullptr);
        pRetries = addr + Signatures::RETRYS_PTR_OFFSET[0];

        /* = uses beatmap offset = */
        /* Beatmap?? */
        ReadProcessMemory(hOsu, LPCVOID(ptr + Signatures::BEATMAP_DATA_OFFSET), &addr, sizeof DWORD, nullptr);
        ppBeatmapData = addr;
    }

    ptr = Signatures::FindPattern(hOsu, Signatures::TIME, Signatures::TIME_MASK, Signatures::TIME_OFFSET, 0);
    if (ptr == NULL)
        OutputDebugStringW(L"time sig not found!\n");
    else
    {
        ReadProcessMemory(hOsu, LPCVOID(ptr), &addr, sizeof DWORD, nullptr);
        pOsuMapTime = addr;
    }

    ptr = Signatures::FindPattern(hOsu, Signatures::PLAYDATA, Signatures::PLAYDATA_MASK, Signatures::PLAYDATA_OFFSET, 0);
    if (ptr == NULL)
        OutputDebugStringW(L"base sig not found!\n");
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

    if (pOsuFramedelay != NULL)
        ReadProcessMemory(hOsu, LPCVOID(pOsuFramedelay), &osu_fps, sizeof std::double_t, nullptr);

    if (pMods != NULL)
        ReadProcessMemory(hOsu, LPCVOID(pMods), &mods, sizeof std::int32_t, nullptr);

    if (pOsuGameMode != NULL)
    {
        std::int32_t osu_gmode;
        ReadProcessMemory(hOsu, LPCVOID(pOsuGameMode), &osu_gmode, sizeof std::int32_t, nullptr);
        gameMode = static_cast<PlayMode>(osu_gmode);
    }

    DWORD pBeatMapData = NULL;
    if (ppBeatmapData != NULL)
        ReadProcessMemory(hOsu, LPCVOID(ppBeatmapData), &pBeatMapData, sizeof std::int32_t, nullptr);

    ///////// bad
    int32_t osu_time;
    ReadProcessMemory(hOsu, LPCVOID(pOsuMapTime), &osu_time, sizeof std::int32_t, nullptr);

    if (loadedMap.loaded)
    {
        try {
            //  the map is restarted. prob a better way to do this exist but /shrug
            if (osuMapTime > osu_time)// std::stod(gameStat.mfOsuMapTime->ReadToEnd()) * 1000)
            {
                loadedMap.currentUninheritTimeIndex = 0;
                loadedMap.currentTimeIndex = 0;
                loadedMap.currentObjectIndex = 0;
                loadedMap.newComboIndex = 0;
                for (uint32_t &i : loadedMap.currentObjectIndex_sorted)
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
            for (uint32_t &i : loadedMap.currentObjectIndex_sorted)
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
            for (uint32_t &i : loadedMap.currentObjectIndex_sorted)
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

        DWORD ptr, length;

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 124), &ptr, sizeof DWORD, nullptr))
        {
            MemoryMapString = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryMapString Read Fail!\n");
        }

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 140), &ptr, sizeof DWORD, nullptr))
        {
            MemoryBeatMapFileName = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryBeatMapFileName Read Fail!\n");
        }

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 116), &ptr, sizeof DWORD, nullptr))
        {
            MemoryBeatMapFolderName = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryBeatMapFolderName Read Fail!\n");
        }

        if (ReadProcessMemory(hOsu, LPCVOID(pBeatMapData + 108), &ptr, sizeof DWORD, nullptr))
        {
            MemoryBeatMapMD5 = Utilities::ReadWStringFromMemory(hOsu, ptr);
        }
        else
        {
            OutputDebugStringW(L"MemoryBeatMapMD5 Read Fail!\n");
        }
    }

    DWORD pPlayData = NULL;
    if (ppPlayData != NULL)
    {
        ReadProcessMemory(hOsu, LPCVOID(ppPlayData), &pPlayData, sizeof DWORD, nullptr);
        bOsuIngame = (pPlayData != NULL);
    }
}

void osuGame::CheckMap()
{
    if (!*bOsuLoaded)
        return;

    if (loadedMap.BeatMapID != MemoryBeatMapID)
    {
        OutputDebugStringW(L"Loading mapset: ");
        OutputDebugStringW(std::to_wstring(MemoryBeatMapID).c_str());
        OutputDebugStringW(L" ");
        OutputDebugStringW(MemoryBeatMapFileName.c_str());
        OutputDebugStringW(L"\n");
        loadedMap.Unload();
        //loadedMap.BeatMapID = MemoryBeatMapID;
        try
        {
            if (!loadedMap.Parse(GetFolderActual()))
            {
                loadedMap.Unload();
                OutputDebugStringW(L"Load failed!\n");
                return;
            }
            loadedMap.loaded = true;
            OutputDebugStringW(L"Loaded!\n");
        }
        catch (std::out_of_range)
        {
            OutputDebugStringW(L"Load failed!\n");
            loadedMap.Unload();
        }
        catch (std::exception &e)
        {
            OutputDebugStringW(L"Load failed! : ALL EXCEPTION IDK WTF THIS IS \n");
            OutputDebugStringA(e.what());
            loadedMap.Unload();
        }
    }
}