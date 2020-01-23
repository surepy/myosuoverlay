#pragma once

#include "pch.h"
#include "Beatmap.h"
#include "StepTimer.h"
#include "OsuOverlay.h"
#include "OsuGame.h"
#include <Math.h>

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

    bool loadMap(osuGame &gameStat);

    void Update(DX::StepTimer const& timer);
    void Render(osuGame &gameStat);

    void RenderStatTexts(osuGame &gameStat);
    DirectX::XMVECTOR RenderStatSquare(std::wstring &text, DirectX::SimpleMath::Vector2 &origin, DirectX::SimpleMath::Vector2 &fontPos, DirectX::XMVECTORF32 tColor = DirectX::Colors::White, DirectX::XMVECTORF32 bgColor = DirectX::Colors::Black, int v = 1, float fontsize = 0.5f);
    void RenderAssistant(osuGame &gameStat);

    void DebugDrawSliderPoints(int x, int y, std::vector<slidercurve> &points, DirectX::XMVECTORF32 color = DirectX::Colors::White);

    /**
        Gets Actual Screen coords from osupixel
    */
    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(int32_t &x, int32_t &y)
    {
        return DirectX::SimpleMath::Vector2(
            257.f + x * 1.5f,
            84.f + y * 1.5f
        );
    }

    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(double_t &x, double_t &y)
    {
        return DirectX::SimpleMath::Vector2(
            257.f + (float)x * 1.5f,
            84.f + (float)y * 1.5f
        );
    }

    inline DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(hitobject *obj)
    {
        return GetScreenCoordFromOsuPixelStandard(obj->x, obj->y);
    }

    /**
        Gets Actual Screen coords from osupixel, this one is used for reading lines completion%
    */
    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(int32_t &x1, int32_t &y1, int32_t &x2, int32_t &y2, double_t *inv_completion)
    {
        return DirectX::SimpleMath::Vector2(
            257.f + (x1 + ((x2 - x1) * static_cast<float_t>(std::clamp(*inv_completion, 0.0, 1.0)))) * 1.5f,
            84.f + (y1 + ((y2 - y1) * static_cast<float_t>(std::clamp(*inv_completion, 0.0, 1.0)))) * 1.5f
        );
    }

    /**
        Gets Actual Screen coords from osupixel, this one is used for reading lines completion%
    */
    DirectX::SimpleMath::Vector2 GetScreenCoordFromOsuPixelStandard(int32_t &x1, int32_t &y1, int32_t &x2, int32_t &y2, float_t *inv_completion)
    {
        return DirectX::SimpleMath::Vector2(
            257.f + (x1 + ((x2 - x1) * std::clamp(*inv_completion, 0.f, 1.f))) * 1.5f,
            84.f + (y1 + ((y2 - y1) * std::clamp(*inv_completion, 0.f, 1.f))) * 1.5f
        );
    }

    DirectX::SimpleMath::Vector2 DrawSlider(hitobject &object, int32_t &time, DirectX::XMVECTORF32 color);

    inline void DrawSliderLinear(slidercurve& init_point, std::vector<slidercurve> &curves, double &dist_left, DirectX::XMVECTORF32 color, float_t inv_completion = 0.f, DirectX::SimpleMath::Vector2 *vec = nullptr);

    void DrawSliderPartBiezer(slidercurve& init_point, std::vector<slidercurve> &curves, double &dist_left, DirectX::XMVECTORF32 color, float_t inv_completion = 0.f, DirectX::SimpleMath::Vector2 *vec = nullptr);

    //  literally if there's more control points than this the mapper is fucking stupid

    void DrawCircleIWantToKillMyself(int32_t radius = 200)
    {
        float x, y, i;

        //        DirectX::VertexPositionColor verticies[360];

                // iterate y up to 2*pi, i.e., 360 degree
                // with small increment in angle as
                // glVertex2i just draws a point on specified co-ordinate
        for (i = 0; i < (2 * (atan(1) * 4)); i += 0.001)
        {
            // let 200 is radius of circle and as,
            // circle is defined as x=r*cos(i) and y=r*sin(i)
            x = (radius * cos(i)) + (m_outputWidth / 2);
            y = (radius * sin(i)) + (m_outputHeight / 2);

            /*verticies[b] = DirectX::VertexPositionColor(
                DirectX::SimpleMath::Vector2(x, y),
                DirectX::Colors::White
            );*/

            m_batch->Draw(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
                &DirectX::VertexPositionColor(
                    DirectX::SimpleMath::Vector2(x, y),
                    DirectX::Colors::White
                ),
                1
            );
        }

        /*m_batch->Draw(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
            verticies,
            360
        );*/
    }

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
};
