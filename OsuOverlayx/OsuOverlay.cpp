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

// Executes the basic game loop.
void Overlay::Tick(osuGame &gameStat)
{
    gameStat.CheckLoaded();

    //std::future<bool> mapLoaded = std::async(std::launch::async, &this->loadMap, this, gameStat);

    gameStat.currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
        );

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
    gameStat.readMemory();
    gameStat.CheckMap();

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

    if (!*gameStat.bOsuLoaded && !*gameStat.bOsuLoading)
    {
        m_font->DrawString(m_spriteBatch.get(), L"Game not detected!",
            DirectX::SimpleMath::Vector2(0.f, 90.f),
            Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.4f);
        return;
    }
    else if (!*gameStat.bOsuLoaded && *gameStat.bOsuLoading)
    {
        m_font->DrawString(m_spriteBatch.get(), L"Loading game.. one second! (10s+)",
            DirectX::SimpleMath::Vector2(0.f, 90.f),
            Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.4f);
        return;
    }
    else if (!gameStat.loadedMap.loaded)
    {
        m_font->DrawString(m_spriteBatch.get(), L"Map Loading... (Most likely Map load fail..)",
            DirectX::SimpleMath::Vector2(0.f, 90.f),
            Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.4f);
        return;
    }

    DirectX::SimpleMath::Vector2 origin = DirectX::SimpleMath::Vector2(0.f, 0.f);

    if (!gameStat.bOsuIngame)
    {
        timingpoint *current_timingpoint = gameStat.loadedMap.getCurrentTimingPoint();
        if (current_timingpoint == nullptr)
            return;
        std::wstring str = std::to_wstring(gameStat.GetActualBPM(current_timingpoint->getBPM()))
            + L"bpm" + (current_timingpoint->kiai ? L"☆ " : L" ") + L"(x" + Utilities::to_wstring_with_precision(current_timingpoint->velocity, 2)
            + L") setid:" + std::to_wstring(gameStat.loadedMap.BeatmapSetID) + L" id:" + std::to_wstring(gameStat.loadedMap.BeatMapID);

        m_font->DrawString(m_spriteBatch.get(),
            str.c_str(),
            DirectX::SimpleMath::Vector2(-1.f, 90.f),
            Colors::Yellow,
            0.f,
            DirectX::SimpleMath::Vector2(0.f, 0.f),
            0.4f
        );
        return;
    }

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

    switch (gameStat.gameMode)
    {
    case PlayMode::STANDARD: {
        hitobject *current_object = gameStat.getCurrentHitObject();
        hitobject *next_object = gameStat.loadedMap.getNextHitObject();
        hitobject *upcoming_object = gameStat.loadedMap.getUpcomingHitObject();

        /*
         *  Current note.
         *
         */

        origin = DirectX::SimpleMath::Vector2(0.f, 0.f);
        m_fontPos = DirectX::SimpleMath::Vector2(0.f, 28.f);

        if (current_object != nullptr && gameStat.osuMapTime < gameStat.loadedMap.hitobjects[gameStat.loadedMap.hitobjects.size() - 1].end_time)
            //(gameStat.currentMap.currentObjectIndex < gameStat.currentMap.hitobjects.size() && gameStat.osuMapTime < gameStat.currentMap.hitobjects[gameStat.currentMap.hitobjects.size() - 1].end_time)
        {
            textString = std::wstring(L"current: x: ") + std::to_wstring((*current_object).x) +
                std::wstring(L" y: ") + std::to_wstring((*current_object).y) +
                std::wstring(L" index: ") + std::to_wstring(gameStat.loadedMap.currentObjectIndex) + std::wstring(L"/") + std::to_wstring(gameStat.loadedMap.hitobjects.size()) +
                std::wstring(L" time index: ") + std::to_wstring(gameStat.loadedMap.currentTimeIndex) + std::wstring(L"(") + std::to_wstring(gameStat.loadedMap.currentUninheritTimeIndex) + std::wstring(L") ") +
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
                std::wstring(L" combo: ") + std::to_wstring(gameStat.loadedMap.currentObjectIndex - gameStat.loadedMap.newComboIndex + 1) +
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

        if (gameStat.osuMapTime < gameStat.loadedMap.hitobjects[gameStat.loadedMap.hitobjects.size() - 1].end_time)
        {
            timingpoint *current_timingpoint = gameStat.loadedMap.getCurrentTimingPoint(), *next_timingpoint = nullptr, *prev_timingpoint = nullptr;
            for (int i = gameStat.loadedMap.currentTimeIndex + 1; i < gameStat.loadedMap.timingpoints.size(); i++)
            {
                if (*current_timingpoint != gameStat.loadedMap.timingpoints.at(i))
                {
                    next_timingpoint = &gameStat.loadedMap.timingpoints.at(i);
                    break;
                }
            }

            for (int i = gameStat.loadedMap.currentTimeIndex - 1; i >= 0; i--)
            {
                if (*current_timingpoint != gameStat.loadedMap.timingpoints.at(i))
                {
                    prev_timingpoint = &gameStat.loadedMap.timingpoints.at(i);
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

            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex; i < gameStat.loadedMap.hitobjects.size() &&

                (gameStat.hasMod(Mods::DoubleTime) || gameStat.hasMod(Mods::Nightcore) ? 1500 : 1000) >= gameStat.osuMapTime - gameStat.loadedMap.hitobjects[i].start_time; i--)
                //(std::uint32_t i = gameStat.currentMap.currentObjectIndex + 1; i < gameStat.currentMap.hitobjects.size() && 1000 >= gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
                //back 1s
            {
                nds++;
            }

            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex + 1; i < gameStat.loadedMap.hitobjects.size() && 1000 >= gameStat.loadedMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
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

            textString = std::wstring(L"map prog: ") + Utilities::to_wstring_with_precision((static_cast<double>(gameStat.osuMapTime) / gameStat.loadedMap.hitobjects[gameStat.loadedMap.hitobjects.size() - 1].start_time) * 100, 1) +
                std::wstring(L"% timing: ") + std::to_wstring(gameStat.beatIndex / 4) +
                std::wstring(L":") + std::to_wstring(gameStat.beatIndex % 4) + std::wstring(L" note/sec: ") + std::to_wstring(nds) + nds_warning;

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::White, 0.f, origin, 0.4f);

            m_fontPos.y += 15;

            if (gameStat.hasMod(Mods::HardRock))
            {
                m_font->DrawString(m_spriteBatch.get(), L"Incorrect Information notice: Hardrock not Supported!",
                    m_fontPos, Colors::White, 0.f, origin, 0.4f);

                m_fontPos.y += 15;
            }
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
            if (gameStat.loadedMap.hitobjects_sorted[i].size() == 0)
                continue;

            current_object = gameStat.loadedMap.getCurrentHitObject(i);
            next_object = gameStat.loadedMap.getNextHitObject(i);

            if (current_object == nullptr || gameStat.osuMapTime > gameStat.loadedMap.hitobjects_sorted[i][gameStat.loadedMap.hitobjects_sorted[i].size() - 1].end_time)
                continue;

            int objIndex = gameStat.loadedMap.currentObjectIndex_sorted[i];

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

            for (std::uint32_t o = objIndex; o < gameStat.loadedMap.hitobjects_sorted[i].size() && 1000 >= gameStat.osuMapTime - gameStat.loadedMap.hitobjects_sorted[i][o].start_time; o--)
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

        if (gameStat.osuMapTime < gameStat.loadedMap.hitobjects[gameStat.loadedMap.hitobjects.size() - 1].end_time)
        {
            timingpoint *current_timingpoint = gameStat.loadedMap.getCurrentTimingPoint(), *next_timingpoint = nullptr, *prev_timingpoint = nullptr;
            for (int i = gameStat.loadedMap.currentTimeIndex + 1; i < gameStat.loadedMap.timingpoints.size(); i++)
            {
                if (*current_timingpoint != gameStat.loadedMap.timingpoints.at(i))
                {
                    next_timingpoint = &gameStat.loadedMap.timingpoints.at(i);
                    break;
                }
            }

            for (int i = gameStat.loadedMap.currentTimeIndex - 1; i >= 0; i--)
            {
                if (*current_timingpoint != gameStat.loadedMap.timingpoints.at(i))
                {
                    prev_timingpoint = &gameStat.loadedMap.timingpoints.at(i);
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

            m_fontPos.x = 570.f;
            m_fontPos.y += 15;

            std::uint32_t nds = 0;
            std::uint32_t nds_f = 0;
            std::uint32_t nds_warning_c = gameStat.loadedMap.CircleSize * 6;

            std::wstring nds_warning = L"";

            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex; i < gameStat.loadedMap.hitobjects.size() && 1000 >= gameStat.osuMapTime - gameStat.loadedMap.hitobjects[i].start_time; i--)
                //backward 1s
            {
                nds++;
            }

            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex + 1; i < gameStat.loadedMap.hitobjects.size() && 1000 >= gameStat.loadedMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
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

            textString = std::wstring(L"map prog: ") + Utilities::to_wstring_with_precision((static_cast<double>(gameStat.osuMapTime) / gameStat.loadedMap.hitobjects[gameStat.loadedMap.hitobjects.size() - 1].end_time) * 100, 1) + std::wstring(L"% timing: ") + std::to_wstring(gameStat.beatIndex / 4) +
                std::wstring(L":") + std::to_wstring(gameStat.beatIndex % 4) + std::wstring(L" total note/sec: ") + std::to_wstring(nds) + nds_warning;

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::White, 0.f, origin, 0.4f);
            m_fontPos.y += 15;
        }

        break;
    case PlayMode::TAIKO:
        origin = DirectX::SimpleMath::Vector2(0.f, 0.f);
        m_fontPos = DirectX::SimpleMath::Vector2(170.f, 380.f);

        if (gameStat.osuMapTime < gameStat.loadedMap.hitobjects[gameStat.loadedMap.hitobjects.size() - 1].end_time)
        {
            timingpoint *next_timingpoint = nullptr, *prev_timingpoint = nullptr;
            for (int i = gameStat.loadedMap.currentTimeIndex + 1; i < gameStat.loadedMap.timingpoints.size(); i++)
            {
                if (*gameStat.loadedMap.getCurrentTimingPoint() != gameStat.loadedMap.timingpoints.at(i))
                {
                    next_timingpoint = &gameStat.loadedMap.timingpoints.at(i);
                    break;
                }
            }

            for (int i = gameStat.loadedMap.currentTimeIndex - 1; i >= 0; i--)
            {
                if (*gameStat.loadedMap.getCurrentTimingPoint() != gameStat.loadedMap.timingpoints.at(i))
                {
                    prev_timingpoint = &gameStat.loadedMap.timingpoints.at(i);
                    break;
                }
            }

            textString = L"bpm: ";

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::White, 0.f, origin, 0.5f);

            m_fontPos.x += (XMVectorGetX(m_font->MeasureString(textString.c_str()))) * 0.5f;

            if (prev_timingpoint != nullptr)
            {
                textString = L" " + std::to_wstring(prev_timingpoint->getBPM()) + L"bpm" + (prev_timingpoint->kiai ? L"☆ " : L" ") + L"(x" + Utilities::to_wstring_with_precision(prev_timingpoint->velocity, 2) + L") ->";
                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    m_fontPos, Colors::White, 0.f, origin, 0.5f);
                m_fontPos.x += XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.5f;
            }

            textString = L" " + std::to_wstring(gameStat.loadedMap.currentBpm) + L"bpm" + (gameStat.loadedMap.kiai ? L"☆ " : L" ") + L"(x" + Utilities::to_wstring_with_precision(gameStat.loadedMap.currentSpeed, 2) + L")";
            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::Yellow, 0.f, origin, 0.5f);
            m_fontPos.x += XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.5f;

            if (next_timingpoint != nullptr)
            {
                textString = L" -> " + std::to_wstring(next_timingpoint->getBPM()) + L"bpm" + (next_timingpoint->kiai ? L"☆ " : L" ") + L"(x" + Utilities::to_wstring_with_precision(next_timingpoint->velocity, 2) + L") -" + Utilities::to_wstring_with_precision(gameStat.GetSecondsFromOsuTime(next_timingpoint->offset - gameStat.osuMapTime), 2) + L"s";
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

            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex; i < gameStat.loadedMap.hitobjects.size() && 1000 >= gameStat.osuMapTime - gameStat.loadedMap.hitobjects[i].start_time; i--)
                //(std::uint32_t i = gameStat.currentMap.currentObjectIndex + 1; i < gameStat.currentMap.hitobjects.size() && 1000 >= gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
                //forward 1s
            {
                nds++;
            }

            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex + 1; i < gameStat.loadedMap.hitobjects.size() && 1000 >= gameStat.loadedMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
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

            textString = std::wstring(L"map prog: ") + Utilities::to_wstring_with_precision((static_cast<double>(gameStat.osuMapTime) / gameStat.loadedMap.hitobjects[gameStat.loadedMap.hitobjects.size() - 1].start_time) * 100, 1) +
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

    if (gameStat.loadedMap.kiai)
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

    if (!gameStat.loadedMap.loaded ||
        (!(gameStat.loadedMap.currentObjectIndex < gameStat.loadedMap.hitobjects.size() - 1) && gameStat.loadedMap.hitobjects.back().IsCircle()
            && (gameStat.loadedMap.hitobjects.back().end_time < gameStat.osuMapTime)))
        return;

    // disable if want
    if (!gameStat.bOsuIngame)
        return;

    // not yet
    if (gameStat.hasMod(Mods::HardRock))
        return;

    std::wstring textString;

    switch (gameStat.gameMode)
    {
    case PlayMode::STANDARD: {
        hitobject *current_object = gameStat.getCurrentHitObject();
        hitobject *next_object = gameStat.loadedMap.getNextHitObject();
        hitobject *upcoming_object = gameStat.loadedMap.getUpcomingHitObject();

        if (false) // has hidden; disabled for now rewrite later: gameStat.hasMod(Mods::Hidden)
        {
            std::uint32_t comboIndex = gameStat.loadedMap.newComboIndex;
            bool bContinue = true;

            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex + 1; i < gameStat.loadedMap.hitobjects.size() && bContinue; i++)
            {
                //  Starts at 1
                //textString = std::to_wstring(i - gameStat.currentMap.currentObjectIndex);

                //  follows map combo

                bContinue = (gameStat.bOsuIngame ? 600 : 800) >= gameStat.loadedMap.hitobjects[i].start_time - gameStat.osuMapTime;

                comboIndex = gameStat.loadedMap.hitobjects[i].IsNewCombo() ? i : comboIndex;

                textString = std::to_wstring(i - comboIndex + 1);

                //std::to_wstring(gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime);

                //  first note
                if (i == gameStat.loadedMap.currentObjectIndex + 1)
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
                            257.f + gameStat.loadedMap.hitobjects[i].x * 1.5f, // padding + pixel
                            84.f + gameStat.loadedMap.hitobjects[i].y * 1.5f
                        ),
                        Colors::White, 0.f, m_font->MeasureString(textString.c_str()) * 0.5f, 0.5f);

                    m_font->DrawString(m_spriteBatch.get(), std::wstring(L" - " + std::to_wstring(gameStat.loadedMap.hitobjects[i].start_time - gameStat.osuMapTime)).c_str(),
                        DirectX::SimpleMath::Vector2(
                            257.f + gameStat.loadedMap.hitobjects[i].x * 1.5f + (XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.3f),
                            84.f + gameStat.loadedMap.hitobjects[i].y * 1.5f
                        ),
                        Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.35f);

                    if ((std::sqrt(
                        std::pow(
                            gameStat.loadedMap.hitobjects[i - 1].x - gameStat.loadedMap.hitobjects[i].x, 2
                        ) +
                        std::pow(
                            gameStat.loadedMap.hitobjects[i - 1].y - gameStat.loadedMap.hitobjects[i].y, 2
                        )
                    ) > 125.0f
                        && gameStat.loadedMap.hitobjects[i - 2].IsCircle()) || !gameStat.bOsuIngame) // todo remove1
                    {
                        double diffx = gameStat.loadedMap.hitobjects[i - 1].x - gameStat.loadedMap.hitobjects[i - 2].x;
                        double diffy = gameStat.loadedMap.hitobjects[i - 1].y - gameStat.loadedMap.hitobjects[i - 2].y;

                        double time_p = ((double)(gameStat.osuMapTime - gameStat.loadedMap.hitobjects[i - 1].start_time) / (gameStat.loadedMap.hitobjects[i - 1].start_time - gameStat.loadedMap.hitobjects[i - 2].start_time));
                        double time_persent = 1.0 + time_p;

                        DirectX::SimpleMath::Vector2 v1 = DirectX::SimpleMath::Vector2(
                            257.f + (gameStat.loadedMap.hitobjects[i - 2].x + (diffx * (time_persent >= 0.1 ? time_persent : 0.1))) * 1.5f,
                            84.f + (gameStat.loadedMap.hitobjects[i - 2].y + (diffy * (time_persent >= 0.1 ? time_persent : 0.1))) * 1.5f
                        );

                        DirectX::SimpleMath::Vector2 v2 = DirectX::SimpleMath::Vector2(
                            257.f + (gameStat.loadedMap.hitobjects[i - 1].x - (diffx * 0.1)) * 1.5f,
                            84.f + (gameStat.loadedMap.hitobjects[i - 1].y - (diffy * 0.1)) * 1.5f
                        );

                        m_batch->DrawLine(
                            DirectX::VertexPositionColor(
                                v1, i == gameStat.loadedMap.currentObjectIndex + 2 ? DirectX::Colors::Yellow : i == gameStat.loadedMap.currentObjectIndex + 3 ? Colors::Aqua : DirectX::Colors::White
                            ),
                            DirectX::VertexPositionColor(
                                v2, i == gameStat.loadedMap.currentObjectIndex + 2 ? DirectX::Colors::Yellow : i == gameStat.loadedMap.currentObjectIndex + 3 ? Colors::Aqua : DirectX::Colors::White
                            )
                        );
                    }
                }
            }
        }
        else
        {
            DirectX::SimpleMath::Vector2 slider_end;

            textString = L">    <";

            /*
                Draw the Current hitobject
            */

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                GetScreenCoordFromOsuPixelStandard(gameStat.loadedMap.getCurrentHitObject()),
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
                    GetScreenCoordFromOsuPixelStandard(gameStat.loadedMap.getCurrentHitObject()),
                    Colors::LightBlue, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                textString = L">    <";
            }

            if (!(gameStat.loadedMap.currentObjectIndex < gameStat.loadedMap.hitobjects.size() - 1))
            {
                return;
            }

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                GetScreenCoordFromOsuPixelStandard(next_object),
                Colors::Yellow, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

            auto getTime = [](int time, int32_t n_stime, int32_t c_stime) -> double { return ((double)(time - n_stime) / (n_stime - c_stime)); };

            double time_persent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->start_time));

            int32_t start_x = current_object->x, start_y = current_object->y;

            if (current_object->IsSlider())
            {
                time_persent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->end_time));
                start_x = (current_object->repeat % 2) == 0 ? current_object->x : current_object->x_sliderend_real;
                start_y = (current_object->repeat % 2) == 0 ? current_object->y : current_object->y_sliderend_real;
            }

            m_batch->DrawLine(
                DirectX::VertexPositionColor(
                    GetScreenCoordFromOsuPixelStandard(start_x, start_y, next_object->x, next_object->y, &time_persent),
                    DirectX::Colors::LightBlue
                ),
                DirectX::VertexPositionColor(
                    GetScreenCoordFromOsuPixelStandard(next_object),
                    DirectX::Colors::Yellow
                )
            );

            bool prev_slider_rev_rend_done = true;

            if (next_object->IsSlider())
            {
                time_persent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->start_time));
                time_persent = time_persent * 4;
                Overlay::DrawSlider(*next_object, gameStat.osuMapTime, Colors::Yellow, true, time_persent, &prev_slider_rev_rend_done);
            }

            if (upcoming_object != nullptr)
            {
                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    GetScreenCoordFromOsuPixelStandard(upcoming_object),
                    Colors::Aqua, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                int32_t start_x = next_object->x, start_y = next_object->y;

                if (next_object->IsSlider())
                {
                    start_x = next_object->repeat % 2 == 0 ? next_object->x : next_object->x_sliderend_real;
                    start_y = next_object->repeat % 2 == 0 ? next_object->y : next_object->y_sliderend_real;

                    if (prev_slider_rev_rend_done && next_object->IsSlider()) // prev_slider_rev_rend_done &&
                    {
                        time_persent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->start_time));
                        time_persent = time_persent * 2;
                    }
                    else {
                        time_persent = 0;
                    }

                    /*m_font->DrawString(m_spriteBatch.get(), (
                        std::to_wstring(time_persent)
                        ).c_str(),
                        DirectX::SimpleMath::Vector2(500, 500),
                        Colors::White, 0.f, DirectX::SimpleMath::Vector2(0, 0), 0.3);*/
                }

                if (!next_object->IsSlider())
                {
                    time_persent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->start_time));
                    time_persent = time_persent * 4;
                }

                m_batch->DrawLine(
                    DirectX::VertexPositionColor(
                        GetScreenCoordFromOsuPixelStandard(start_x, start_y),
                        DirectX::Colors::Yellow
                    ),
                    DirectX::VertexPositionColor(
                        GetScreenCoordFromOsuPixelStandard(start_x, start_y, upcoming_object->x, upcoming_object->y, &time_persent),
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

        int columnpx = playField_size.y / gameStat.loadedMap.CircleSize;

        float notesize = 20.f / (gameStat.loadedMap.CircleSize / 4);

        for (int col = 0; col < gameStat.loadedMap.CircleSize; ++col)
        {
            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex_sorted[col]; i < gameStat.loadedMap.hitobjects_sorted[col].size() && 1000 >= gameStat.loadedMap.hitobjects_sorted[col][i].start_time - gameStat.osuMapTime; ++i)
            {
                if (gameStat.loadedMap.hitobjects_sorted[col][i].IsCircle())
                {
                    float startpos = 540.f + (gameStat.loadedMap.hitobjects_sorted[col][i].start_time - gameStat.osuMapTime);

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
                    float startpos = 540.f + (gameStat.loadedMap.hitobjects_sorted[col][i].start_time - gameStat.osuMapTime);
                    float endpos = 540.f + (gameStat.loadedMap.hitobjects_sorted[col][i].end_time - gameStat.osuMapTime);

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

void Overlay::DrawSlider(hitobject &object, int32_t &time, DirectX::XMVECTORF32 color, bool reverse, float_t reverse_completion, bool* rev_render_complete)
{
    if (!object.IsSlider())
        return;

    std::vector<slidercurve> curves;
    slidercurve init_curve;
    DirectX::SimpleMath::Vector2 end_point = DirectX::SimpleMath::Vector2(1, 1);

    init_curve.x = object.x;
    init_curve.y = object.y;

    double length_left = object.pixel_length;

    //double time_persent = 1.0 + ((double)(time - object.start_time) / (object.start_time - object.end_time));

    int32_t time_left = object.end_time - time;
    double slider_time_actual = ((object.end_time - object.start_time) / object.repeat);
    double start_time_actual = object.start_time + ((object.end_time - object.start_time) / object.repeat) * (object.repeat - 1);
    double time_till_actual_start = start_time_actual - time;
    double completion_end_actual = time_till_actual_start > 0 ? 0.0f : (1.f - (time_left / slider_time_actual));
    if (reverse)
        completion_end_actual = std::clamp(1.f - reverse_completion, 0.f, 1.f);

    if (rev_render_complete != nullptr && reverse)
        *rev_render_complete = completion_end_actual == 0.f;
    //reverse_completion < 1 ? 1.f : reverse_completion;

/*
OutputDebugStringW(L"DrawSlider: slidercurvesize: ");
OutputDebugStringW(std::to_wstring(object.slidercurves.size()).c_str());
OutputDebugStringW(L" type: ");
OutputDebugStringW(object.slidertype.c_str());
OutputDebugStringW(L"\n");
*/

#ifdef _DEBUG
    m_font->DrawString(m_spriteBatch.get(), (
        std::to_wstring(completion_end_actual) + L" " +
        std::to_wstring(object.x_sliderend_real) + L" " +
        std::to_wstring(object.y_sliderend_real) + L" " + (reverse ? L"r:True" : L"r:False") + L" " + (rev_render_complete != nullptr && *rev_render_complete ? L"c:True" : L"c:False") + L" " +
        std::to_wstring((int)(time_left / slider_time_actual))
        ).c_str(),
        GetScreenCoordFromOsuPixelStandard(&object),
        color, 0.f, DirectX::SimpleMath::Vector2(0, 0), 0.3);
    DebugDrawSliderPoints(object.x, object.y, object.slidercurves, Colors::Brown);
#endif

    if (object.slidertype == L"B")
    {
        //std::clamp((1.f + (float)((float)(time - start_time_actual) / (start_time_actual - object.end_time))), 0.f, 1.f);
        /*
        for (int32_t i = 0; i < object.slidercurves.size(); i++)
        {
            //double time = i * timePerSection;

            if (curves.size() != 0 && curves.back() == object.slidercurves.at(i))
            {
                DrawSliderPartBiezer(init_curve, curves, length_left, color, completion_end_actual, object.repeat % 2 == 0 || reverse);
                init_curve = object.slidercurves.at(i);
                curves.clear();
            }
            curves.push_back(object.slidercurves.at(i));
        }*/

        DrawSliderPartBiezer(init_curve, object.slidercurves, length_left, color, completion_end_actual, object.repeat % 2 == 0 || reverse, &end_point);

        //temp
        //end_point = static_cast<DirectX::SimpleMath::Vector2>(object.slidercurves.back());
    }
    else if (object.slidertype == L"L")
    {
        slidercurve first_curve = init_curve;

        for (int i = 0; i < object.slidercurves.size(); i++)
        {
            //float section_completion = ((time_left - start_time_actual) - (i * (slider_time_actual / object.slidercurves.size())));
            DrawSliderLinear(first_curve, object.slidercurves.at(i), length_left, color, completion_end_actual, object.repeat % 2 == 0 || reverse, &end_point);
            first_curve = object.slidercurves.at(i);
        }

        /*DebugDrawSliderPoints(
            object.x,
            object.y,
            object.slidercurves,
            color
        );*/
    }
    else if (object.slidertype == L"P")
    {
        // PerfectCircle
        DrawSliderPerfectCircle(init_curve, object.slidercurves, length_left, color, completion_end_actual, object.repeat % 2 == 0 || reverse, &end_point);

        // temp
        //end_point = Utilities::SliderCurveToVector2(object.slidercurves.back());
    }
    // is this even correct? not that it matters..
    else if (object.slidertype == L"C")
    {
        std::vector<DirectX::VertexPositionColor> lines;

        for (double t = 0; t <= 1; t += 0.1)
        {
            lines.push_back(DirectX::VertexPositionColor(
                GetScreenCoordFromOsuPixelStandard(
                    DirectX::SimpleMath::Vector2::CatmullRom(
                        DirectX::SimpleMath::Vector2(object.x, object.y),
                        static_cast<DirectX::SimpleMath::Vector2>(object.slidercurves.at(0)),
                        static_cast<DirectX::SimpleMath::Vector2>(object.slidercurves.at(1)),
                        static_cast<DirectX::SimpleMath::Vector2>(object.slidercurves.at(2)),
                        t
                    )
                ), color
            ));
        }

        m_batch->Draw(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
            lines.data(),
            lines.size()
        );
    }
    else
    {
        DebugDrawSliderPoints(object.x, object.y, object.slidercurves, color);

        //temp
        end_point = static_cast<DirectX::SimpleMath::Vector2>(object.slidercurves.back());
    }

    object.x_sliderend_real = end_point.x;
    object.y_sliderend_real = end_point.y;
    return;  // should be sliderend
}

inline void Overlay::DrawSliderPerfectCircle(slidercurve init_point, std::vector<slidercurve> &curves, double &dist_left, DirectX::XMVECTORF32 color, float_t completion, bool reverse, DirectX::SimpleMath::Vector2 *vec)
{
    // https://github.com/ppy/osu/blob/a8579c49f9327b0a4a8278a76167e081d14ea516/osu.Game/Rulesets/Objects/CircularArcApproximator.cs
    // copy-paste modification
    if (curves.size() != 2)
    {
        throw new std::invalid_argument("More or Less than 3 Points in DrawSliderPerfectCircle! what!?");
    }

    const long double pi = (atan(1) * 4);

    DirectX::SimpleMath::Vector2 first = DirectX::SimpleMath::Vector2(init_point.x, init_point.y),  // first
        second = static_cast<DirectX::SimpleMath::Vector2>(curves.at(0)), // passthrough
        third = static_cast<DirectX::SimpleMath::Vector2>(curves.at(1));  // end

    double aSq = DirectX::SimpleMath::Vector2::DistanceSquared(second, third);
    double bSq = DirectX::SimpleMath::Vector2::DistanceSquared(first, third);
    double cSq = DirectX::SimpleMath::Vector2::DistanceSquared(first, second);

    // degenerate triangle i'mma ignore until things break

    double s = aSq * (bSq + cSq - aSq);
    double t = bSq * (aSq + cSq - bSq);
    double u = cSq * (aSq + bSq - cSq);

    float sum = s + t + u;

    // again, see above.

    DirectX::SimpleMath::Vector2 centre = (s * first + t * second + u * third) / sum;
    DirectX::SimpleMath::Vector2 dA = first - centre;
    DirectX::SimpleMath::Vector2 dC = third - centre;

    float r = dA.Length();  // is this the correct func?

    double thetaStart = std::atan2(dA.y, dA.x);
    double thetaEnd = std::atan2(dC.y, dC.x);

    while (thetaEnd < thetaStart)
        thetaEnd += 2 * pi;

    double dir = 1;
    double thetaRange = thetaEnd - thetaStart;

    // Decide in which direction to draw the circle, depending on which side of
    // AC B lies.
    DirectX::SimpleMath::Vector2 orthoAtoC = third - first;
    orthoAtoC = DirectX::SimpleMath::Vector2(orthoAtoC.y, -orthoAtoC.x);
    if (orthoAtoC.Dot(second - first) < 0)
    {
        dir = -dir;
        thetaRange = 2 * pi - thetaRange;
    }

    const float tolerance = 0.1f;

    // We select the amount of points for the approximation by requiring the discrete curvature
    // to be smaller than the provided tolerance. The exact angle required to meet the tolerance
    // is: 2 * Math.Acos(1 - TOLERANCE / r)
    // The special case is required for extremely short sliders where the radius is smaller than
    // the tolerance. This is a pathological rather than a realistic case.
    int amountPoints = 2 * r <= tolerance ? 2 : std::max(2, (int)std::ceil(thetaRange / (2 * std::acos(1 - tolerance / r))));

    std::vector<DirectX::VertexPositionColor> output;

    for (int i = 0; i < amountPoints; ++i)
    {
        double fract = (double)i / (amountPoints - 1);
        double theta = thetaStart + dir * fract * thetaRange;
        DirectX::SimpleMath::Vector2 o = DirectX::SimpleMath::Vector2((float)std::cos(theta), (float)std::sin(theta)) * r;
        output.push_back(DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(centre + o),
            color
        ));
    }

    int max = (amountPoints * (completion));

    for (int i = 0; i < max; ++i)
    {
        if (output.size() == 0)
            break;
        if (!reverse)
            output.erase(output.begin() + 0);
        else
            output.pop_back();
    }

    if (output.size() > 0)
    {
        m_batch->Draw(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
            output.data(),
            output.size()
        );
    }

    *vec = static_cast<DirectX::SimpleMath::Vector2>(curves.back());
}

/**

*/
inline void Overlay::DrawSliderLinear(slidercurve init_point, slidercurve &curve, double &dist_left, DirectX::XMVECTORF32 color, float_t inv_completion, bool reverse, DirectX::SimpleMath::Vector2 *vec)
{
    if (dist_left < 0.0)
        return;

    DirectX::SimpleMath::Vector2 init_pt = static_cast<DirectX::SimpleMath::Vector2>(init_point),
        curve_pt = static_cast<DirectX::SimpleMath::Vector2>(curve);

    DirectX::SimpleMath::Vector2 vec_final = Utilities::ClampVector2Magnitude(init_pt, curve_pt, dist_left);

    m_batch->DrawLine(
        DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(init_pt + ((vec_final - init_pt) * (reverse ? 0.f : inv_completion))), color
        ),
        DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(vec_final + ((init_pt - vec_final) * (reverse ? inv_completion : 0.f))), color
        )
    );

    *vec = vec_final;
    dist_left -= DirectX::SimpleMath::Vector2::Distance(init_pt, curve_pt);
}

void Overlay::DrawSliderPartBiezer(slidercurve init_point, std::vector<slidercurve> &curves, double &dist_left, DirectX::XMVECTORF32 color, float_t completion, bool reverse, DirectX::SimpleMath::Vector2 *vec)
{
    //if (dist_left < 0.0)
    //    return;

    int size = curves.size();
    std::vector<int> x_p, y_p;
    double x, y;
    std::vector<DirectX::VertexPositionColor> lines;
    x_p.push_back(init_point.x);
    y_p.push_back(init_point.y);
    //DirectX::SimpleMath::Vector2 prev_point = DirectX::SimpleMath::Vector2(init_point.x, y);

    for (int i = 0; i < curves.size(); i++)
    {
        x_p.push_back(curves.at(i).x);
        y_p.push_back(curves.at(i).y);

        if (i + 1 < curves.size() && curves.at(i) == curves.at(i + 1))
        {
            for (double t = 0; t <= 1; t += 0.1)
            {
                x = Utilities::getBezierPoint(&x_p, t);
                y = Utilities::getBezierPoint(&y_p, t);

                //prev_point = DirectX::SimpleMath::Vector2(x, y);

                lines.push_back(DirectX::VertexPositionColor(
                    GetScreenCoordFromOsuPixelStandard(x, y),
                    color
                ));
            }
            x_p.clear();
            y_p.clear();
        }
    }

    for (double t = 0; t <= 1; t += 0.1)
    {
        x = Utilities::getBezierPoint(&x_p, t);
        y = Utilities::getBezierPoint(&y_p, t);

        lines.push_back(DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(x, y),
            color
        ));
    }

    /*if (vec == nullptr)
        lines.push_back(DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(curves.back()),
            color
        ));*/

    int max = (lines.size() * (completion));

    for (int i = 0; i < max; ++i)
    {
        if (lines.size() == 0)
            break;
        if (!reverse)
            lines.erase(lines.begin() + 0);
        else
            lines.pop_back();
    }

    if (lines.size() > 0)
    {
        m_batch->Draw(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP,
            lines.data(),
            lines.size()
        );

#ifdef _DEBUG
        m_font->DrawString(m_spriteBatch.get(), (std::to_wstring(dist_left) + L" ").c_str(),
            DirectX::SimpleMath::Vector2(lines.back().position.x, lines.back().position.y),
            Colors::Yellow, 0.f, DirectX::SimpleMath::Vector2(0, 0), 0.3f);
#endif // _DEBUG
    }

    if (vec != nullptr)
        *vec = DirectX::SimpleMath::Vector2(Utilities::getBezierPoint(&x_p, 1.0), Utilities::getBezierPoint(&y_p, 1.0));
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

#ifdef _DEBUG
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Black);
#else
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Green);
#endif // _DEBUG

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