#pragma once

#include "pch.h"
#include "Beatmap.h"
#include "StepTimer.h"
#include "OsuOverlay.h"
#include "OsuGame.h"

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
