//
// Game.cpp
//

#include "pch.h"
#include "Signatures.h"
#include <thread>
#include <future>

extern void ExitGame();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

template <typename T>
std::wstring Utilities::to_wstring_with_precision(const T a_value, const int n)
{
    std::wstringstream out;
    out.precision(n);
    out << std::fixed << a_value;
    return out.str();
}

Overlay::Overlay() noexcept :
    m_window(nullptr),
    m_outputWidth(800),
    m_outputHeight(600),
    m_featureLevel(D3D_FEATURE_LEVEL_9_1)
{
}

// Initialize the Direct3D resources required to run.
void Overlay::Initialize(HWND window, int width, int height)
{
    m_window = window;
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateDevice();

    CreateResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
}

void Overlay::SetName(std::wstring name)
{
    user = name;
}

bool Overlay::loadMap(osuGame &gameStat)  //  refactor todo?
{
    std::wstring osuMap = static_cast<std::wstring>(gameStat.mfOsuFileLoc->ReadToEnd()); // TODO get directly from osu
    if (gameStat.currentMap.loadedMap.compare(osuMap) != 0)
    {
        gameStat.bOsuIngame = false;
        gameStat.currentMap.Unload();
        try
        {
            if (!gameStat.currentMap.Parse(osuMap))
            {
                return false;
            }
            gameStat.currentMap.loadedMap = osuMap;
            OutputDebugStringW(L"Loaded!\n");
        }
        catch (std::out_of_range)
        {
            OutputDebugStringW(L"Load failed!\n");
            return false;
        }
    }
    return true;
}

// Executes the basic game loop.
void Overlay::Tick(osuGame &gameStat)
{
    gameStat.CheckGame();
    gameStat.CheckLoaded();

    //std::future<bool> mapLoaded = std::async(std::launch::async, &this->loadMap, this, gameStat);
    bool mapLoaded = loadMap(gameStat);
    gameStat.currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
        );

    gameStat.bStreamCompanionRunning = FindWindow(NULL, L"osu!StreamCompanion by Piotrekol") != 0;

    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    /*
     *  Cursor location
     *
     *  This code is terrible
     *
     */
    POINT cursorLocationCurrent;
    if (GetCursorPos(&cursorLocationCurrent))
    {
        // this runs every 100 msecond
        if ((gameStat.previousDistTime + std::chrono::milliseconds(50)) < gameStat.currentTime)
        {
            double dist = std::sqrt(std::pow((cursorLocationCurrent.x - gameStat.cursorLocation.x), 2) + std::pow((cursorLocationCurrent.y - gameStat.cursorLocation.y), 2));
            gameStat.cursorSpeed = dist * 10; // 1 second displacement

            //  72.f + gameStat.currentMap.hitobjects[gameStat.currentMap.currentObjectIndex + 1].y * 1.5f

            gameStat.cursorLocation = DirectX::SimpleMath::Vector2(static_cast<float>(cursorLocationCurrent.x), static_cast<float>(cursorLocationCurrent.y));

            //  set previous time
            gameStat.previousDistTime = gameStat.currentTime;
        }
    }

    /*
     *  Key BPM or kps + data
     */
     // remove old data
    for (std::uint32_t i = 0; i < gameStat.clicks.size(); i++) {
        if (gameStat.clicks[i] + std::chrono::milliseconds(1000) < gameStat.currentTime)
        {
            gameStat.clicks.erase(gameStat.clicks.begin() + i);
        }
    }
    // x click data
    if ((GetAsyncKeyState(0x58) || GetAsyncKeyState(VK_NUMPAD2)) && !gameStat.clickx)
    {
        gameStat.clicks.push_back(gameStat.currentTime);
        gameStat.clickCounter++;
        gameStat.clickx = true;
    }
    else if ((GetAsyncKeyState(0x58) || GetAsyncKeyState(VK_NUMPAD2)) && gameStat.clickx)
    {
    }
    else
    {
        gameStat.clickx = false;
    }
    // y click data
    if ((GetAsyncKeyState(0x5A) || GetAsyncKeyState(VK_NUMPAD3)) && !gameStat.clicky)
    {
        gameStat.clicks.push_back(gameStat.currentTime);
        gameStat.clickCounter++;
        gameStat.clicky = true;
    }
    else if ((GetAsyncKeyState(0x5A) || GetAsyncKeyState(VK_NUMPAD3)) && gameStat.clicky)
    {
    }
    else
    {
        gameStat.clicky = false;
    }

    std::int32_t osu_time;
    ReadProcessMemory(gameStat.hOsu, LPCVOID(gameStat.pOsuMapTime), &osu_time, sizeof std::int32_t, nullptr);

    std::double_t osu_fps;
    ReadProcessMemory(gameStat.hOsu, LPCVOID(gameStat.pOsuFramedelay), &osu_fps, sizeof std::double_t, nullptr);

    ReadProcessMemory(gameStat.hOsu, LPCVOID(gameStat.pMods), &gameStat.mods, sizeof std::int32_t, nullptr);

    if (mapLoaded)
    {
        try {
            //  the map is restarted. prob a better way to do this exist but /shrug
            if (gameStat.osuMapTime > osu_time)// std::stod(gameStat.mfOsuMapTime->ReadToEnd()) * 1000)
            {
                gameStat.currentMap.currentUninheritTimeIndex = 0;
                gameStat.currentMap.currentTimeIndex = 0;
                gameStat.currentMap.currentObjectIndex = 0;
                gameStat.currentMap.newComboIndex = 0;
                for (uint32_t &i : gameStat.currentMap.currentObjectIndex_sorted)
                {
                    i = 0;
                }
            }

            //streamcompanion legacy
            //= static_cast<int>(std::stod(gameStat.mfOsuMapTime->ReadToEnd()) * 1000);
            gameStat.osuMapTime = osu_time;

            if (gameStat.osuMapTime && gameStat.osuMapTime > 0) {
                for (uint32_t i = gameStat.currentMap.currentTimeIndex; i < gameStat.currentMap.timingpoints.size() && gameStat.osuMapTime >= static_cast<int>(gameStat.currentMap.timingpoints.at(i).offset); i++)
                {
                    gameStat.currentMap.currentBpm = static_cast<int>(60000 / gameStat.currentMap.timingpoints.at(i).ms_per_beat);
                    gameStat.currentMap.currentSpeed = gameStat.currentMap.timingpoints.at(i).velocity;
                    gameStat.currentMap.kiai = gameStat.currentMap.timingpoints.at(i).kiai;
                    gameStat.currentMap.currentTimeIndex = i;
                    if (!gameStat.currentMap.timingpoints.at(i).inherited)
                    {
                        gameStat.currentMap.currentUninheritTimeIndex = i;
                    }
                }
            }
            else
            {
                gameStat.currentMap.currentTimeIndex = 0;
                gameStat.currentMap.currentUninheritTimeIndex = 0;
            }

            for (uint32_t i = gameStat.currentMap.currentObjectIndex; i < gameStat.currentMap.hitobjects.size() && gameStat.osuMapTime >= gameStat.currentMap.hitobjects[i].start_time; i++)
            {
                gameStat.currentMap.currentObjectIndex = i;
                if (gameStat.currentMap.hitobjects.at(i).IsNewCombo())
                {
                    gameStat.currentMap.newComboIndex = i;
                }
            }
            if (gameStat.gameMode == PlayMode::MANIA)
            {
                for (int i = 0; i < 10; ++i)    // 10 because hardcoded row lol
                {
                    if (gameStat.currentMap.hitobjects_sorted[i].size() == 0)
                        break;

                    for (uint32_t a = gameStat.currentMap.currentObjectIndex_sorted[i]; a < gameStat.currentMap.hitobjects_sorted[i].size() && gameStat.osuMapTime >= gameStat.currentMap.hitobjects_sorted[i].at(a).start_time; a++)
                    {
                        gameStat.currentMap.currentObjectIndex_sorted[i] = a;
                    }
                }
            }

            gameStat.beatIndex = static_cast<int>((gameStat.osuMapTime - static_cast<int>(gameStat.currentMap.timingpoints.at(gameStat.currentMap.currentUninheritTimeIndex).offset)) / gameStat.currentMap.timingpoints.at(gameStat.currentMap.currentUninheritTimeIndex).ms_per_beat);

            //

            // for in-game detection; i'll figure out a better way.
            gameStat.bOsuIngame = !(std::wstring(gameStat.mfOsuPlayHP->ReadToEnd()).size() == 3);

            //
        }
        catch (std::out_of_range)
        {
            for (uint32_t &i : gameStat.currentMap.currentObjectIndex_sorted)
            {
                i = 0;
            }
            gameStat.currentMap.currentUninheritTimeIndex = 0;
            gameStat.currentMap.currentTimeIndex = 0;
            gameStat.currentMap.currentObjectIndex = 0;
            gameStat.currentMap.newComboIndex = 0;
            gameStat.bOsuIngame = false;
        }
        catch (std::invalid_argument)
        {
            for (uint32_t &i : gameStat.currentMap.currentObjectIndex_sorted)
            {
                i = 0;
            }
            gameStat.currentMap.currentUninheritTimeIndex = 0;
            gameStat.currentMap.currentTimeIndex = 0;
            gameStat.currentMap.currentObjectIndex = 0;
            gameStat.currentMap.newComboIndex = 0;
            gameStat.bOsuIngame = false;
        }
    }
    else
    {
        // still update things that go directly to memory.
        gameStat.osuMapTime = osu_time;
    }

    if (gameStat.bOsuIngame)
    {
        std::int32_t osu_gmode;
        ReadProcessMemory(gameStat.hOsu, LPCVOID(gameStat.pOsuGlobalGameMode), &osu_gmode, sizeof std::int32_t, nullptr);
        gameStat.gameMode = static_cast<PlayMode>(osu_gmode);
    }
    else {
        std::wstring mode = gameStat.mfCurrentOsuGMode->ReadToEnd();
        if (mode == L"Osu")
        {
            gameStat.gameMode = PlayMode::STANDARD;
        }
        else if (mode == L"OsuMania")
        {
            gameStat.gameMode = PlayMode::MANIA;
        }
        else if (mode == L"Taiko")
        {
            gameStat.gameMode = PlayMode::TAIKO;
        }
        else
        {
            gameStat.gameMode = PlayMode::UNKNOWN;
        }
    }

    Render(gameStat);
}

// Updates the world.
void Overlay::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    // TODO: Add your game logic here.
    elapsedTime;
}

// Draws the scene.
void Overlay::Render(osuGame &gameStat)
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    // TODO: Add your rendering code here.

    m_d3dContext->OMSetBlendState(m_states->Opaque(), nullptr, 0xFFFFFFFF);
    m_d3dContext->OMSetDepthStencilState(m_states->DepthNone(), 0);
    m_d3dContext->RSSetState(m_states->CullNone());

    m_effect->Apply(m_d3dContext.Get());

    m_d3dContext->IASetInputLayout(m_inputLayout.Get());

    m_spriteBatch->Begin();
    m_batch->Begin();

    //DrawCircleIWantToKillMyself();

    std::wstring textString;
    XMVECTOR result;

    // set font origin
    DirectX::SimpleMath::Vector2 origin = DirectX::SimpleMath::Vector2(0.f, 0.f);
    //m_font->MeasureString(textString.c_str()) / 2.f;
    //middle

    /*
     *  Current local time
     *
     *  why? good question
     *
     *  idk how to do this so i copied this off stackoverflow
     */

    std::time_t now = std::time(nullptr);
    std::tm timestamp;
    ::localtime_s(&timestamp, &now);
    std::wstringstream wss;
    wss << std::put_time(&timestamp, L" | %c PST");
    textString = this->user + wss.str();

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos = gameStat.bOsuIngame ? DirectX::SimpleMath::Vector2(1000, 10) : DirectX::SimpleMath::Vector2(static_cast<float>(m_outputWidth), m_outputHeight - 21.f - 45.f);

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::HotPink, Colors::Black, 2);

    /*
     * Kps + (totalclicks) string
     */

    textString = std::wstring(L"kps: ") + std::to_wstring(gameStat.clicks.size()) + std::wstring(L" (") + std::to_wstring(gameStat.clickCounter) + std::wstring(L")");

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos = DirectX::SimpleMath::Vector2(0.f + 370.f, m_outputHeight - 21.f);

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::Red, Colors::Black, 2);

    origin = DirectX::SimpleMath::Vector2(0.f, 0.f);

    /*textString =
        std::to_wstring(gameStat.clicks.size() * 60 / 4) + std::wstring(L"/") + std::to_wstring(gameStat.currentMap.currentBpm) + std::wstring(L" (x")
        + Utilities::to_wstring_with_precision(gameStat.currentMap.currentSpeed, 2) + std::wstring(L") bpm");

    m_fontPos.x += 30;

    result = RenderStatSquare(textString, origin, m_fontPos);*/

    /*
     *  skip map diff + map name for now.
     *
     *  map current stat.
     */

    textString = std::wstring(gameStat.mfOsuPlayHits->ReadToEnd()).size() == 3 ? L"" : static_cast<std::wstring>(gameStat.mfOsuPlayHits->ReadToEnd());

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos.x = static_cast<float>(m_outputWidth);    // all the way to right
    m_fontPos.y -= 46;  //  skip 2 boxes from bottom.

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::LightGreen, Colors::Black, 2);

    /*
     *  Cursor stuff (stat)
     */

    textString = std::wstring(L"x: ") + std::to_wstring(static_cast<int>(gameStat.cursorLocation.x)) +
        std::wstring(L" y: ") + std::to_wstring(static_cast<int>(gameStat.cursorLocation.y)) +
        std::wstring(L" vel: ") + std::to_wstring((int)round(gameStat.cursorSpeed)) + std::wstring(L" p/s");

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos.y -= 18;  //  go up a box. it's 21px with this font + scale.

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::DeepSkyBlue, Colors::Black, 2, 0.45f);

    /*
     *  current HP
     */

    textString = std::wstring(gameStat.mfOsuPlayHP->ReadToEnd()).size() == 3 ? L"" : static_cast<std::wstring>(gameStat.mfOsuPlayHP->ReadToEnd());

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos.y -= 21;  //  go up a box. magic number lol

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::LightSkyBlue, Colors::Black, 2, 0.5f);

    /*
     *  current pp
     */

    textString = std::wstring(gameStat.mfOsuPlayPP->ReadToEnd()).size() == 3 ? L"" : static_cast<std::wstring>(gameStat.mfOsuPlayPP->ReadToEnd());

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos.y -= 24;  //  go up a box. magic number lol

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::Yellow, Colors::Black, 2, 0.55f);

    RenderStatTexts(gameStat);
    RenderAssistant(gameStat);

    m_batch->End();
    m_spriteBatch->End();

    Present();
}

void Overlay::RenderStatTexts(osuGame &gameStat)
{
    //  Assumes font->Begin() and batch->Begin() is called.

    std::wstring textString;

    if (!gameStat.hOsu)
    {
        m_font->DrawString(m_spriteBatch.get(), L"Game not detected!",
            DirectX::SimpleMath::Vector2(0.f, 30.f),
            Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.4f);
        return;
    }
    else if (!gameStat.bOsuLoaded && gameStat.hOsu)
    {
        m_font->DrawString(m_spriteBatch.get(), L"Loading game.. please be patient! (10s+)",
            DirectX::SimpleMath::Vector2(0.f, 30.f),
            Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.4f);
        if (!gameStat.bOsuLoading)
        {
            std::thread loadGame = gameStat.LoadGameThread();
            loadGame.detach();
        }
        return;
    }

    if (!gameStat.bStreamCompanionRunning)
    {
        m_font->DrawString(m_spriteBatch.get(), L"StreamCompanion is not running!",
            DirectX::SimpleMath::Vector2(0.f, 30.f),
            Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.4f);
        return;
    }

    if (!gameStat.bOsuIngame)
        return;

    XMVECTORF32 beat_color = Colors::Yellow;

    switch (gameStat.beatIndex % 4)
    {
    case 0:
        beat_color = Colors::Yellow;
        break;
    case 1:
        beat_color = Colors::AliceBlue;
        break;
    case 2:
        beat_color = Colors::LightBlue;
        break;
    case 3:
        beat_color = Colors::AliceBlue;
        break;
    }

    DirectX::SimpleMath::Vector2 origin;

    switch (gameStat.gameMode)
    {
    case PlayMode::STANDARD: {
        hitobject *current_object = gameStat.getCurrentHitObject();
        hitobject *next_object = gameStat.currentMap.getNextHitObject();
        hitobject *upcoming_object = gameStat.currentMap.getUpcomingHitObject();

        /*
         *  Current note.
         *
         */

        origin = DirectX::SimpleMath::Vector2(0.f, 0.f);
        m_fontPos = DirectX::SimpleMath::Vector2(0.f, 30.f);

        if (current_object != nullptr && gameStat.osuMapTime < gameStat.currentMap.hitobjects[gameStat.currentMap.hitobjects.size() - 1].end_time)
            //(gameStat.currentMap.currentObjectIndex < gameStat.currentMap.hitobjects.size() && gameStat.osuMapTime < gameStat.currentMap.hitobjects[gameStat.currentMap.hitobjects.size() - 1].end_time)
        {
            textString = std::wstring(L"current: x: ") + std::to_wstring((*current_object).x) +
                std::wstring(L" y: ") + std::to_wstring((*current_object).y) +
                std::wstring(L" index: ") + std::to_wstring(gameStat.currentMap.currentObjectIndex) + std::wstring(L"/") + std::to_wstring(gameStat.currentMap.hitobjects.size()) +
                std::wstring(L" time index: ") + std::to_wstring(gameStat.currentMap.currentTimeIndex) + std::wstring(L"(") + std::to_wstring(gameStat.currentMap.currentUninheritTimeIndex) + std::wstring(L") ") +
                std::wstring(L" time: ") + std::to_wstring((*current_object).start_time);

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::LightBlue, 0.f, origin, 0.4f);

            m_fontPos.y += 15;
        }

        /*
         *  next note:
         *
         */

        if (next_object != nullptr)
        {
            textString = std::wstring(L"next: x: ") + std::to_wstring(next_object->x) +
                std::wstring(L" y: ") + std::to_wstring(next_object->y) +
                std::wstring(L"  dist: ") + Utilities::to_wstring_with_precision(
                    std::sqrt(
                        std::pow(
                            next_object->x - current_object->x, 2
                        ) +
                        std::pow(
                            next_object->y - current_object->y, 2
                        )
                    ), 1) +
                std::wstring(L" combo: ") + std::to_wstring(gameStat.currentMap.currentObjectIndex - gameStat.currentMap.newComboIndex + 1) +
                std::wstring(L" time left: ") + std::to_wstring(static_cast<int>(gameStat.GetSecondsFromOsuTime(next_object->start_time - gameStat.osuMapTime) * 1000)) + L"ms";
            //std::wstring(L"/") + std::to_wstring(next_object->start_time - current_object->start_time);

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::Yellow, 0.f, origin, 0.4f);

            m_fontPos.y += 15;
        }

        /*
         *  upcoming note
         *
         */

        if (upcoming_object != nullptr)
        {
            textString = std::wstring(L"upcoming: x: ") + std::to_wstring(upcoming_object->x) +
                std::wstring(L" y: ") + std::to_wstring(upcoming_object->y) +
                std::wstring(L"  dist: ") + Utilities::to_wstring_with_precision(
                    std::sqrt(
                        std::pow(
                            upcoming_object->x - next_object->x, 2
                        ) +
                        std::pow(
                            upcoming_object->y - next_object->y, 2
                        )
                    ), 1) +
                std::wstring(L"  time left: ") + std::to_wstring(static_cast<int>(gameStat.GetSecondsFromOsuTime(upcoming_object->start_time - gameStat.osuMapTime) * 1000)) + L"ms";
            //std::wstring(L"/") + std::to_wstring(gameStat.currentMap.hitobjects[gameStat.currentMap.currentObjectIndex + 2].start_time - gameStat.currentMap.hitobjects[gameStat.currentMap.currentObjectIndex].start_time);

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::Aqua, 0.f, origin, 0.4f);

            m_fontPos.y += 15;
        }

        /*
         *  Others
         *
         */

        if (gameStat.osuMapTime < gameStat.currentMap.hitobjects[gameStat.currentMap.hitobjects.size() - 1].end_time)
        {
            timingpoint *current_timingpoint = gameStat.currentMap.getCurrentTimingPoint(), *next_timingpoint = nullptr, *prev_timingpoint = nullptr;
            for (int i = gameStat.currentMap.currentTimeIndex + 1; i < gameStat.currentMap.timingpoints.size(); i++)
            {
                if (*current_timingpoint != gameStat.currentMap.timingpoints.at(i))
                {
                    next_timingpoint = &gameStat.currentMap.timingpoints.at(i);
                    break;
                }
            }

            for (int i = gameStat.currentMap.currentTimeIndex - 1; i >= 0; i--)
            {
                if (*current_timingpoint != gameStat.currentMap.timingpoints.at(i))
                {
                    prev_timingpoint = &gameStat.currentMap.timingpoints.at(i);
                    break;
                }
            }

            textString = L"bpm: ";

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::White, 0.f, origin, 0.4f);

            m_fontPos.x += (XMVectorGetX(m_font->MeasureString(textString.c_str()))) * 0.4f;

            if (prev_timingpoint != nullptr)
            {
                origin.y -= (XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.325f) / 2;
                textString = L" " + std::to_wstring(gameStat.GetActualBPM(prev_timingpoint->getBPM())) + L"bpm" + (prev_timingpoint->kiai ? L"☆ " : L" ") + L"(x" + Utilities::to_wstring_with_precision(prev_timingpoint->velocity, 2) + L") ->";
                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    m_fontPos, Colors::White, 0.f, origin, 0.325f);
                m_fontPos.x += XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.325f;
            }

            origin.y = 0.f;
            // gameStat.GetActualBPM(next_timingpoint->getBPM())  gameStat.currentMap.currentBpm
            textString = L" " + std::to_wstring(gameStat.GetActualBPM(current_timingpoint->getBPM())) + L"bpm" + (current_timingpoint->kiai ? L"☆ " : L" ") + L"(x" + Utilities::to_wstring_with_precision(current_timingpoint->velocity, 2) + L") ";
            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::Yellow, 0.f, origin, 0.4f);
            m_fontPos.x += XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.4f;

            // TODO auto bpm
            if (next_timingpoint != nullptr)
            {
                origin.y -= (XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.325f) / 2;
                textString = L" -> " + std::to_wstring(gameStat.GetActualBPM(next_timingpoint->getBPM())) + L"bpm" + (next_timingpoint->kiai ? L"☆ " : L" ") + L"(x" + Utilities::to_wstring_with_precision(next_timingpoint->velocity, 2) + L") -" + Utilities::to_wstring_with_precision(gameStat.GetSecondsFromOsuTime(next_timingpoint->offset - gameStat.osuMapTime), 2) + L"s";
                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    m_fontPos, Colors::White, 0.f, origin, 0.325f);
            }

            m_fontPos.x = 0.f;
            m_fontPos.y += 15;

            /*
                skip line
            */

            std::uint32_t nds = 0;
            std::uint32_t nds_f = 0;

            std::wstring nds_warning = L"";

            for (std::uint32_t i = gameStat.currentMap.currentObjectIndex; i < gameStat.currentMap.hitobjects.size() &&

                (gameStat.hasMod(Mods::DoubleTime) || gameStat.hasMod(Mods::Nightcore) ? 1500 : 1000) >= gameStat.osuMapTime - gameStat.currentMap.hitobjects[i].start_time; i--)
                //(std::uint32_t i = gameStat.currentMap.currentObjectIndex + 1; i < gameStat.currentMap.hitobjects.size() && 1000 >= gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
                //back 1s
            {
                nds++;
            }

            for (std::uint32_t i = gameStat.currentMap.currentObjectIndex + 1; i < gameStat.currentMap.hitobjects.size() && 1000 >= gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
                //forward 1s
            {
                nds_f++;
            }

            if (nds > 12 && nds < 22)
            {
                for (uint8_t i = 0; i < nds - 12; i++)
                {
                    nds_warning += L"!";
                }
            }
            else if (nds >= 22)
            {
                nds_warning += L"!!!!!!!!!!";
            }
            if (nds > 15)
            {
                nds_warning += L" warning!";
            }

            textString = std::wstring(L"map prog: ") + Utilities::to_wstring_with_precision((static_cast<double>(gameStat.osuMapTime) / gameStat.currentMap.hitobjects[gameStat.currentMap.hitobjects.size() - 1].start_time) * 100, 1) +
                std::wstring(L"% timing: ") + std::to_wstring(gameStat.beatIndex / 4) +
                std::wstring(L":") + std::to_wstring(gameStat.beatIndex % 4) + std::wstring(L" note/sec: ") + std::to_wstring(nds) + nds_warning;

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::White, 0.f, origin, 0.4f);

            m_fontPos.y += 15;
        }

        break;
    }
    case PlayMode::MANIA: {
        hitobject *next_object;
        hitobject *current_object;
        origin = DirectX::SimpleMath::Vector2(0, 0);
        m_fontPos = DirectX::SimpleMath::Vector2(570.f, 90.f);

        for (int i = 0; i < 10; ++i)    // 10 because hardcoded row lol
        {
            if (gameStat.currentMap.hitobjects_sorted[i].size() == 0)
                break;

            current_object = gameStat.currentMap.getCurrentHitObject(i);
            next_object = gameStat.currentMap.getNextHitObject(i);

            if (current_object == nullptr || gameStat.osuMapTime > gameStat.currentMap.hitobjects_sorted[i][gameStat.currentMap.hitobjects_sorted[i].size() - 1].end_time)
                continue;

            int objIndex = gameStat.currentMap.currentObjectIndex_sorted[i];

            textString = std::wstring(L"row ") + std::to_wstring(i) + std::wstring(L": index: ") + std::to_wstring(objIndex) +
                std::wstring(L" hold?: ") + ((*current_object).IsHold() ? L"Yes, time: " + (((*current_object).end_time - gameStat.osuMapTime) > 0 ? std::to_wstring((*current_object).end_time - gameStat.osuMapTime) : L"ok!") + L"/" + std::to_wstring((*current_object).end_time - (*current_object).start_time) : L"No");

            // next_object
            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::Aqua, 0.f, origin, 0.4f);

            if (next_object == nullptr)
            {
                m_fontPos.y += 15;
                continue;
            }

            std::uint32_t nds = 0;

            std::wstring nds_warning = L"";

            for (std::uint32_t o = objIndex; o < gameStat.currentMap.hitobjects_sorted[i].size() && 1000 >= gameStat.osuMapTime - gameStat.currentMap.hitobjects_sorted[i][o].start_time; o--)
            {
                nds++;
            }

            //m_fontPos.x += XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.4f;
            m_fontPos.y += 15;
            m_fontPos.x = 600.f;

            textString = std::wstring(L" / next: hold?: ") + ((*next_object).IsHold() ? L"Yes" : L"No") + std::wstring(L" note/sec: ") + std::to_wstring(nds) + std::wstring(L" time: ") +
                std::to_wstring((*next_object).start_time - gameStat.osuMapTime) + L"/" + std::to_wstring((*next_object).start_time - (*current_object).end_time);

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::Yellow, 0.f, origin, 0.4f);

            m_fontPos.y += 15;
            m_fontPos.x = 570.f;
        }

        if (gameStat.osuMapTime < gameStat.currentMap.hitobjects[gameStat.currentMap.hitobjects.size() - 1].end_time)
        {
            std::uint32_t nds = 0;
            std::uint32_t nds_f = 0;
            std::uint32_t nds_warning_c = gameStat.currentMap.CircleSize * 6;

            std::wstring nds_warning = L"";

            for (std::uint32_t i = gameStat.currentMap.currentObjectIndex; i < gameStat.currentMap.hitobjects.size() && 1000 >= gameStat.osuMapTime - gameStat.currentMap.hitobjects[i].start_time; i--)
                //backward 1s
            {
                nds++;
            }

            for (std::uint32_t i = gameStat.currentMap.currentObjectIndex + 1; i < gameStat.currentMap.hitobjects.size() && 1000 >= gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
                //forward 1s
            {
                nds_f++;
            }

            if (nds > nds_warning_c && nds < (nds_warning_c + 10))
            {
                for (uint8_t i = 0; i < nds - nds_warning_c; i++)
                {
                    nds_warning += L"!";
                }
            }
            else if (nds >= (nds_warning_c + 10))
            {
                nds_warning += L"!!!!!!!!!!";
            }
            if (nds > (nds_warning_c + 6))
            {
                nds_warning += L" warning!";
            }

            textString = std::wstring(L"map prog: ") + Utilities::to_wstring_with_precision((static_cast<double>(gameStat.osuMapTime) / gameStat.currentMap.hitobjects[gameStat.currentMap.hitobjects.size() - 1].end_time) * 100, 1) + std::wstring(L"% timing: ") + std::to_wstring(gameStat.beatIndex / 4) +
                std::wstring(L":") + std::to_wstring(gameStat.beatIndex % 4) + std::wstring(L" total note/sec: ") + std::to_wstring(nds) + nds_warning;

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::White, 0.f, origin, 0.4f);
            m_fontPos.y += 15;
        }

        break;
    case PlayMode::TAIKO:
        origin = DirectX::SimpleMath::Vector2(0.f, 0.f);
        m_fontPos = DirectX::SimpleMath::Vector2(170.f, 380.f);

        if (gameStat.osuMapTime < gameStat.currentMap.hitobjects[gameStat.currentMap.hitobjects.size() - 1].end_time)
        {
            timingpoint *next_timingpoint = nullptr, *prev_timingpoint = nullptr;
            for (int i = gameStat.currentMap.currentTimeIndex + 1; i < gameStat.currentMap.timingpoints.size(); i++)
            {
                if (*gameStat.currentMap.getCurrentTimingPoint() != gameStat.currentMap.timingpoints.at(i))
                {
                    next_timingpoint = &gameStat.currentMap.timingpoints.at(i);
                    break;
                }
            }

            for (int i = gameStat.currentMap.currentTimeIndex - 1; i >= 0; i--)
            {
                if (*gameStat.currentMap.getCurrentTimingPoint() != gameStat.currentMap.timingpoints.at(i))
                {
                    prev_timingpoint = &gameStat.currentMap.timingpoints.at(i);
                    break;
                }
            }

            textString = L"bpm: ";

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::White, 0.f, origin, 0.5f);

            m_fontPos.x += (XMVectorGetX(m_font->MeasureString(textString.c_str()))) * 0.5f;

            if (prev_timingpoint != nullptr)
            {
                textString = L" " + std::to_wstring(prev_timingpoint->getBPM()) + L"bpm (x" + Utilities::to_wstring_with_precision(prev_timingpoint->velocity, 2) + L") ->";
                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    m_fontPos, Colors::White, 0.f, origin, 0.5f);
                m_fontPos.x += XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.5f;
            }

            textString = L" " + std::to_wstring(gameStat.currentMap.currentBpm) + L"bpm (x" + Utilities::to_wstring_with_precision(gameStat.currentMap.currentSpeed, 2) + L")";
            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::Yellow, 0.f, origin, 0.5f);
            m_fontPos.x += XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.5f;

            if (next_timingpoint != nullptr)
            {
                textString = L" -> " + std::to_wstring(next_timingpoint->getBPM()) + L"bpm (x" + Utilities::to_wstring_with_precision(next_timingpoint->velocity, 2) + L") -" + std::to_wstring(next_timingpoint->offset - gameStat.osuMapTime);
                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    m_fontPos, Colors::White, 0.f, origin, 0.5f);
            }

            m_fontPos.x = 170.f;
            m_fontPos.y += 25;

            /*
                skip line
            */

            std::uint32_t nds = 0;
            std::uint32_t nds_f = 0;

            std::wstring nds_warning = L"";

            for (std::uint32_t i = gameStat.currentMap.currentObjectIndex; i < gameStat.currentMap.hitobjects.size() && 1000 >= gameStat.osuMapTime - gameStat.currentMap.hitobjects[i].start_time; i--)
                //(std::uint32_t i = gameStat.currentMap.currentObjectIndex + 1; i < gameStat.currentMap.hitobjects.size() && 1000 >= gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
                //forward 1s
            {
                nds++;
            }

            for (std::uint32_t i = gameStat.currentMap.currentObjectIndex + 1; i < gameStat.currentMap.hitobjects.size() && 1000 >= gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
                //forward 1s
            {
                nds_f++;
            }

            if (nds > 12 && nds < 22)
            {
                for (uint8_t i = 0; i < nds - 12; i++)
                {
                    nds_warning += L"!";
                }
            }
            else if (nds >= 22)
            {
                nds_warning += L"!!!!!!!!!!";
            }
            if (nds > 15)
            {
                nds_warning += L" warning!";
            }

            textString = std::wstring(L"map prog: ") + Utilities::to_wstring_with_precision((static_cast<double>(gameStat.osuMapTime) / gameStat.currentMap.hitobjects[gameStat.currentMap.hitobjects.size() - 1].start_time) * 100, 1) +
                std::wstring(L"% timing: ") + std::to_wstring(gameStat.beatIndex / 4) +
                std::wstring(L":") + std::to_wstring(gameStat.beatIndex % 4) + std::wstring(L" note/sec: ") + std::to_wstring(nds) + nds_warning;

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::White, 0.f, origin, 0.4f);

            m_fontPos.y += 15;
        }
        break;
    }
    }

    /*
     *  kiai
     *
     *
     */

    if (gameStat.currentMap.kiai)
    {
        textString = std::wstring(L"kiai mode! " + std::to_wstring((gameStat.beatIndex % 4) + 1) + ((gameStat.beatIndex % 4) % 2 == 0 ? L" >" : L" <"));

        m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
            m_fontPos,
            beat_color, 0.f, origin, 0.4f);
    }
}

void Overlay::RenderAssistant(osuGame &gameStat)
{
    //  Assumes font->Begin() and batch->Begin() is called.
    /*
     *  hidden assistant - time till next hitobject
     *
     *  take notes from OsuBot::setCursorStartPoints() at
     *  https://github.com/CookieHoodie/OsuBot/blob/master/OsuBots/OsuBot.cpp#L13
     *
     *  hard-coded resolution value because i'm lazy:
     *  screenWidth = 960
     *  screenWidth = 720
     *
     *  multiplierX = 1.5
     *  multiplierY = 1.5
     *
     *  offsetX = 256
     *  offsetY = 72
     *
     *  cursorStartPoints.x = 257
     *  cursorStartPoints.y = 12 + 72
     */

    if (gameStat.currentMap.hitobjects.size() == 0 ||
        (!(gameStat.currentMap.currentObjectIndex < gameStat.currentMap.hitobjects.size() - 1) && gameStat.currentMap.hitobjects.back().IsCircle()
            && (gameStat.currentMap.hitobjects.back().end_time < gameStat.osuMapTime)))
        return;

    // disable if want
    if (!gameStat.bOsuIngame)
        return;

    std::wstring textString;

    switch (gameStat.gameMode)
    {
    case PlayMode::STANDARD: {
        hitobject *current_object = gameStat.getCurrentHitObject();
        hitobject *next_object = gameStat.currentMap.getNextHitObject();
        hitobject *upcoming_object = gameStat.currentMap.getUpcomingHitObject();

        if (gameStat.hasMod(Mods::Hidden) || !gameStat.bOsuIngame) // is hidden or not in game
        {
            std::uint32_t comboIndex = gameStat.currentMap.newComboIndex;
            bool bContinue = true;

            for (std::uint32_t i = gameStat.currentMap.currentObjectIndex + 1; i < gameStat.currentMap.hitobjects.size() && bContinue; i++)
            {
                //  Starts at 1
                //textString = std::to_wstring(i - gameStat.currentMap.currentObjectIndex);

                //  follows map combo

                bContinue = (gameStat.bOsuIngame ? 600 : 800) >= gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime;

                comboIndex = gameStat.currentMap.hitobjects[i].IsNewCombo() ? i : comboIndex;

                textString = std::to_wstring(i - comboIndex + 1);

                //std::to_wstring(gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime);

                //  first note
                if (i == gameStat.currentMap.currentObjectIndex + 1)
                {
                    /*if (!gameStat.bOsuIngame)
                    {
                        textString = std::to_wstring(i - comboIndex);

                        m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                            DirectX::SimpleMath::Vector2(
                                2 + 257.f + (*current_object).x * 1.5f, // padding + pixel
                                84.f + (*current_object).y * 1.5f
                            ),
                            Colors::LightBlue, 0.f, m_font->MeasureString(textString.c_str()) * 0.4f, 0.4f);
                    }*/

                    textString = L"> " + textString + L" <";

                    m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                        DirectX::SimpleMath::Vector2(
                            2 + 257.f + (*next_object).x * 1.5f, // padding + pixel
                            84.f + (*next_object).y * 1.5f
                        ),
                        Colors::Yellow, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                    m_font->DrawString(m_spriteBatch.get(), std::wstring(L" - " + std::to_wstring((*next_object).start_time - gameStat.osuMapTime)).c_str(),
                        DirectX::SimpleMath::Vector2(
                            257.f + (*next_object).x * 1.5f + (XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.3f),
                            84.f + (*next_object).y * 1.5f
                        ),
                        Colors::Yellow, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.35f);
                }
                else
                {
                    m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                        DirectX::SimpleMath::Vector2(
                            257.f + gameStat.currentMap.hitobjects[i].x * 1.5f, // padding + pixel
                            84.f + gameStat.currentMap.hitobjects[i].y * 1.5f
                        ),
                        Colors::White, 0.f, m_font->MeasureString(textString.c_str()) * 0.5f, 0.5f);

                    m_font->DrawString(m_spriteBatch.get(), std::wstring(L" - " + std::to_wstring(gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime)).c_str(),
                        DirectX::SimpleMath::Vector2(
                            257.f + gameStat.currentMap.hitobjects[i].x * 1.5f + (XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.3f),
                            84.f + gameStat.currentMap.hitobjects[i].y * 1.5f
                        ),
                        Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.35f);

                    if ((std::sqrt(
                        std::pow(
                            gameStat.currentMap.hitobjects[i - 1].x - gameStat.currentMap.hitobjects[i].x, 2
                        ) +
                        std::pow(
                            gameStat.currentMap.hitobjects[i - 1].y - gameStat.currentMap.hitobjects[i].y, 2
                        )
                    ) > 125.0f
                        && gameStat.currentMap.hitobjects[i - 2].IsCircle()) || !gameStat.bOsuIngame) // todo remove1
                    {
                        double diffx = gameStat.currentMap.hitobjects[i - 1].x - gameStat.currentMap.hitobjects[i - 2].x;
                        double diffy = gameStat.currentMap.hitobjects[i - 1].y - gameStat.currentMap.hitobjects[i - 2].y;

                        double time_p = ((double)(gameStat.osuMapTime - gameStat.currentMap.hitobjects[i - 1].start_time) / (gameStat.currentMap.hitobjects[i - 1].start_time - gameStat.currentMap.hitobjects[i - 2].start_time));
                        double time_persent = 1.0 + time_p;

                        DirectX::SimpleMath::Vector2 v1 = DirectX::SimpleMath::Vector2(
                            257.f + (gameStat.currentMap.hitobjects[i - 2].x + (diffx * (time_persent >= 0.1 ? time_persent : 0.1))) * 1.5f,
                            84.f + (gameStat.currentMap.hitobjects[i - 2].y + (diffy * (time_persent >= 0.1 ? time_persent : 0.1))) * 1.5f
                        );

                        DirectX::SimpleMath::Vector2 v2 = DirectX::SimpleMath::Vector2(
                            257.f + (gameStat.currentMap.hitobjects[i - 1].x - (diffx * 0.1)) * 1.5f,
                            84.f + (gameStat.currentMap.hitobjects[i - 1].y - (diffy * 0.1)) * 1.5f
                        );

                        m_batch->DrawLine(
                            DirectX::VertexPositionColor(
                                v1, i == gameStat.currentMap.currentObjectIndex + 2 ? DirectX::Colors::Yellow : i == gameStat.currentMap.currentObjectIndex + 3 ? Colors::Aqua : DirectX::Colors::White
                            ),
                            DirectX::VertexPositionColor(
                                v2, i == gameStat.currentMap.currentObjectIndex + 2 ? DirectX::Colors::Yellow : i == gameStat.currentMap.currentObjectIndex + 3 ? Colors::Aqua : DirectX::Colors::White
                            )
                        );
                    }
                }
            }
        }
        else
        {
            textString = L">    <";

            /*
                Draw the Current hitobject
            */

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                GetScreenCoordFromOsuPixelStandard(gameStat.currentMap.getCurrentHitObject()),
                Colors::LightBlue, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

            /*
                Extra stuff for Current Hitobject
            */

            if (current_object->IsSlider() && current_object->end_time > gameStat.osuMapTime)
            {
                Overlay::DrawSlider(*current_object, gameStat.osuMapTime, Colors::LightBlue);
            }
            else if (current_object->IsSpinner() && current_object->end_time > gameStat.osuMapTime)
            {
                textString = Utilities::to_wstring_with_precision(gameStat.GetSecondsFromOsuTime(current_object->end_time - gameStat.osuMapTime), 1) + L"s";

                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    GetScreenCoordFromOsuPixelStandard(gameStat.currentMap.getCurrentHitObject()),
                    Colors::LightBlue, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                textString = L">    <";
            }

            if (!(gameStat.currentMap.currentObjectIndex < gameStat.currentMap.hitobjects.size() - 1))
            {
                return;
            }

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                GetScreenCoordFromOsuPixelStandard(next_object),
                Colors::Yellow, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

            double diffx = (*next_object).x - (*current_object).x;
            double diffy = (*next_object).y - (*current_object).y;

            double time_persent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->start_time));

            int32_t start_x = current_object->x, start_y = current_object->y;

            if (current_object->IsSlider())
            {
                time_persent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->end_time));
                start_x = (current_object->repeat % 2) == 0 ? current_object->x : current_object->slidercurves.back().x;
                start_y = (current_object->repeat % 2) == 0 ? current_object->y : current_object->slidercurves.back().y;
            }

            m_batch->DrawLine(
                DirectX::VertexPositionColor(
                    GetScreenCoordFromOsuPixelStandard(start_x, start_y, next_object->x, next_object->y, &time_persent),
                    DirectX::Colors::LightBlue
                ),
                DirectX::VertexPositionColor(
                    GetScreenCoordFromOsuPixelStandard(next_object->x, next_object->y),
                    DirectX::Colors::Yellow
                )
            );

            if (next_object->IsSlider())
            {
                Overlay::DrawSlider(*next_object, gameStat.osuMapTime, Colors::Yellow);
            }

            if (upcoming_object != nullptr)
            {
                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    GetScreenCoordFromOsuPixelStandard(upcoming_object),
                    Colors::Aqua, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                int32_t start_x = next_object->x, start_y = next_object->y;

                if (next_object->IsSlider())
                {
                    start_x = next_object->repeat % 2 == 0 ? next_object->x : next_object->slidercurves.back().x;
                    start_y = next_object->repeat % 2 == 0 ? next_object->y : next_object->slidercurves.back().y;

                    /*
                    m_font->DrawString(m_spriteBatch.get(), L"- -",
                        GetScreenCoordFromOsuPixelStandard(start_x, start_y),
                        Colors::Yellow, 0.f, m_font->MeasureString(L"- -") * 0.6f, 0.6f);
                     */
                }

                m_batch->DrawLine(
                    DirectX::VertexPositionColor(
                        GetScreenCoordFromOsuPixelStandard(start_x, start_y),
                        DirectX::Colors::Yellow
                    ),
                    DirectX::VertexPositionColor(
                        GetScreenCoordFromOsuPixelStandard(upcoming_object),
                        DirectX::Colors::Aqua
                    )
                );
            }
        }
    }
    case PlayMode::MANIA: {
        // render vertically. until then don't draw anything.
        if (!gameStat.bOsuIngame)
            return;
        // origin: x= 540.f, y= 360.f

        DirectX::SimpleMath::Vector3 v1, v2, v3, v4;
        DirectX::XMVECTORF32 bgColor = Colors::White;

        DirectX::SimpleMath::Vector3 playField = DirectX::SimpleMath::Vector3(540, 360, 0);
        DirectX::SimpleMath::Vector2 playField_size = DirectX::SimpleMath::Vector2(740, 220);

        int columnpx = playField_size.y / gameStat.currentMap.CircleSize;

        float notesize = 20.f / (gameStat.currentMap.CircleSize / 4);

        for (int col = 0; col < gameStat.currentMap.CircleSize; ++col)
        {
            for (std::uint32_t i = gameStat.currentMap.currentObjectIndex_sorted[col]; i < gameStat.currentMap.hitobjects_sorted[col].size() && 1000 >= gameStat.currentMap.hitobjects_sorted[col][i].start_time - gameStat.osuMapTime; ++i)
            {
                if (gameStat.currentMap.hitobjects_sorted[col][i].IsCircle())
                {
                    float startpos = 540.f + (gameStat.currentMap.hitobjects_sorted[col][i].start_time - gameStat.osuMapTime);

                    if (startpos <= 540.f)
                        continue;

                    m_batch->DrawQuad(
                        VertexPositionColor(DirectX::SimpleMath::Vector3(startpos + notesize / 2, playField.y + (columnpx * col), 0.f), bgColor),
                        VertexPositionColor(DirectX::SimpleMath::Vector3(startpos + notesize / 2, playField.y + (columnpx * (col + 1)), 0.f), bgColor),
                        VertexPositionColor(DirectX::SimpleMath::Vector3(startpos - notesize / 2, playField.y + (columnpx * (col + 1)), 0.f), bgColor),
                        VertexPositionColor(DirectX::SimpleMath::Vector3(startpos - notesize / 2, playField.y + (columnpx * col), 0.f), bgColor)
                    );
                }
                else
                {
                    float startpos = 540.f + (gameStat.currentMap.hitobjects_sorted[col][i].start_time - gameStat.osuMapTime);
                    float endpos = 540.f + (gameStat.currentMap.hitobjects_sorted[col][i].end_time - gameStat.osuMapTime);

                    if (startpos < 540.f)
                        startpos = 540.f;

                    if (endpos >= 1280.f)
                        endpos = 1280.f;
                    if (endpos <= 540.f)
                        continue;

                    m_batch->DrawQuad(
                        VertexPositionColor(DirectX::SimpleMath::Vector3(startpos - notesize / 2, playField.y + (columnpx * col), 0.f), bgColor),
                        VertexPositionColor(DirectX::SimpleMath::Vector3(startpos - notesize / 2, playField.y + (columnpx * (col + 1)), 0.f), bgColor),
                        VertexPositionColor(DirectX::SimpleMath::Vector3(endpos + notesize / 2, playField.y + (columnpx * (col + 1)), 0.f), bgColor),
                        VertexPositionColor(DirectX::SimpleMath::Vector3(endpos + notesize / 2, playField.y + (columnpx * col), 0.f), bgColor)
                    );
                }
            }
        }
        break;
    }
    }
}

void Overlay::DebugDrawSliderPoints(int x, int y, std::vector<slidercurve> &points, DirectX::XMVECTORF32 color)
{
    int x1 = x, y1 = y, x2, y2, num = 0;

    for (const slidercurve &point : points)
    {
        if (x1 == point.x && y1 == point.y)
        {
            num = 0;
            continue;
        }

        x2 = point.x;
        y2 = point.y;

        m_batch->DrawLine(
            DirectX::VertexPositionColor(
                GetScreenCoordFromOsuPixelStandard(x1, y1), color
            ),
            DirectX::VertexPositionColor(
                GetScreenCoordFromOsuPixelStandard(x2, y2), color
            )
        );

        m_font->DrawString(m_spriteBatch.get(), std::to_wstring(num).c_str(),
            GetScreenCoordFromOsuPixelStandard(x2, y2),
            color, 0.f, DirectX::SimpleMath::Vector2(0, 0), 0.3);

        num++;

        x1 = x2;
        y1 = y2;
    }
}

DirectX::SimpleMath::Vector2 Overlay::DrawSlider(hitobject &object, int32_t &time, DirectX::XMVECTORF32 color)
{
    if (!object.IsSlider())
        return DirectX::SimpleMath::Vector2(0, 0);

    std::vector<slidercurve> curves;
    slidercurve init_curve;
    DirectX::SimpleMath::Vector2 end_point = DirectX::SimpleMath::Vector2(1, 1);

    init_curve.x = object.x;
    init_curve.y = object.y;

    double length_left = object.pixel_length;

    OutputDebugStringW(L"DrawSlider: slidercurvesize: ");
    OutputDebugStringW(std::to_wstring(object.slidercurves.size()).c_str());
    OutputDebugStringW(L" type: ");
    OutputDebugStringW(object.slidertype.c_str());
    OutputDebugStringW(L"\n");

    if (object.slidertype == L"B")
    {
        for (int32_t i = 0; i < object.slidercurves.size(); i++)
        {
            if (curves.size() != 0 && curves.back() == object.slidercurves.at(i))
            {
                DrawSliderPartBiezer(init_curve, curves, length_left, color);
                init_curve = object.slidercurves.at(i);
                curves.clear();
            }
            curves.push_back(object.slidercurves.at(i));
        }

        DrawSliderPartBiezer(init_curve, curves, length_left, color, 0.0f);
    }
    else if (object.slidertype == L"L")
    {
        //DrawSliderLinear
        DebugDrawSliderPoints(
            object.x,
            object.y,
            object.slidercurves,
            color
        );
    }
    else if (object.slidertype == L"P")
    {
        //PerfectCircle
        DebugDrawSliderPoints(
            object.x,
            object.y,
            object.slidercurves,
            color
        );
    }
    else
    {
        DebugDrawSliderPoints(object.x, object.y, object.slidercurves, color);
    }

    return end_point;  // should be sliderend
}

void Overlay::DrawSliderPartBiezer(slidercurve &init_point, std::vector<slidercurve> &curves, double &dist_left, DirectX::XMVECTORF32 color, float_t inv_completion, DirectX::SimpleMath::Vector2 *vec)
{
    int size = curves.size();
    std::vector<int> x_p, y_p;
    std::vector<DirectX::VertexPositionColor> lines;
    x_p.push_back(init_point.x);
    y_p.push_back(init_point.y);
    for (int i = 0; i < curves.size(); i++)
    {
        x_p.push_back(curves.at(i).x);
        y_p.push_back(curves.at(i).y);
        //        dist_left -= curves.at(i).x
    }

    for (double t = 0; t <= 1; t += 0.1)
    {
        double x = Utilities::getBezierPoint(&x_p, t);
        double y = Utilities::getBezierPoint(&y_p, t);

        lines.push_back(DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(x, y),
            color
        ));
    }

    m_batch->Draw(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
        lines.data(),
        lines.size()
    );
}

XMVECTOR Overlay::RenderStatSquare(std::wstring &text, DirectX::SimpleMath::Vector2 &origin, DirectX::SimpleMath::Vector2 &fontPos, DirectX::XMVECTORF32 tColor, DirectX::XMVECTORF32 bgColor, int v, float fontsize)
{
    /* we're gonna draw this like:
     0,0
              v4------------------------------v3 (offsetx 3px)
              |                               |
              v1------------------------------v2 (offsetx 3px)
                                                       1280,720

         where v (default v1) = origin.
         Assumes font->Begin() and batch->Begin() is called.
         returns the string measure.
     */

    XMVECTOR textvec = m_font->MeasureString(text.c_str());

    //   (XMVectorGetX(textvec) / 2.f), (XMVectorGetY(textvec) / 2.f)

    /*
     *  Cannot think of better way it's 12:02- shitcoding.
     */
    DirectX::SimpleMath::Vector3 v1, v2, v3, v4;
    switch (v)
    {
    case 1:
        v1 = DirectX::SimpleMath::Vector3(fontPos.x, fontPos.y, 0.f);
        v2 = DirectX::SimpleMath::Vector3(fontPos.x + (XMVectorGetX(textvec) * fontsize) + 3, fontPos.y, 0.f);
        v3 = DirectX::SimpleMath::Vector3(fontPos.x + (XMVectorGetX(textvec) * fontsize) + 3, fontPos.y + (XMVectorGetY(textvec) * fontsize), 0.f);
        v4 = DirectX::SimpleMath::Vector3(fontPos.x, fontPos.y + (XMVectorGetY(textvec) * fontsize), 0.f);
        break;
    case 2:
        v1 = DirectX::SimpleMath::Vector3(fontPos.x - (XMVectorGetX(textvec) * fontsize), fontPos.y, 0.f);
        v2 = DirectX::SimpleMath::Vector3(fontPos.x + 3, fontPos.y, 0.f);
        v3 = DirectX::SimpleMath::Vector3(fontPos.x + 3, fontPos.y + (XMVectorGetY(textvec) * fontsize), 0.f);
        v4 = DirectX::SimpleMath::Vector3(fontPos.x - (XMVectorGetX(textvec) * fontsize), fontPos.y + (XMVectorGetY(textvec) * fontsize), 0.f);
        break;
    case 3:
        v1 = DirectX::SimpleMath::Vector3(fontPos.x - (XMVectorGetX(textvec) * fontsize), fontPos.y - (XMVectorGetY(textvec) * fontsize), 0.f);
        v2 = DirectX::SimpleMath::Vector3(fontPos.x + 3, fontPos.y - (XMVectorGetY(textvec) * fontsize), 0.f);
        v3 = DirectX::SimpleMath::Vector3(fontPos.x + 3, fontPos.y, 0.f);
        v4 = DirectX::SimpleMath::Vector3(fontPos.x - (XMVectorGetX(textvec) * fontsize), fontPos.y + (XMVectorGetY(textvec) * fontsize), 0.f);
        break;
    case 4:
        v1 = DirectX::SimpleMath::Vector3(fontPos.x, fontPos.y - (XMVectorGetY(textvec) * fontsize), 0.f);;
        v2 = DirectX::SimpleMath::Vector3(fontPos.x + (XMVectorGetX(textvec) * fontsize) + 3, fontPos.y - (XMVectorGetY(textvec) * fontsize), 0.f);;
        v3 = DirectX::SimpleMath::Vector3(fontPos.x + (XMVectorGetX(textvec) * fontsize) + 3, fontPos.y, 0.f);
        v4 = DirectX::SimpleMath::Vector3(fontPos.x, fontPos.y, 0.f);
        break;
    }

    VertexPositionColor vp1(v1, bgColor);
    VertexPositionColor vp2(v2, bgColor);
    VertexPositionColor vp3(v3, bgColor);
    VertexPositionColor vp4(v4, bgColor);

    m_batch->DrawQuad(vp1, vp2, vp3, vp4);

    m_font->DrawString(m_spriteBatch.get(), text.c_str(),
        fontPos, tColor, 0.f, origin, fontsize);

    return textvec;
}

// Helper method to clear the back buffers.
void Overlay::Clear()
{
    // Clear the views.
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Green);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // Set the viewport.
    CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
    m_d3dContext->RSSetViewports(1, &viewport);
}

// Presents the back buffer contents to the screen.
void Overlay::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}

// Message handlers
void Overlay::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Overlay::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Overlay::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Overlay::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Overlay::OnWindowSizeChanged(int width, int height)
{
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateResources();

    // TODO: Game window is being resized.
}

// Properties
void Overlay::GetDefaultSize(int& width, int& height) const
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 1280;
    height = 720;
}

// These are the resources that depend on the device.
void Overlay::CreateDevice()
{
    UINT creationFlags = 0;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static const D3D_FEATURE_LEVEL featureLevels[] =
    {
        // TODO: Modify for supported Direct3D feature levels
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    // Create the DX11 API device object, and get a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    DX::ThrowIfFailed(D3D11CreateDevice(
        nullptr,                            // specify nullptr to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        device.ReleaseAndGetAddressOf(),    // returns the Direct3D device created
        &m_featureLevel,                    // returns feature level of device created
        context.ReleaseAndGetAddressOf()    // returns the device immediate context
    ));

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(device.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide[] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // TODO: Add more message IDs here as needed.
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = _countof(hide);
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    DX::ThrowIfFailed(device.As(&m_d3dDevice));
    DX::ThrowIfFailed(context.As(&m_d3dContext));

    //  fonts

    m_font = std::make_unique<SpriteFont>(m_d3dDevice.Get(), L"myfile.spritefont");
    m_spriteBatch = std::make_unique<SpriteBatch>(m_d3dContext.Get());

    //  rendering

    m_states = std::make_unique<CommonStates>(m_d3dDevice.Get());

    m_effect = std::make_unique<BasicEffect>(m_d3dDevice.Get());
    m_effect->SetVertexColorEnabled(true);

    void const* shaderByteCode;
    size_t byteCodeLength;

    m_effect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

    DX::ThrowIfFailed(
        m_d3dDevice->CreateInputLayout(VertexPositionColor::InputElements,
            VertexPositionColor::InputElementCount,
            shaderByteCode, byteCodeLength,
            m_inputLayout.ReleaseAndGetAddressOf()));

    m_batch = std::make_unique<PrimitiveBatch<VertexPositionColor>>(m_d3dContext.Get());

    // TODO: Initialize device dependent objects here (independent of window size).
}

// Allocate all memory resources that change on a window SizeChanged event.
void Overlay::CreateResources()
{
    // Clear the previous window size specific context.
    ID3D11RenderTargetView* nullViews[] = { nullptr };
    m_d3dContext->OMSetRenderTargets(_countof(nullViews), nullViews, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    UINT backBufferHeight = static_cast<UINT>(m_outputHeight);
    DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    UINT backBufferCount = 2;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device.
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        // And obtain the factory object that created it.
        ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = backBufferCount;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a SwapChain from a Win32 window.
        DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            m_d3dDevice.Get(),
            m_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            m_swapChain.ReleaseAndGetAddressOf()
        ));

        // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
        DX::ThrowIfFailed(dxgiFactory->MakeWindowAssociation(m_window, DXGI_MWA_NO_ALT_ENTER));
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

    CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), &depthStencilViewDesc, m_depthStencilView.ReleaseAndGetAddressOf()));

    // TODO: Initialize windows-size dependent objects here.
    m_fontPos.x = backBufferWidth / 2.f;
    m_fontPos.y = backBufferHeight / 2.f;

    DirectX::SimpleMath::Matrix proj = DirectX::SimpleMath::Matrix::CreateOrthographicOffCenter(0.f, float(backBufferWidth), float(backBufferHeight), 0.f, 0.f, 1.f);
    /*DirectX::SimpleMath::Matrix::CreateOrthographicOffCenter(-float(backBufferWidth / 2.f), float(backBufferWidth / 2.f),
    -float(backBufferHeight / 2.f), float(backBufferHeight / 2.f), 0.f, 1.f);*/
    m_effect->SetProjection(proj);
}

void Overlay::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.

    m_depthStencilView.Reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();

    m_spriteBatch.reset();

    m_states.reset();
    m_effect.reset();
    m_batch.reset();
    m_inputLayout.Reset();

    CreateDevice();

    CreateResources();
}