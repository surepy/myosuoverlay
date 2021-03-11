#pragma once

#include "StepTimer.h"
#include "Beatmap.h"

class osuGame;
class Utilities;

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
// from: https://github.com/walbourn/directx-vs-templates/tree/master/d3d11game_win32
class Overlay
{
public:

    Overlay() noexcept;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick(osuGame &gameStat);

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize(int& width, int& height) const;

    void SetName(std::wstring name);

private:
    void Update(DX::StepTimer const& timer);
    void Render(osuGame &gameStat);

    // draw our stat texts.
    void RenderStatTexts(osuGame &gameStat);

    // re-draw game (mania)
    void RenderStatTextsStandard(osuGame& gameStat);

    // re-draw game (mania)
    void RenderStatTextsMania(osuGame& gameStat);
    
    // re-draw game
    void RenderAssistant(osuGame& gameStat);

    // re-draw game (mania)
    void RenderAssistantStandardHidden(osuGame& gameStat);
    void RenderAssistantStandard(osuGame& gameStat);

    // re-draw game (mania)
    void RenderAssistantMania(osuGame& gameStat);

    // draw a text with a square behind.
    DirectX::XMVECTOR RenderStatSquare(std::wstring &text, DirectX::SimpleMath::Vector2 &origin, DirectX::SimpleMath::Vector2 &fontPos, DirectX::XMVECTORF32 tColor = DirectX::Colors::White, DirectX::XMVECTORF32 bgColor = DirectX::Colors::Black, int v = 1, float fontsize = 0.5f);
    
    // draw text.
    DirectX::SimpleMath::Vector2 DrawText(std::wstring text, float size, DirectX::XMVECTORF32 color, DirectX::SimpleMath::Vector2* pos_force = nullptr, DirectX::SimpleMath::Vector2* offset = nullptr);

    DirectX::SimpleMath::Vector2 DrawText(std::wstring text, float size, DirectX::XMVECTORF32 color, bool direction, DirectX::SimpleMath::Vector2* pos_force = nullptr, DirectX::SimpleMath::Vector2* offset = nullptr);

    // debug func: slider points.
    void DebugDrawSliderPoints(int x, int y, std::vector<slidercurve>& points, DirectX::XMVECTORF32 color = DirectX::Colors::White, bool hardrock = false);

    static constexpr float playfield_size[] = { 512.0f, 384.0f };

    float x_multiplier, y_multiplier;
    float x_offset, y_offset;

    // blatantly pasted from https://github.com/CookieHoodie/OsuBot/blob/master/OsuBots/OsuBot.cpp#L133
    // calculates screen cooarniate offsets
    void CalculateScreenCoordOffsets()
    {
        float screenWidth = m_outputWidth;
        float screenHeight = m_outputHeight;

        // ????
        if (screenWidth * 3 > screenHeight * 4) {
            screenWidth = screenHeight * 4 / 3;
        }
        else {
            screenHeight = screenWidth * 3 / 4;
        }

        x_multiplier = screenWidth / 640.f;
        y_multiplier = screenHeight / 480.0f;

        x_offset = 1.f + static_cast<int>(m_outputWidth - playfield_size[0] * x_multiplier) / 2;
        y_offset = 8.0f * y_multiplier + static_cast<int>(m_outputHeight - playfield_size[1] * y_multiplier) / 2;
    }

    float x_multiplier_cursor, y_multiplier_cursor;
    float x_offset_cursor, y_offset_cursor;

    // pasted too, there's really not much i can do to uh not do this.
    void CalculateScreenCoordOffsetsCursor()
    {
        // GetSystemMetrics(SM_CXSCREEN) and GetSystemMetrics(SM_CYSCREEN) is freaking out, hardcoded 1920x1080 for now.
        // subtract 1 becasue getcursorpos can't get to 1920, stops at 1919
        float screenWidth = 1920 - 1;
        float screenHeight = 1080 - 1;

        // ????
        if (screenWidth * 3 > screenHeight * 4) {
            screenWidth = screenHeight * 4 / 3;
        }
        else {
            screenHeight = screenWidth * 3 / 4;
        }

        x_multiplier_cursor = screenWidth / 640.f;
        y_multiplier_cursor = screenHeight / 480.0f;

        x_offset_cursor = 1.f + static_cast<int>((1920 - 1) - playfield_size[0] * x_multiplier_cursor) / 2;
        y_offset_cursor = 8.0f * y_multiplier_cursor + static_cast<int>((1080 - 1) - playfield_size[1] * y_multiplier_cursor) / 2;
    }

    /**
        Gets Actual Screen coords from osupixel
    */
    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(int32_t x, int32_t y, bool hardrock = false)
    {
        if (hardrock)
        {
            y = 384 - y;
        }

        return DirectX::SimpleMath::Vector2(
            x_offset + (float)x * x_multiplier,
            y_offset + (float)y * y_multiplier
        );
    }

    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(DirectX::SimpleMath::Vector2 vec, bool hardrock = false)
    {
        int y = vec.y; 

        if (hardrock)
        {
            y = 384 - y;
        }

        return DirectX::SimpleMath::Vector2(
            x_offset + vec.x * x_multiplier,
            y_offset + y * y_multiplier
        );
    }

    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(double_t x, double_t y, bool hardrock = false)
    {
        if (hardrock)
        {
            y = 384 - y;
        }

        return DirectX::SimpleMath::Vector2(
            x_offset + (float)x * x_multiplier,
            y_offset + (float)y * y_multiplier
        );
    }

    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(hitobject *obj, bool hardrock = false)
    {
        return GetScreenCoordFromOsuPixelStandard(obj->x, obj->y, hardrock);
    } 

    // Gets Actual Screen coords from osupixel, this one is used for reading lines completion%
    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(slidercurve const& first, slidercurve const& second, double_t inv_completion, bool hardrock = false);

    // Gets Actual Screen coords from osupixel, this one is used for reading lines completion%
    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(int32_t x1, int32_t y1, int32_t x2, int32_t y2, double_t inv_completion, bool hardrock = false)
    {
        int y = (y1 + ((y2 - y1) * static_cast<float_t>(std::clamp(inv_completion, 0.0, 1.0))));

        if (hardrock)
        {
            y = 384 - y;
        }

        return DirectX::SimpleMath::Vector2(
            x_offset + (x1 + ((x2 - x1) * static_cast<float_t>(std::clamp(inv_completion, 0.0, 1.0)))) * x_multiplier,
            y_offset + y * y_multiplier
        );
    }

    // Gets Actual Screen coords from osupixel, this one is used for reading lines completion%
    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(int32_t x1, int32_t y1, int32_t x2, int32_t y2, float_t inv_completion, bool hardrock = false)
    {
        int y = (y1 + ((y2 - y1) * std::clamp(inv_completion, 0.f, 1.f)));

        if (hardrock)
        {
            y = 384 - y;
        }

        return DirectX::SimpleMath::Vector2(
            x_offset + (x1 + ((x2 - x1) * std::clamp(inv_completion, 0.f, 1.f))) * x_multiplier,
            y_offset + y * y_multiplier
        );
    }

    // Draw a slider.
    void DrawSlider(hitobject& object, int32_t& time, DirectX::XMVECTORF32 color, bool hardrock = false, bool reverse = false, float_t reverse_completion = 0.f, bool* render_complete = nullptr);

    inline void DrawSliderLinear(slidercurve init_point, slidercurve &curves, double &dist_left, DirectX::XMVECTORF32 color, bool hardrock = false, float_t completion = 0.f, bool reverse = false);

    void DrawSliderPartBiezer_Deprecated(slidercurve init_point, std::vector<slidercurve> &curves, double &dist_left, DirectX::XMVECTORF32 color, bool hardrock = false, float_t completion = 0.f, bool reverse = false);

    void DrawSliderPartBiezer(hitobject& object, double& dist_left, DirectX::XMVECTORF32 color, bool hardrock = false, float_t completion = 0.f, bool reverse = false);

    void DrawSliderPerfectCircle_Deprecated(slidercurve init_point, std::vector<slidercurve> &curves, double &dist_left, DirectX::XMVECTORF32 color, bool hardrock = false, float_t completion = 0.f, bool reverse = false);

    void DrawSliderPerfectCircle(hitobject& object, DirectX::XMVECTORF32 color, bool hardrock = false, float_t completion = 0.f, bool reverse = false);

    /*
    void DrawCircleIWantToKillMyself(int32_t radius = 200)
    {
        float x, y, i;

        //        DirectX::VertexPositionColor verticies[360];

        // iterate y up to 2*pi, i.e., 360 degree
        // with small increment in angle as
        // glVertex2i just draws a point on specified co-ordinate
        for (i = 0; i < (2 * (atan(1) * 4)); i += 0.01)
        {
            // let 200 is radius of circle and as,
            // circle is defined as x=r*cos(i) and y=r*sin(i)
            x = (radius * cos(i)) + (m_outputWidth / 2);
            y = (radius * sin(i)) + (m_outputHeight / 2);

            /*verticies[b] = DirectX::VertexPositionColor(
                DirectX::SimpleMath::Vector2(x, y),
                DirectX::Colors::White
            );*/
    /*
            m_batch->Draw(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
                &DirectX::VertexPositionColor(
                    DirectX::SimpleMath::Vector2(x, y),
                    DirectX::Colors::White
                ),
                1
            );
        }*/

        /*m_batch->Draw(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
            verticies,
            360
        );*/
    /*}*/

    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void OnDeviceLost();

    // Device resources.
    HWND                                            m_window;
    int                                             m_outputWidth;
    int                                             m_outputHeight;

    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext;

    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

    // Rendering loop timer.
    DX::StepTimer                                   m_timer;

    //  font related
    std::unique_ptr<DirectX::SpriteFont> m_font;
    DirectX::SimpleMath::Vector2 m_fontPos;
    std::unique_ptr<DirectX::SpriteBatch> m_spriteBatch;

    //  drawing
    std::unique_ptr<DirectX::CommonStates> m_states;
    std::unique_ptr<DirectX::BasicEffect> m_effect;
    std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_batch;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;

    std::wstring user;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_raster;
};
