#pragma once

#include "pch.h"
#include "Beatmap.h"
#include "StepTimer.h"

struct MappingFile
{
public:
    HANDLE handle;
    LPVOID* data;
    std::string name;

    MappingFile(std::string map_name)
    {
        name = map_name;
        handle = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 8 * 1024, map_name.c_str()); // todo readonly
        data = reinterpret_cast<PVOID*>(MapViewOfFile(handle, FILE_MAP_ALL_ACCESS, 0, 0, 8 * 1024));
    }

    ~MappingFile()
    {
        UnmapViewOfFile(data);
        CloseHandle(handle);
    }

    TCHAR* ReadToEnd()
    {
        if (handle == NULL)
            return 0;

        return reinterpret_cast<TCHAR*>(data);
    }
};

enum gameMode {
    STANDARD,
    MANIA,
    TAIKO,
    CATCH,
    UNKNOWN
};

struct gameplayStats {
    std::unique_ptr<MappingFile> mfCursor;
    std::unique_ptr<MappingFile> mfKey;
    std::unique_ptr<MappingFile> mfOsuPlayHits;
    std::unique_ptr<MappingFile> mfOsuPlayPP;
    std::unique_ptr<MappingFile> mfOsuPlayHP;
    std::unique_ptr<MappingFile> mfKeyStat;
    std::unique_ptr<MappingFile> mfOsuMapTime;
    std::unique_ptr<MappingFile> mfOsuFileLoc;
    std::unique_ptr<MappingFile> mfOsuKiaiStat;
    std::unique_ptr<MappingFile> mfCurrentModsStr;
    std::unique_ptr<MappingFile> mfCurrentOsuGMode;

    std::chrono::milliseconds currentTime;
    std::chrono::milliseconds previousDistTime;

    beatmap currentMap;
    std::vector<std::chrono::milliseconds> clicks;

    int osuMapTime;
    gameMode gameMode = gameMode::STANDARD;
    bool osuMapTimeLoaded = false;  //  idk if i will use

    DirectX::SimpleMath::Vector2 cursorLocation;
    double cursorSpeed = 0;

    bool clickx = false, clicky = false;
    int clickCounter = 0;
    int beatIndex = 0;
    bool streamCompanionRunning = true;
};

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
    void Tick(gameplayStats &gameStat);

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

    bool loadMap(gameplayStats &gameStat);

    void Update(DX::StepTimer const& timer);
    void Render(gameplayStats &gameStat);

    DirectX::XMVECTOR RenderStatSquare(std::wstring &text, DirectX::SimpleMath::Vector2 &origin, DirectX::SimpleMath::Vector2 &fontPos, DirectX::XMVECTORF32 tColor = DirectX::Colors::White, DirectX::XMVECTORF32 bgColor = DirectX::Colors::Black, int v = 1, float fontsize = 0.5f);
    void RenderAssistant(gameplayStats &gameStat);

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
