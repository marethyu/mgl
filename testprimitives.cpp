/* g++ testprimitives.cpp -o testprimitives -lSDL2 */

#include <Windows.h>

#include <SDL2/SDL.h>

#include "mygl.h"

#define ID_TIMER 1

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
const int SCREEN_SCALE_FACTOR = 1;

class TestPrimitives : public RendererBase3D
{
public:
    TestPrimitives(int width, int height);
    ~TestPrimitives();

    void Create(HWND hwnd, int updateInterval);
    void Show();
    void CleanUp();
    void Destroy();

    void Init();
    void Update();
    void Render();
private:
    HWND hwnd;

    SDL_Window *wnd;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    float radius;
    float angle, da;
};

TestPrimitives::TestPrimitives(int width, int height)
  : RendererBase3D(width, height)
{}

TestPrimitives::~TestPrimitives()
{}

void TestPrimitives::Create(HWND hWnd, int updateInterval)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s", SDL_GetError());
        std::exit(1);
    }

    wnd = SDL_CreateWindowFrom(hWnd);
    if (wnd == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create window: %s", SDL_GetError());
        std::exit(1);
    }

    renderer = SDL_CreateRenderer(wnd, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create renderer: %s", SDL_GetError());
        std::exit(1);
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (texture == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't create texture: %s", SDL_GetError());
        std::exit(1);
    }

    if(!SetTimer(hWnd, ID_TIMER, updateInterval, NULL))
    {
        MessageBox(hWnd, "Could not set timer!", "errYor", MB_OK | MB_ICONEXCLAMATION);
        PostQuitMessage(1);
    }

    hwnd = hWnd;

    Init();
}

void TestPrimitives::Show()
{
    ClearScreen();
    Render();

    SDL_UpdateTexture(texture, NULL, &pixels[0], width * 4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void TestPrimitives::CleanUp()
{
    SDL_DestroyTexture(texture);
    texture = NULL;

    SDL_DestroyRenderer(renderer);
    renderer = NULL;

    SDL_DestroyWindow(wnd);
    wnd = NULL;

    SDL_Quit();
    KillTimer(hwnd, ID_TIMER);
}

void TestPrimitives::Destroy()
{
    PostQuitMessage(0);
}

void TestPrimitives::Init()
{
    radius = 180.0;
    angle = 0.0;
    da = 0.01;
}

void TestPrimitives::Update()
{
    angle += da;
}

void TestPrimitives::Render()
{
    DrawFilledTriangle(10, 50, 10, 400, 100, 10, 290, 380, 10, RED);
    DrawFilledTriangle(50, 350, 2, 130, 40, 20, 380, 200, 5, GREEN);
    DrawWireFrameTriangle(250, 250, 0, 70, 400, 0, 320, 400, 0, BLUE);

    float centerX = width / 2.0;
    float centerY = height / 2.0;
    float x = centerX + radius * std::cos(angle);
    float y = centerY + radius * std::sin(angle);

    DrawLine(centerX, centerY, 9, x, y, 9, INDIGO);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static TestPrimitives app(SCREEN_WIDTH, SCREEN_HEIGHT);

    switch (msg)
    {
    case WM_CREATE:
    {
        app.Create(hWnd, 0);
        break;
    }
    case WM_TIMER:
    {
        app.Update();
        InvalidateRect(hWnd, NULL, FALSE);
        break;
    }
    case WM_PAINT:
    {
        app.Show();
        break;
    }
    case WM_CLOSE:
    {
        app.CleanUp();
        DestroyWindow(hWnd);
        break;
    }
    case WM_DESTROY:
    {
        app.Destroy();
        break;
    }
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    const TCHAR szClassName[] = TEXT("MyClass");

    WNDCLASS wc;
    HWND hWnd;
    MSG msg;
    RECT rcClient;
    UINT style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE; // no maximize box and resizing

    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szClassName;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClass(&wc))
    {
        MessageBox(NULL, TEXT("Window Registration Failed!"), TEXT("errYor!"),
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    rcClient.left = 0;
    rcClient.top = 0;
    rcClient.right = SCREEN_WIDTH * SCREEN_SCALE_FACTOR;
    rcClient.bottom = SCREEN_HEIGHT * SCREEN_SCALE_FACTOR;

    AdjustWindowRectEx(&rcClient, style, TRUE, 0);

    hWnd = CreateWindow(szClassName,
        TEXT("SDLWin1"),
        style,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rcClient.right - rcClient.left,
        rcClient.bottom - rcClient.top,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (hWnd == NULL)
    {
        MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("errYor!"),
            MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}