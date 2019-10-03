//
// Game.cpp
//

#include "pch.h"
#include "OsuOverlay.h"

extern void ExitGame();

using namespace DirectX;

using Microsoft::WRL::ComPtr;

template <typename T>
// helper function.. todo move to main.cpp
std::wstring to_wstring_with_precision(const T a_value, const int n = 6)
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

bool Overlay::loadMap(gameplayStats &gameStat)
{
    std::wstring osuMap = static_cast<std::wstring>(gameStat.mfOsuFileLoc->ReadToEnd());
    if (gameStat.currentMap.loadedMap.compare(osuMap) != 0)
    {
        gameStat.currentMap.timingpoints.clear();
        gameStat.currentMap.hitobjects.clear();

        gameStat.currentMap.currentTimeIndex = 0;
        gameStat.currentMap.currentObjectIndex = 0;
        try
        {
            if (!gameStat.currentMap.Parse(osuMap))
            {
                return false;
            }
            gameStat.currentMap.loadedMap = osuMap;
        }
        catch (std::out_of_range)
        {
            gameStat.currentMap.currentTimeIndex = 0;
            gameStat.currentMap.currentObjectIndex = 0;
            return false;
        }
    }
    return true;
}

// Executes the basic game loop.
void Overlay::Tick(gameplayStats &gameStat)
{
    bool mapLoaded = loadMap(gameStat);
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
        if ((gameStat.previousDistTime + std::chrono::milliseconds(10)) < gameStat.currentTime)
        {
            double dist = std::sqrt(std::pow((cursorLocationCurrent.x - gameStat.cursorLocation.x), 2) + std::pow((cursorLocationCurrent.y - gameStat.cursorLocation.y), 2));
            gameStat.cursorSpeed = dist * 10; // 1 second displacement

            gameStat.cursorLocation = DirectX::SimpleMath::Vector2(static_cast<int>(cursorLocationCurrent.x), static_cast<int>(cursorLocationCurrent.y));

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

    if (mapLoaded)
    {
        try {
            //  the map is restarted. prob a better way to do this exist but /shrug
            if (gameStat.osuMapTime > std::stod(gameStat.mfOsuMapTime->ReadToEnd()) * 1000)
            {
                gameStat.currentMap.currentTimeIndex = 0;
                gameStat.currentMap.currentObjectIndex = 0;
            }

            gameStat.osuMapTime = static_cast<int>(std::stod(gameStat.mfOsuMapTime->ReadToEnd()) * 1000);

            for (uint32_t i = gameStat.currentMap.currentTimeIndex; i < gameStat.currentMap.timingpoints.size() && gameStat.osuMapTime >= gameStat.currentMap.timingpoints[i].offset; i++)
            {
                gameStat.currentMap.currentBpm = static_cast<int>(60000 / gameStat.currentMap.timingpoints[i].ms_per_beat);
                gameStat.currentMap.currentSpeed = gameStat.currentMap.timingpoints[i].velocity;
                gameStat.currentMap.kiai = gameStat.currentMap.timingpoints[i].kiai;
                gameStat.currentMap.currentTimeIndex = i;
            }

            for (uint32_t i = gameStat.currentMap.currentObjectIndex; i < gameStat.currentMap.hitobjects.size() && gameStat.osuMapTime >= gameStat.currentMap.hitobjects[i].start_time; i++)
            {
                gameStat.currentMap.currentObjectIndex = i;
            }

            gameStat.osuMapTimeLoaded = true;
        }
        catch (std::invalid_argument)
        {
            gameStat.currentMap.currentTimeIndex = 0;
            gameStat.currentMap.currentObjectIndex = 0;
            gameStat.osuMapTimeLoaded = false;
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
void Overlay::Render(gameplayStats &gameStat)
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

    std::wstring textString;
    XMVECTOR result;

    // set font origin
    DirectX::SimpleMath::Vector2 origin = DirectX::SimpleMath::Vector2(0.f, 0.f);
    //m_font->MeasureString(textString.c_str()) / 2.f;
    //middle

    /*
     * tap bpm + map bpm + velocity string
     */
    textString = std::to_wstring(gameStat.clicks.size() * 60 / 4) + std::wstring(L"/") + std::to_wstring(gameStat.currentMap.currentBpm) + std::wstring(L" (x")
        + to_wstring_with_precision(gameStat.currentMap.currentSpeed, 2) + std::wstring(L") bpm");

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos = DirectX::SimpleMath::Vector2(0.f + 370.f, m_outputHeight - 21.f);

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::Red, Colors::Black, 2);

    /*
     * Kps + (totalclicks) string
     */
    origin = DirectX::SimpleMath::Vector2(0.f, 0.f);

    textString = std::wstring(L"kps: ") + std::to_wstring(gameStat.clicks.size()) + std::wstring(L" (") + std::to_wstring(gameStat.clickCounter) + std::wstring(L")");

    m_fontPos.x += 30;

    result = RenderStatSquare(textString, origin, m_fontPos);

    /*
     *  skip map diff + map name for now.
     */

    textString = std::wstring(gameStat.mfOsuPlayHits->ReadToEnd()).size() == 3 ? L"" : static_cast<std::wstring>(gameStat.mfOsuPlayHits->ReadToEnd());

    origin.x = XMVectorGetX(m_font->MeasureString(textString.c_str()));

    m_fontPos.x = static_cast<float>(m_outputWidth);    // all the way to right
    m_fontPos.y -= 46;  //  skip 2 boxes from bottom.

    result = RenderStatSquare(textString, origin, m_fontPos, Colors::LightGreen, Colors::Black, 2);

    /*
     *  Cursor stuff (stat)
     */

    textString = std::wstring(L"x: ") + std::to_wstring(static_cast<int>(gameStat.cursorLocation.x)) + std::wstring(L" y: ") + std::to_wstring(static_cast<int>(gameStat.cursorLocation.y)) + std::wstring(L" vel: ") + std::to_wstring((int)round(gameStat.cursorSpeed)) + std::wstring(L" p/s");

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

    /*
     *  hidden assistant - time till next hitobject
     */
     /*
     m_font->DrawString(m_spriteBatch.get(), std::to_string(gameStat.osuMapTime).c_str(),
         DirectX::SimpleMath::Vector2(0.f, 0.f),
         Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 1.f);*/

    if (std::wstring(gameStat.mfCurrentOsuGMode->ReadToEnd()).compare(L"Osu") == 0 && gameStat.osuMapTimeLoaded && std::wstring(gameStat.mfCurrentModsStr->ReadToEnd()).find(L"HD") != std::string::npos
        && gameStat.currentMap.currentObjectIndex < gameStat.currentMap.hitobjects.size() - 1 && gameStat.osuMapTimeLoaded // temp
        )
    {
        /*
        textString = std::to_wstring(gameStat.currentMap.hitobjects[gameStat.currentMap.currentObjectIndex + 1].start_time - gameStat.osuMapTime); //  time till next note.

        m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
            DirectX::SimpleMath::Vector2(
                252 + gameStat.currentMap.hitobjects[gameStat.currentMap.currentObjectIndex + 1].x * 1.5, // padding + pixel
                64 + 16 + gameStat.currentMap.hitobjects[gameStat.currentMap.currentObjectIndex + 1].y * 1.5
            ),
            Colors::White, 0.f, m_font->MeasureString(textString.c_str()) / 2.f, 0.3f);*/

        for (std::uint32_t i = gameStat.currentMap.currentObjectIndex + 1; i < gameStat.currentMap.hitobjects.size() && 600 >= gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime; i++)
        {
            textString = std::to_wstring(i - gameStat.currentMap.currentObjectIndex);
            //std::to_wstring(gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime);

            if (i == gameStat.currentMap.currentObjectIndex + 1)
            {
                textString = L"> " + textString + L" <";

                // draw the text- circle
                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    DirectX::SimpleMath::Vector2(
                        256 + gameStat.currentMap.hitobjects[i].x * 1.5f, // padding + pixel
                        64 + 16 + gameStat.currentMap.hitobjects[i].y * 1.5f
                    ),
                    Colors::Yellow, 0.f, m_font->MeasureString(textString.c_str()) * 0.6f, 0.6f);

                m_font->DrawString(m_spriteBatch.get(), std::wstring(L" - " + std::to_wstring(gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime)).c_str(),
                    DirectX::SimpleMath::Vector2(
                        256 + gameStat.currentMap.hitobjects[i].x * 1.5f + (XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.3f),
                        64 + 16 + gameStat.currentMap.hitobjects[i].y * 1.5f
                    ),
                    Colors::Yellow, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.3f);
            }
            else
            {
                m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                    DirectX::SimpleMath::Vector2(
                        256 + gameStat.currentMap.hitobjects[i].x * 1.5f, // padding + pixel
                        64 + 16 + gameStat.currentMap.hitobjects[i].y * 1.5f
                    ),
                    Colors::White, 0.f, m_font->MeasureString(textString.c_str()) * 0.5f, 0.5f);

                m_font->DrawString(m_spriteBatch.get(), std::wstring(L" - " + std::to_wstring(gameStat.currentMap.hitobjects[i].start_time - gameStat.osuMapTime)).c_str(),
                    DirectX::SimpleMath::Vector2(
                        256 + gameStat.currentMap.hitobjects[i].x * 1.5 + (XMVectorGetX(m_font->MeasureString(textString.c_str())) * 0.3f),
                        64 + 16 + gameStat.currentMap.hitobjects[i].y * 1.5f
                    ),
                    Colors::White, 0.f, DirectX::SimpleMath::Vector2(0.f, 0.f), 0.3f);
            }

            /*m_font->DrawString(m_spriteBatch.get(), textString.c_str(),
                DirectX::SimpleMath::Vector2(
                    252 + gameStat.currentMap.hitobjects[i].x * 1.5, // padding + pixel
                    64 + 16 + gameStat.currentMap.hitobjects[i].y * 1.5
                ),
                Colors::White, 0.f, m_font->MeasureString(textString.c_str()) / 2.f, 0.3f);*/
        }
    }

    m_batch->End();
    m_spriteBatch->End();

    Present();
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