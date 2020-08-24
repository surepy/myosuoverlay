//
// Game.cpp
//

#include "pch.h"
#include "Signatures.h"
#include <thread>
#include <future>
#define PLAYFIELD_BOX false

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

    CalculateScreenCoordOffsets();
    CalculateScreenCoordOffsetsCursor();
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
            gameStat.cursorLocation = //DirectX::SimpleMath::Vector2(static_cast<float>(cursorLocationCurrent.x), static_cast<float>(cursorLocationCurrent.y));
                DirectX::SimpleMath::Vector2(static_cast<float>(cursorLocationCurrent.x - x_offset_cursor) / x_multiplier_cursor, 
                    static_cast<float>(cursorLocationCurrent.y - y_offset_cursor) / y_multiplier_cursor);
                    
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
// Never allow me to write UI code
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
    //m_d3dContext->RSSetState(m_states->CullNone());
    m_d3dContext->RSSetState(m_raster.Get());

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

    m_fontPos = (gameStat.bOsuIngame ? DirectX::SimpleMath::Vector2(m_outputWidth / 2 + m_outputWidth / 3, 10) : DirectX::SimpleMath::Vector2(static_cast<float>(m_outputWidth), m_outputHeight - 21.f - 45.f));

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::HotPink, Colors::Black, 2);

    /*
     * Kps + (totalclicks) string
     */

    textString = L"";
    //std::wstring(L"kps: ") + std::to_wstring(gameStat.clicks.size()) + std::wstring(L" (") + std::to_wstring(gameStat.clickCounter) + std::wstring(L")");

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos = DirectX::SimpleMath::Vector2(0.f + 370.f, m_outputHeight - 21.f);

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::Red, Colors::Black, 2);

    origin = DirectX::SimpleMath::Vector2(0.f, 0.f);

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

    textString = L"";
    //textString = std::wstring(L"x: ") + std::to_wstring(static_cast<int>(gameStat.cursorLocation.x)) + std::wstring(L" y: ") + std::to_wstring(static_cast<int>(gameStat.cursorLocation.y));

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos.y -= 18;  

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::DeepSkyBlue, Colors::Black, 2, 0.45f);

    /*
     *  current HP
     */

    textString = std::wstring(gameStat.mfOsuPlayHP->ReadToEnd()).size() == 3 ? L"" : static_cast<std::wstring>(gameStat.mfOsuPlayHP->ReadToEnd());

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos.y -= 21;  

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::LightSkyBlue, Colors::Black, 2, 0.5f);

    /*
     *  current pp
     */

    textString = std::wstring(gameStat.mfOsuPlayPP->ReadToEnd()).size() == 3 ? L"" : static_cast<std::wstring>(gameStat.mfOsuPlayPP->ReadToEnd());

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos.y -= 24; 

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::Yellow, Colors::Black, 2, 0.55f);

    RenderStatTexts(gameStat);
    RenderAssistant(gameStat);

    m_batch->End();
    m_spriteBatch->End();

    Present();
}

DirectX::SimpleMath::Vector2 Overlay::DrawText(std::wstring text, float size, DirectX::XMVECTORF32 color, DirectX::SimpleMath::Vector2* pos_force, DirectX::SimpleMath::Vector2* offset)
{
    return this->DrawText(text, size, color, false, pos_force, offset);
}

DirectX::SimpleMath::Vector2 Overlay::DrawText(std::wstring text, float size, DirectX::XMVECTORF32 color, bool direction, DirectX::SimpleMath::Vector2* pos_force, DirectX::SimpleMath::Vector2* offset)
{
    if (pos_force != nullptr)
        m_fontPos = *pos_force;

    if (offset == nullptr)
        m_font->DrawString(m_spriteBatch.get(), text.c_str(), m_fontPos, color, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), size);
    else 
        m_font->DrawString(m_spriteBatch.get(), text.c_str(), m_fontPos, color, 0.f, *offset, size);

    DirectX::XMVECTOR string_size = m_font->MeasureString(text.c_str()) * size;
    DirectX::SimpleMath::Vector2 vec = m_fontPos;
    float offset_y = XMVectorGetY(string_size);
    m_fontPos.y += (direction ? -offset_y : +offset_y);

    // return data
    
    vec.x += XMVectorGetX(string_size);
    return vec;
}

//  Assumes font->Begin() and batch->Begin() is called.
void Overlay::RenderStatTexts(osuGame &gameStat)
{
    std::wstring textString;

    if (!*gameStat.bOsuLoaded && !*gameStat.bOsuLoading)
    {
        this->DrawText(L"Game not detected!", 0.4f, Colors::White, &DirectX::SimpleMath::Vector2(0.f, m_outputHeight / 8), nullptr);
        return;
    }
    else if (!*gameStat.bOsuLoaded && *gameStat.bOsuLoading)
    {
        this->DrawText(L"Loading game.. one second! (10s+)", 0.4f, Colors::White, &DirectX::SimpleMath::Vector2(0.f, m_outputHeight / 8), nullptr);
        return;
    }
    else if (!gameStat.loadedMap.loaded)
    {
        this->DrawText(L"Map Loading... (Most likely Map load fail..)", 0.4f, Colors::LightBlue, &DirectX::SimpleMath::Vector2(0.f, m_outputHeight / 8), nullptr);
        return;
    }

    DirectX::SimpleMath::Vector2 origin = DirectX::SimpleMath::Vector2(0.f, 0.f);

    if (!gameStat.bOsuIngame)
    {
        timingpoint *current_timingpoint = gameStat.loadedMap.getCurrentTimingPoint();
        if (current_timingpoint == nullptr)
            return;

        this->DrawText(
            std::to_wstring(gameStat.GetActualBPM(current_timingpoint->getBPM()))
            + L"bpm" + (current_timingpoint->kiai ? L"☆" : L"") +
            L" (x" + Utilities::to_wstring_with_precision(current_timingpoint->velocity, 2) + L") " +
            L" setid:" + std::to_wstring(gameStat.loadedMap.BeatmapSetID) +
            L" id:" + std::to_wstring(gameStat.loadedMap.BeatMapID), 0.4f, Colors::Yellow, &DirectX::SimpleMath::Vector2(-1.f, m_outputHeight / 8), nullptr);
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


        origin = DirectX::SimpleMath::Vector2(0.f, 0.f);


        //this->DrawText(std::to_wstring(gameStat.hp) + L"", 0.4f, Colors::White);

        /*
         *  Current note.
         *
         */
        if (current_object != nullptr && gameStat.osuMapTime < gameStat.loadedMap.hitobjects[gameStat.loadedMap.hitobjects.size() - 1].end_time)
        {
            textString = 
                L"current: x: " + std::to_wstring(current_object->x) +
                L" y: " + std::to_wstring(gameStat.hasMod(Mods::HardRock) ? 384 - current_object->y : current_object->y) +
                L" index: " + std::to_wstring(gameStat.loadedMap.currentObjectIndex) + 
                L"/" + std::to_wstring(gameStat.loadedMap.hitobjects.size() - 1);

            if (current_object->IsSlider() || current_object->IsSpinner())
            {
                textString += L" time: ";

                if (((int32_t)gameStat.osuMapTime < current_object->end_time))
                {
                    textString += std::to_wstring(static_cast<int>(gameStat.GetSecondsFromOsuTime(current_object->end_time - gameStat.osuMapTime) * 1000)) + L"ms";
                }
                else
                {
                    textString += L"done!";
                }
            }


            this->DrawText(textString, 0.4f, Colors::LightBlue, &DirectX::SimpleMath::Vector2(0.f, m_outputHeight / 22), nullptr);
        }

        /*
         * cursor
         *
         */
        if ((current_object != nullptr && gameStat.osuMapTime < current_object->end_time) || (next_object != nullptr))
        {
            slidercurve* pos = nullptr;

            if (current_object->IsSlider() && gameStat.osuMapTime < current_object->end_time)
            {
                double slider_progression_time = gameStat.osuMapTime - current_object->start_time;
                double slider_time_actual = ((current_object->end_time - current_object->start_time) / current_object->repeat);
                double slider_progress_rate = slider_progression_time / slider_time_actual;
                double slider_progress_actual = slider_progress_rate - (int)slider_progress_rate;
                bool slider_reverse = ((int)slider_progress_rate % 2) == 1;
                double completion_end_actual = slider_reverse ? 1.f - slider_progress_actual : slider_progress_actual;


                if (current_object->slidertype == L"B" || current_object->slidertype == L"P")
                {
                    int index = ((current_object->slidercurves_calculated.size() - 1) * std::clamp(completion_end_actual, 0.0, 1.0));

                    pos = &current_object->slidercurves_calculated.at(index);
                }
                else if (current_object->slidertype == L"L")
                {
                    DirectX::SimpleMath::Vector2 init_pt = static_cast<DirectX::SimpleMath::Vector2>(*current_object),
                        curve_pt = static_cast<DirectX::SimpleMath::Vector2>(current_object->slidercurves.back());

                    pos = &static_cast<slidercurve>(init_pt + ((curve_pt - init_pt) * (slider_reverse ? 0.f : std::clamp(completion_end_actual, 0.0, 1.0))));
                }
            }
            else if (current_object->IsSpinner()) 
            {
                pos = &slidercurve(current_object->x, current_object->y);
            }
            
            if (pos != nullptr)
            {
                textString =
                    L"cursor: x: " + std::to_wstring(pos->x) +
                    L" y: " + std::to_wstring(gameStat.hasMod(Mods::HardRock) ? 384 - pos->y : pos->y);

                if (current_object->IsSpinner())
                    textString += L" (Spinner)";
            }
            else if (current_object->IsSlider())
            {
                double time_percent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->end_time));

                DirectX::SimpleMath::Vector2 pos_2 =
                    (
                        static_cast<DirectX::SimpleMath::Vector2>(current_object->slidercurves.back()) + ((
                        static_cast<DirectX::SimpleMath::Vector2>(*next_object) -
                        static_cast<DirectX::SimpleMath::Vector2>(current_object->slidercurves.back())) *
                        static_cast<float_t>(std::clamp(time_percent, 0.0, 1.0))));

                textString =
                    L"cursor: x: " + std::to_wstring((int)pos_2.x) + L" y: " + std::to_wstring((int)(gameStat.hasMod(Mods::HardRock) ? 384 - pos_2.y : pos_2.y));
            }
            else if (current_object->IsCircle())
            {
                double time_percent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->start_time));

                DirectX::SimpleMath::Vector2 pos_2 =
                    (*current_object + ((
                        static_cast<DirectX::SimpleMath::Vector2>(*next_object) -
                        static_cast<DirectX::SimpleMath::Vector2>(*current_object)) *
                        static_cast<float_t>(std::clamp(time_percent, 0.0, 1.0))));

                textString =
                    L"cursor: x: " + std::to_wstring((int)pos_2.x) + L" y: " + std::to_wstring((int)(gameStat.hasMod(Mods::HardRock) ? 384 - pos_2.y : pos_2.y));
            }
            else {
                textString = L"cursor: x: ??? y: ??? (unsupported)";
            }

            // for dumb aspire objects, upto 20.
            int count = 0;
            for (std::int32_t i = gameStat.loadedMap.currentObjectIndex - 1; i > 0 && i >= (int32_t)gameStat.loadedMap.currentObjectIndex - 20; i--)
            {
                if (!gameStat.loadedMap.hitobjects.at(i).IsSlider())
                    continue;

                if (gameStat.loadedMap.hitobjects.at(i).end_time > gameStat.osuMapTime)
                {
                    count++;
                }
            }

            if (count != 0)
            {
                textString += L" Overlapping: " + std::to_wstring(count) + (count == 20 ? L"+." : L".");
            }

            this->DrawText(textString, 0.4f, Colors::LightPink, nullptr, nullptr);
        }

        /*
         *  next note:
         *
         */

        if (next_object != nullptr)
        {
            textString = L"subseq: x: " + std::to_wstring(next_object->x) +
                L" y: " + std::to_wstring(gameStat.hasMod(Mods::HardRock) ? 384 - next_object->y : next_object->y) +
                L" objdist: " +
                Utilities::to_wstring_with_precision(
                    DirectX::SimpleMath::Vector2::Distance(*next_object, *current_object), 1
                ) +
                std::wstring(L" time left: ") + std::to_wstring(static_cast<int>(gameStat.GetSecondsFromOsuTime(next_object->start_time - gameStat.osuMapTime) * 1000)) + L"ms";

            this->DrawText(textString, 0.4f, Colors::Yellow, nullptr, nullptr);
        }

        /*
         *  upcoming note
         *
         */

        if (upcoming_object != nullptr)
        {
            textString = std::wstring(L"follow: x: ") + std::to_wstring(upcoming_object->x) +
                std::wstring(L" y: ") + std::to_wstring(gameStat.hasMod(Mods::HardRock) ? 384 - upcoming_object->y : upcoming_object->y) +
                std::wstring(L" objdist: ") + Utilities::to_wstring_with_precision(
                    DirectX::SimpleMath::Vector2::Distance(*upcoming_object, *next_object), 1
                ) +
                std::wstring(L" time left: ") + std::to_wstring(static_cast<int>(gameStat.GetSecondsFromOsuTime(upcoming_object->start_time - gameStat.osuMapTime) * 1000)) + L"ms";

            this->DrawText(textString, 0.4f, Colors::Aqua, nullptr, nullptr);
        }

        /*
         *  Others
         *
         */

        if (gameStat.osuMapTime < gameStat.loadedMap.hitobjects[gameStat.loadedMap.hitobjects.size() - 1].end_time)
        {
            timingpoint *current_timingpoint = gameStat.loadedMap.getCurrentTimingPoint(), 
                *next_timingpoint = gameStat.loadedMap.getCurrentTimingPoint(1),
                *prev_timingpoint = gameStat.loadedMap.getCurrentTimingPoint(-1);


            textString = L"bpm: ";

            m_fontPos = this->DrawText(textString, 0.4f, Colors::White);

            if (prev_timingpoint != nullptr)
            {
                textString = L" " + std::to_wstring(gameStat.GetActualBPM(prev_timingpoint->getBPM())) + L"bpm" + (prev_timingpoint->kiai ? L"☆ " : L" ") + 
                    // multiplier
                    L"(x" + Utilities::to_wstring_with_precision(prev_timingpoint->velocity, 2) + L") ->";
                m_fontPos = this->DrawText(textString, 0.325f, Colors::White, nullptr, &DirectX::SimpleMath::Vector2(0, -(XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.325f) / 2));
            }

            textString = L" [" + std::to_wstring(gameStat.loadedMap.currentTimeIndex) + L"] " + std::to_wstring(gameStat.GetActualBPM(current_timingpoint->getBPM())) + L"bpm" + (current_timingpoint->kiai ? L"☆ " : L" ") + L"(x" + Utilities::to_wstring_with_precision(current_timingpoint->velocity, 2) + L") ";
            
            m_fontPos = this->DrawText(textString, 0.4f, Colors::Yellow);

            // TODO auto bpm
            if (next_timingpoint != nullptr)
            {
                textString = L" -> " + std::to_wstring(gameStat.GetActualBPM(next_timingpoint->getBPM())) + L"bpm" + (next_timingpoint->kiai ? L"☆ " : L" ") + 
                    // multiplier
                    L"(x" + Utilities::to_wstring_with_precision(next_timingpoint->velocity, 2) + 
                    // seconds left over
                    L") -" + Utilities::to_wstring_with_precision(gameStat.GetSecondsFromOsuTime(next_timingpoint->offset - gameStat.osuMapTime), 2) + L"s " ;
                m_fontPos = this->DrawText(textString, 0.325f, Colors::White, nullptr, &DirectX::SimpleMath::Vector2(0, -(XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.325f) / 2));
            }

            m_fontPos.x = 0.f;
            m_fontPos.y += XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.4f;

            /*
                skip line
            */

            std::wstring nds_warning = L"";

            //back 1s
            std::uint32_t nds = 0;
            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex; i < gameStat.loadedMap.hitobjects.size() &&
                (gameStat.hasMod(Mods::DoubleTime) || gameStat.hasMod(Mods::Nightcore) ? 1500 : 1000) >= gameStat.osuMapTime - gameStat.loadedMap.hitobjects[i].start_time; i--)
            {
                nds++;
            }

            std::double_t nds_avg = 0;
            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex; i < gameStat.loadedMap.hitobjects.size() &&
                (gameStat.hasMod(Mods::DoubleTime) || gameStat.hasMod(Mods::Nightcore) ? 3000 : 2000) >= gameStat.osuMapTime - gameStat.loadedMap.hitobjects[i].start_time; i--)
            {
                nds_avg++;
            }

            nds_avg /= 2;


            //forward 1s
            //std::uint32_t nds_f = 0;
            //for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex + 1; i < gameStat.loadedMap.hitobjects.size() && 1000 >= gameStat.loadedMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
            //{
            //    nds_f++;
            //}

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
                std::wstring(L":") + std::to_wstring(gameStat.beatIndex % 4);

            m_fontPos = this->DrawText(textString, 0.4f, Colors::White);

            this->DrawText(
                (L" note/sec: " + std::to_wstring(nds) + L" (" + Utilities::to_wstring_with_precision(nds_avg, 1) + L")" + nds_warning),
                0.4f, 
                { { { 1.00f, nds > 9 ? 1.00f - (nds - 9) * (1.00f / 12.f) : 1.f, nds > 12 ? 1.00f - (nds - 9) * (1.00f / 12.f) : 1.f, 1.00f } } }
            );

            m_fontPos.x = 0;

            
            m_fontPos = this->DrawText(L"stat:" , 0.4f, Colors::White);
            m_fontPos = this->DrawText(L" " + std::to_wstring(gameStat.hit300), 0.4f, Colors::Yellow);
            m_fontPos = this->DrawText(L" /", 0.4f, Colors::White);
            m_fontPos = this->DrawText(L" " + std::to_wstring(gameStat.hit100), 0.4f, Colors::LightBlue);
            m_fontPos = this->DrawText(L" /", 0.4f, Colors::White);
            m_fontPos = this->DrawText(L" " + std::to_wstring(gameStat.hit50), 0.4f, Colors::Green);
            m_fontPos = this->DrawText(L" /", 0.4f, Colors::White);
            this->DrawText(L" " + std::to_wstring(gameStat.hitmiss), 0.4f, Colors::Red);

            m_fontPos.x = 0;
        }

        break;
    }
    case PlayMode::MANIA: {
        hitobject *next_object;
        hitobject *current_object;
        origin = DirectX::SimpleMath::Vector2(0, 0);
        m_fontPos = DirectX::SimpleMath::Vector2(m_outputWidth * 0.4453125, m_outputHeight * 0.125);
        // m_fontPos = DirectX::SimpleMath::Vector2(570.f, 90.f);

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
                m_fontPos.y += XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.4f;
                continue;
            }

            std::uint32_t nds = 0;

            std::wstring nds_warning = L"";

            for (std::uint32_t o = objIndex; o < gameStat.loadedMap.hitobjects_sorted[i].size() && 1000 >= gameStat.osuMapTime - gameStat.loadedMap.hitobjects_sorted[i][o].start_time; o--)
            {
                nds++;
            }

            //m_fontPos.x += 15;
            m_fontPos.y += XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.4f;
            //m_fontPos.x = 600.f;
            m_fontPos.x += 30;

            textString = std::wstring(L" / next: hold?: ") + ((*next_object).IsHold() ? L"Yes" : L"No") + std::wstring(L" note/sec: ") + std::to_wstring(nds) + std::wstring(L" time: ") +
                std::to_wstring((*next_object).start_time - gameStat.osuMapTime) + L"/" + std::to_wstring((*next_object).start_time - (*current_object).end_time);

            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                m_fontPos, Colors::Yellow, 0.f, origin, 0.4f);

            m_fontPos.y += XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.4f;
            m_fontPos.x -= 30;
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

            m_fontPos.x = m_outputWidth * 0.4453125;
            m_fontPos.y += XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.4f;

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
            m_fontPos.y += XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.4f;
        }

        break;
    case PlayMode::TAIKO:
        origin = DirectX::SimpleMath::Vector2(0.f, 0.f);
        m_fontPos = DirectX::SimpleMath::Vector2(m_outputWidth * 0.1328125, m_outputHeight * 0.52777777777);
        // m_fontPos = DirectX::SimpleMath::Vector2(170.f, 380.f);

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

            m_fontPos.x = m_outputWidth * 0.1328125;
            m_fontPos.y += XMVectorGetY(m_font->MeasureString(textString.c_str())) * 0.6f;

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
        this->DrawText(std::wstring(L"kiai mode! " + std::to_wstring((gameStat.beatIndex % 4) + 1) + ((gameStat.beatIndex % 4) % 2 == 0 ? L" >" : L" <")), 0.4f, beat_color);
    }
}

//  Assumes font->Begin() and batch->Begin() is called.
void Overlay::RenderAssistant(osuGame &gameStat)
{
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

    std::wstring textString;

    switch (gameStat.gameMode)
    {
    case PlayMode::STANDARD: {
        hitobject *current_object = gameStat.getCurrentHitObject();
        hitobject *next_object = gameStat.loadedMap.getNextHitObject();
        hitobject *upcoming_object = gameStat.loadedMap.getUpcomingHitObject();

        // for dumb aspire objects, upto 20.
        for (std::int32_t i = gameStat.loadedMap.currentObjectIndex - 1; i > 0 && i >= (int32_t)gameStat.loadedMap.currentObjectIndex - 20; i--)
        {
            if (!gameStat.loadedMap.hitobjects.at(i).IsSlider())
                continue;

            if (gameStat.loadedMap.hitobjects.at(i).end_time > gameStat.osuMapTime)
            {
                Overlay::DrawSlider(gameStat.loadedMap.hitobjects.at(i), gameStat.osuMapTime, Colors::Green, gameStat.hasMod(Mods::HardRock));
            }
        }

        if (gameStat.hasMod(Mods::Hidden)) // has hidden;
        {
            std::size_t comboIndex = gameStat.loadedMap.newComboIndex;
            bool bContinue = true;
            bool bNextContinue = true;
            DirectX::SimpleMath::Vector2 last_start_vec;

            for (std::size_t i = gameStat.loadedMap.currentObjectIndex; i < gameStat.loadedMap.hitobjects.size() && bContinue; i++)
            {
                //  Starts at 1
                //textString = std::to_wstring(i - gameStat.currentMap.currentObjectIndex);

                //  follows map combo
                bContinue = gameStat.GetARDelay(gameStat.getAR_Real(gameStat.loadedMap.ApproachRate)) >= gameStat.loadedMap.hitobjects[i].start_time - gameStat.osuMapTime;
                bNextContinue = (i + 1) < gameStat.loadedMap.hitobjects.size() &&
                    gameStat.GetARDelay(gameStat.getAR_Real(gameStat.loadedMap.ApproachRate)) >= gameStat.loadedMap.hitobjects[i + 1].start_time - gameStat.osuMapTime;

                if (!bContinue)
                    return;

                comboIndex = gameStat.loadedMap.hitobjects[i].IsNewCombo() ? i : comboIndex;

                textString = std::to_wstring(i - comboIndex + 1);
                double time_percent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->start_time));

                //  first note
                if (i == gameStat.loadedMap.currentObjectIndex)
                {
                    textString = L">    <";

                    m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                        GetScreenCoordFromOsuPixelStandard(current_object, gameStat.hasMod(Mods::HardRock)),
                        Colors::LightBlue, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                    last_start_vec = *current_object;

                    if (current_object->IsSlider())
                    {
                        last_start_vec = DirectX::SimpleMath::Vector2(
                            (current_object->repeat % 2) == 0 ? current_object->x : current_object->slidercurves_calculated.back().x,
                            (current_object->repeat % 2) == 0 ? current_object->y : current_object->slidercurves_calculated.back().y
                        );

                        if (current_object->end_time > gameStat.osuMapTime)
                            Overlay::DrawSlider(*current_object, gameStat.osuMapTime, Colors::LightBlue, gameStat.hasMod(Mods::HardRock));
                    }
                    else if (current_object->IsSpinner() && current_object->end_time > gameStat.osuMapTime)
                    {
                        textString = Utilities::to_wstring_with_precision(gameStat.GetSecondsFromOsuTime(current_object->end_time - gameStat.osuMapTime), 1) + L"s";

                        m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                            GetScreenCoordFromOsuPixelStandard(current_object, gameStat.hasMod(Mods::HardRock)),
                            Colors::LightBlue, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                        textString = L">    <";
                    }
                }
                else if (i == gameStat.loadedMap.currentObjectIndex + 1)
                {
                    if (((i - comboIndex + 1) > 9))
                        textString = L"> " + textString + L" <";
                    else
                        textString = L">  " + textString + L" <";

                    m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                        GetScreenCoordFromOsuPixelStandard(*next_object, gameStat.hasMod(Mods::HardRock)),
                        Colors::Yellow, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                    if (upcoming_object != nullptr && DirectX::SimpleMath::Vector2::Distance(*next_object, *upcoming_object) < 1.f)
                    {
                        int repeat_count = 0;
                        for (std::size_t i_2 = i; i_2 < gameStat.loadedMap.hitobjects.size(); i_2++)
                        {
                            if (!(gameStat.GetARDelay(gameStat.getAR_Real(gameStat.loadedMap.ApproachRate)) >= gameStat.loadedMap.hitobjects[i_2].start_time - gameStat.osuMapTime))
                                break;
                            if (DirectX::SimpleMath::Vector2::Distance(*next_object, *upcoming_object) > 1.f)
                                break;

                            repeat_count++;
                        }

                        m_font->DrawString(m_spriteBatch.get(), std::wstring(L" - " + std::to_wstring((*next_object).start_time - gameStat.osuMapTime) + L" x" + std::to_wstring(repeat_count)).c_str(),
                            GetScreenCoordFromOsuPixelStandard(*next_object, gameStat.hasMod(Mods::HardRock)) +
                            DirectX::SimpleMath::Vector2((XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.3f), 0),
                            Colors::Yellow, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.35f);
                    }
                    else 
                    {
                        m_font->DrawString(m_spriteBatch.get(), std::wstring(L" - " + std::to_wstring((*next_object).start_time - gameStat.osuMapTime)).c_str(),
                            GetScreenCoordFromOsuPixelStandard(*next_object, gameStat.hasMod(Mods::HardRock)) +
                            DirectX::SimpleMath::Vector2((XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.3f), 0),
                            Colors::Yellow, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.35f);
                    }

                    //  draw the slider.
                    Overlay::DrawSlider(*next_object, gameStat.osuMapTime, Colors::Yellow, gameStat.hasMod(Mods::HardRock));

                    //  temp for aspire sliders
                    if (current_object->end_time > next_object->start_time)
                        gameStat.loadedMap.aspire_dumb_slider_hitobjects.push_back(*current_object);
                }
                else
                {
                    if (DirectX::SimpleMath::Vector2::Distance(gameStat.loadedMap.hitobjects[i - 1], gameStat.loadedMap.hitobjects[i]) > 1.f)
                    {
                        m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                            GetScreenCoordFromOsuPixelStandard(gameStat.loadedMap.hitobjects[i], gameStat.hasMod(Mods::HardRock)),
                            Colors::White, 0.f, m_font->MeasureString(textString.c_str()) * 0.5f, 0.5f);

                        m_font->DrawString(m_spriteBatch.get(), std::wstring(L" - " + std::to_wstring(gameStat.loadedMap.hitobjects[i].start_time - gameStat.osuMapTime)).c_str(),
                            GetScreenCoordFromOsuPixelStandard(gameStat.loadedMap.hitobjects[i], gameStat.hasMod(Mods::HardRock)) +
                            DirectX::SimpleMath::Vector2((XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.3f), 0),
                            Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.35f);
                    }

                    //  draw the slider.
                    Overlay::DrawSlider(gameStat.loadedMap.hitobjects.at(i), gameStat.osuMapTime, Colors::White, gameStat.hasMod(Mods::HardRock));
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
                GetScreenCoordFromOsuPixelStandard(gameStat.loadedMap.getCurrentHitObject(), gameStat.hasMod(Mods::HardRock)),
                Colors::LightBlue, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

            /*
                Extra stuff for Current Hitobject
            */

            if (current_object->IsSlider() && current_object->end_time > gameStat.osuMapTime)
            {
                Overlay::DrawSlider(*current_object, gameStat.osuMapTime, Colors::LightBlue, gameStat.hasMod(Mods::HardRock));
            }
            else if (current_object->IsSpinner() && current_object->end_time > gameStat.osuMapTime)
            {
                textString = Utilities::to_wstring_with_precision(gameStat.GetSecondsFromOsuTime(current_object->end_time - gameStat.osuMapTime), 1) + L"s";

                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    GetScreenCoordFromOsuPixelStandard(gameStat.loadedMap.getCurrentHitObject(), gameStat.hasMod(Mods::HardRock)),
                    Colors::LightBlue, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                textString = L">    <";
            }

            if (!(gameStat.loadedMap.currentObjectIndex < gameStat.loadedMap.hitobjects.size() - 1))
            {
                return;
            }

            // draw next object.
            m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                GetScreenCoordFromOsuPixelStandard(next_object, gameStat.hasMod(Mods::HardRock)),
                Colors::Yellow, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);
            //

            auto getTime = [](int time, int32_t n_stime, int32_t c_stime) -> double { return ((double)(time - n_stime) / (n_stime - c_stime)); };

            double time_percent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->start_time));

            int32_t start_x = current_object->x, start_y = current_object->y;

            if (current_object->IsSlider())
            {
                time_percent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->end_time));
                start_x = (current_object->repeat % 2) == 0 ? current_object->x : current_object->slidercurves_calculated.back().x;
                start_y = (current_object->repeat % 2) == 0 ? current_object->y : current_object->slidercurves_calculated.back().y;
            }

            // draw current object -> next object line.
            m_batch->DrawLine(
                DirectX::VertexPositionColor(
                    GetScreenCoordFromOsuPixelStandard(start_x, start_y, next_object->x, next_object->y, &time_percent, gameStat.hasMod(Mods::HardRock)),
                    DirectX::Colors::LightBlue
                ),
                DirectX::VertexPositionColor(
                    GetScreenCoordFromOsuPixelStandard(next_object, gameStat.hasMod(Mods::HardRock)),
                    DirectX::Colors::Yellow
                )
            );

            // next slider render is done?
            bool prev_slider_rev_rend_done = true;
            // current to next_object
            double distCtoNObj = DirectX::SimpleMath::Vector2::Distance(DirectX::SimpleMath::Vector2(start_x, start_y), *next_object);
            // total distance we have to travel
            double distTotal = distCtoNObj + next_object->pixel_length;
            // ratio
            double distRate = distCtoNObj / distTotal;
            time_percent = 1.0 + ((double)(gameStat.osuMapTime - next_object->start_time) / (next_object->start_time - current_object->start_time));
            time_percent = time_percent * 1.75;

            if (next_object->IsSlider())
            {
                Overlay::DrawSlider(*next_object, gameStat.osuMapTime, Colors::Yellow, gameStat.hasMod(Mods::HardRock), true, time_percent / distRate, &prev_slider_rev_rend_done);
            }

            if (upcoming_object != nullptr)
            {
                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    GetScreenCoordFromOsuPixelStandard(upcoming_object, gameStat.hasMod(Mods::HardRock)),
                    Colors::Aqua, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                int32_t start_x = next_object->x, start_y = next_object->y;

                if (next_object->IsSlider())
                {
                    start_x = next_object->repeat % 2 == 0 ? next_object->x : next_object->slidercurves_calculated.back().x;
                    start_y = next_object->repeat % 2 == 0 ? next_object->y : next_object->slidercurves_calculated.back().y;

                    if (prev_slider_rev_rend_done && next_object->IsSlider())
                    {
                        time_percent = (time_percent - distRate);
                    }
                    else {
                        time_percent = 0;
                    }
                }

                m_batch->DrawLine(
                    DirectX::VertexPositionColor(
                        GetScreenCoordFromOsuPixelStandard(start_x, start_y, gameStat.hasMod(Mods::HardRock)),
                        DirectX::Colors::Yellow
                    ),
                    DirectX::VertexPositionColor(
                        GetScreenCoordFromOsuPixelStandard(start_x, start_y, upcoming_object->x, upcoming_object->y, &time_percent, gameStat.hasMod(Mods::HardRock)),
                        DirectX::Colors::Aqua
                    )
                );
            }
        }
#if PLAYFIELD_BOX == true
        m_batch->DrawLine(
            DirectX::VertexPositionColor(
                GetScreenCoordFromOsuPixelStandard(0, 0),
                DirectX::Colors::White
            ),
            DirectX::VertexPositionColor(
                GetScreenCoordFromOsuPixelStandard(0, 0),
                DirectX::Colors::White
            )
        );
#endif
        }
    case PlayMode::MANIA: {
        // render vertically. until then don't draw anything.
        if (!gameStat.bOsuIngame)
            return;
        // origin: x= 540.f, y= 360.f

        DirectX::SimpleMath::Vector3 v1, v2, v3, v4;
        DirectX::XMVECTORF32 bgColor = Colors::White;

        // this defines where we start
        DirectX::SimpleMath::Vector3 playField = DirectX::SimpleMath::Vector3(m_outputWidth * 0.421875, m_outputHeight / 2, 0);
        // this only defines the size; x is x and y is y. it's not flipped or anything.
        DirectX::SimpleMath::Vector2 playField_size = DirectX::SimpleMath::Vector2(m_outputWidth / 0.578125, m_outputHeight / 3);
        //DirectX::SimpleMath::Vector2 playField_size = DirectX::SimpleMath::Vector2(740, gameStat.loadedMap.CircleSize * 40);

        // this works well for >=7k
        // everything else... well..
        if (gameStat.loadedMap.CircleSize != 7)
        {
            playField.x += (playField_size.y / gameStat.loadedMap.CircleSize * (gameStat.loadedMap.CircleSize - 7) / 1.5);
        }

        int columnpx = playField_size.y / gameStat.loadedMap.CircleSize;

        float notesize = 20.f / (gameStat.loadedMap.CircleSize / 4);

        for (int col = 0; col < gameStat.loadedMap.CircleSize; ++col)
        {
            for (std::uint32_t i = gameStat.loadedMap.currentObjectIndex_sorted[col]; i < gameStat.loadedMap.hitobjects_sorted[col].size() && 1000 >= gameStat.loadedMap.hitobjects_sorted[col][i].start_time - gameStat.osuMapTime; ++i)
            {
                if (gameStat.loadedMap.hitobjects_sorted[col][i].IsCircle())
                {
                    float startpos = playField.x + (gameStat.loadedMap.hitobjects_sorted[col][i].start_time - gameStat.osuMapTime);

                    if (startpos <= playField.x)
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
                    float startpos = playField.x + (gameStat.loadedMap.hitobjects_sorted[col][i].start_time - gameStat.osuMapTime);
                    float endpos = playField.x + (gameStat.loadedMap.hitobjects_sorted[col][i].end_time - gameStat.osuMapTime);

                    if (startpos < playField.x)
                        startpos = playField.x;

                    if (endpos >= m_outputWidth)
                        endpos = m_outputWidth;
                    if (endpos <= playField.x)
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

void Overlay::DebugDrawSliderPoints(int x, int y, std::vector<slidercurve> &points, DirectX::XMVECTORF32 color, bool hardrock)
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
                GetScreenCoordFromOsuPixelStandard(x1, y1, hardrock), color
            ),
            DirectX::VertexPositionColor(
                GetScreenCoordFromOsuPixelStandard(x2, y2, hardrock), color
            )
        );

        m_font->DrawString(m_spriteBatch.get(), std::to_wstring(num).c_str(),
            GetScreenCoordFromOsuPixelStandard(x2, y2, hardrock),
            color, 0.f, DirectX::SimpleMath::Vector2(0, 0), 0.3);

        num++;

        x1 = x2;
        y1 = y2;
    }
}

void Overlay::DrawSlider(hitobject &object, int32_t &time, DirectX::XMVECTORF32 color, bool hardrock, bool reverse, float_t reverse_completion, bool* rev_render_complete)
{
    if (!object.IsSlider())
        return;

#ifdef _DEBUG
    DebugDrawSliderPoints(object.x, object.y, object.slidercurves, Colors::Brown);
#endif 

    std::vector<slidercurve> curves;
    slidercurve init_curve (object.x, object.y);
    DirectX::SimpleMath::Vector2 end_point = DirectX::SimpleMath::Vector2(1, 1);

    // Unused, remove
    double length_left = object.pixel_length;

    int32_t time_left = object.end_time - time;
    double slider_time_actual = ((object.end_time - object.start_time) / object.repeat);
    double start_time_actual = static_cast<double>(object.start_time + ((object.end_time - object.start_time) / object.repeat) * (object.repeat - 1));
    double time_till_actual_start = start_time_actual - time;
    double completion_end_actual = time_till_actual_start > 0 ? 0.0f : (1.f - (time_left / slider_time_actual));

    if (reverse)
        completion_end_actual = std::clamp(1.f - reverse_completion, 0.f, 1.f);

    if (rev_render_complete != nullptr && reverse)
        *rev_render_complete = completion_end_actual == 0.f;

    uint32_t slider_left = std::clamp(static_cast<uint32_t>(time_left / slider_time_actual), (uint32_t)0, object.repeat - 1);

    if (object.slidertype == L"B")
    {
        DrawSliderPartBiezer(object, length_left, color, hardrock, completion_end_actual, object.repeat % 2 == 0 || reverse);
    }
    else if (object.slidertype == L"L")
    {
        slidercurve first_curve = init_curve;

        for (int i = 0; i < object.slidercurves.size(); i++)
        {
            DrawSliderLinear(first_curve, object.slidercurves.at(i), length_left, color, hardrock, completion_end_actual, object.repeat % 2 == 0 || reverse);
            first_curve = object.slidercurves.at(i);
        }
    }
    else if (object.slidertype == L"P")
    {
        DrawSliderPerfectCircle(object, color, hardrock, completion_end_actual, object.repeat % 2 == 0 || reverse);
    }
    // is this even correct? not that it matters.. (C-type isn't used at all)
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
                    ), hardrock
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
    }

    // output string
#ifdef _DEBUG
    m_font->DrawString(m_spriteBatch.get(), (object.slidertype + L" " +
        std::to_wstring(completion_end_actual) + L" " + 
        (reverse ? L"r:True" : L"r:False") + L" " + (rev_render_complete != nullptr && *rev_render_complete ? L"c:True" : L"c:False") + L" " +
        std::to_wstring(slider_left) + L" left: " + std::to_wstring(length_left)
        ).c_str(),
        GetScreenCoordFromOsuPixelStandard(&object),
        color, 0.f, DirectX::SimpleMath::Vector2(0, 0), 0.3);
#else
    if (slider_left > 0)
        m_font->DrawString(m_spriteBatch.get(), (std::to_wstring(slider_left) + (slider_left % 2 == 1 ? L" inv" : L" str")).c_str(),
            GetScreenCoordFromOsuPixelStandard(object.slidercurves_calculated.back(), hardrock),
            color, 0.f, DirectX::SimpleMath::Vector2(0, 0), 0.5);
#endif

    return;
}

void Overlay::DrawSliderPerfectCircle(hitobject& object, DirectX::XMVECTORF32 color, bool hardrock, float_t completion, bool reverse)
{
    std::vector<DirectX::VertexPositionColor> output;

    for (slidercurve &curve : object.slidercurves_calculated)
    {
        output.push_back(DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(curve, hardrock),
            color
            ));
    }

    int max = (output.size() * (completion));

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
}

void Overlay::DrawSliderPerfectCircle_Deprecated(slidercurve init_point, std::vector<slidercurve> &curves, double &dist_left, DirectX::XMVECTORF32 color, bool hardrock, float_t completion, bool reverse)
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
    DirectX::SimpleMath::Vector2 last_point = curves.back();

    for (int i = 0; i < amountPoints; ++i)
    {
        double fract = (double)i / (amountPoints - 1);
        double theta = thetaStart + dir * fract * thetaRange;
        DirectX::SimpleMath::Vector2 o = DirectX::SimpleMath::Vector2((float)std::cos(theta), (float)std::sin(theta)) * r;

        // calculate total arch length, if overshoot. stop.
        if (std::abs((dir * fract * thetaRange) * r) >= dist_left)
        {
            dist_left -= std::abs((dir * fract * thetaRange) * r);
            break;
        }

        last_point = centre + o;
            
        output.push_back(DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(centre + o, hardrock),
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
}

/**

*/
inline void Overlay::DrawSliderLinear(slidercurve init_point, slidercurve &curve, double &dist_left, DirectX::XMVECTORF32 color, bool hardrock, float_t inv_completion, bool reverse)
{
    if (dist_left < 0.0)
        return;

    DirectX::SimpleMath::Vector2 init_pt = static_cast<DirectX::SimpleMath::Vector2>(init_point),
        curve_pt = static_cast<DirectX::SimpleMath::Vector2>(curve);

    DirectX::SimpleMath::Vector2 vec_final = Utilities::ClampVector2Magnitude(init_pt, curve_pt, dist_left);

    m_batch->DrawLine(
        DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(init_pt + ((vec_final - init_pt) * (reverse ? 0.f : inv_completion)), hardrock), color
        ),
        DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(vec_final + ((init_pt - vec_final) * (reverse ? inv_completion : 0.f)), hardrock), color
        )
    );

    dist_left -= DirectX::SimpleMath::Vector2::Distance(init_pt, vec_final);
}

void Overlay::DrawSliderPartBiezer(hitobject& object, double& dist_left, DirectX::XMVECTORF32 color, bool hardrock, float_t completion, bool reverse )
{

    std::vector<DirectX::VertexPositionColor> output;

    for (slidercurve& curve : object.slidercurves_calculated)
    {
        output.push_back(DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(curve, hardrock),
            color
            ));
    }

    int max = (output.size() * (completion));


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

#ifdef _DEBUG
        m_font->DrawString(m_spriteBatch.get(), (std::to_wstring(dist_left) + L" " + std::to_wstring(output.size())).c_str(),
            DirectX::SimpleMath::Vector2(output.back().position.x, output.back().position.y),
            Colors::Yellow, 0.f, DirectX::SimpleMath::Vector2(0, 0), 0.3f);
#endif // _DEBUG
    }
}

void Overlay::DrawSliderPartBiezer_Deprecated(slidercurve init_point, std::vector<slidercurve> &curves, double &dist_left, DirectX::XMVECTORF32 color, bool hardrock, float_t completion, bool reverse)
{
    int size = curves.size();
    std::vector<int> x_p, y_p;
    double x, y;

    //  segment size (pixels) btw
    double segment_length = 20;
    std::vector<DirectX::VertexPositionColor> lines;
    std::vector<double> arcLengths;
    x_p.push_back(init_point.x);
    y_p.push_back(init_point.y);
    DirectX::SimpleMath::Vector2 prev_point;

    for (int i = 0; i < curves.size(); i++)
    {
        x_p.push_back(curves.at(i).x);
        y_p.push_back(curves.at(i).y);

        if (i + 1 < curves.size() && curves.at(i) == curves.at(i + 1))
        {
            prev_point = DirectX::SimpleMath::Vector2(x_p.front(), y_p.front());

            //  https://gamedev.stackexchange.com/questions/5373/moving-ships-between-two-planets-along-a-bezier-missing-some-equations-for-acce/5427#5427
            for (double t = 0; t <= 1; t += 0.01)
            {
                x = Utilities::getBezierPoint(&x_p, t);
                y = Utilities::getBezierPoint(&y_p, t);

                if (DirectX::SimpleMath::Vector2::Distance(prev_point, DirectX::SimpleMath::Vector2(x, y)) < 10 && t != 0)
                {
                    continue;
                }

                dist_left -= DirectX::SimpleMath::Vector2::Distance(prev_point, DirectX::SimpleMath::Vector2(x, y));

                if (dist_left < 0)
                    break;

                prev_point = DirectX::SimpleMath::Vector2(x, y);

                lines.push_back(DirectX::VertexPositionColor(
                    GetScreenCoordFromOsuPixelStandard(x, y, hardrock),
                    color
                ));
                
            }

            x_p.clear();
            y_p.clear();
        }
    }

    prev_point = DirectX::SimpleMath::Vector2(x_p.front(), y_p.front());

    for (double t = 0; t <= 1; t += 0.01)
    {
        x = Utilities::getBezierPoint(&x_p, t);
        y = Utilities::getBezierPoint(&y_p, t);

        // you see, we need to make the segment distances the same. i chose 10.
        // but i really don't know math and this seems to get close enough so this is a temporary measure.
        if (DirectX::SimpleMath::Vector2::Distance(prev_point, DirectX::SimpleMath::Vector2(x, y)) < 10 && t != 0)
        {
            continue;
        }

        dist_left -= DirectX::SimpleMath::Vector2::Distance(prev_point, DirectX::SimpleMath::Vector2(x, y));

        if (dist_left < 0)
            break;

        prev_point = DirectX::SimpleMath::Vector2(x, y);

        lines.push_back(DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(x, y, hardrock),
            color
        ));
    }

    // line connection
    if (lines.back().position.x != Utilities::getBezierPoint(&x_p, 1.0) && lines.back().position.y != Utilities::getBezierPoint(&y_p, 1.0))
    {
        lines.push_back(DirectX::VertexPositionColor(
            GetScreenCoordFromOsuPixelStandard(Utilities::getBezierPoint(&x_p, 1.0), Utilities::getBezierPoint(&y_p, 1.0), hardrock),
            color
        ));
    }

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
        m_font->DrawString(m_spriteBatch.get(), (std::to_wstring(dist_left) + L" " + std::to_wstring(lines.size())).c_str(),
            DirectX::SimpleMath::Vector2(lines.back().position.x, lines.back().position.y),
            Colors::Yellow, 0.f, DirectX::SimpleMath::Vector2(0, 0), 0.3f);
#endif // _DEBUG
    }
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
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::Brown);
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

    CD3D11_RASTERIZER_DESC rastDesc(D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE,
        D3D11_DEFAULT_DEPTH_BIAS, D3D11_DEFAULT_DEPTH_BIAS_CLAMP,
        D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, TRUE, FALSE, FALSE, TRUE);

    DX::ThrowIfFailed(m_d3dDevice->CreateRasterizerState(&rastDesc,
        m_raster.ReleaseAndGetAddressOf()));

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

    m_raster.Reset();

    CreateDevice();

    CreateResources();
}