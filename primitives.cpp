/* g++ primitives.cpp -o primitives -lSDL2 */

#include <algorithm>
#include <vector>
#include <limits>
#include <cmath>

#include <Windows.h>

#include <SDL2/SDL.h>

#define ID_TIMER 1
#define UPDATE_INTERVAL 0

struct Colour
{
    uint32_t argb;

    Colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        argb = (a << 24) | (r << 16) | (g << 8) | b;
    }
};

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
const int SCREEN_SCALE_FACTOR = 1;

const Colour RED(255, 0, 0, 255);
const Colour ORANGE(255, 127, 0, 255);
const Colour YELLOW(255, 255, 0, 255);
const Colour GREEN(0, 255, 0, 255);
const Colour BLUE(0, 0, 255, 255);
const Colour INDIGO(75, 0, 130, 255);
const Colour VIOLET(148, 0, 211, 255);

const Colour rainbow[7] = {
    RED,
    ORANGE,
    YELLOW,
    GREEN,
    BLUE,
    INDIGO,
    VIOLET
};

const float ZMIN = -1.0;

class RendererBase3D
{
public:
    RendererBase3D(int width, int height);
    ~RendererBase3D();

    void Create(HWND hwnd);
    void Show();
    void CleanUp();
    void Destroy();

    virtual void Init() = 0;
    virtual void Update() = 0;
    virtual void Render() = 0;
protected:
    SDL_Window *wnd;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    int width;
    int height;

    std::vector<uint32_t> pixels;
    std::vector<float> zdepth;

    /* Coordinate system:
       x goes right starting from top left corner
       y goes down starting from top left corner
       z goes into page starting from top left corner
     */
    void DrawFilledTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, const Colour& colour);
    void DrawUpperFilledTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, const Colour& colour);
    void DrawLowerFilledTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, const Colour& colour);
    void DrawWireFrameTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, const Colour& colour);
    void DrawLine(float x1, float y1, float z1, float x2, float y2, float z2, const Colour& colour);

    bool WithinBounds(float x, float y, float z); // for clipping
    bool Equalf(float f1, float f2);
    void Swap(float& x1, float& y1, float& z1, float& x2, float& y2, float& z2);
};

RendererBase3D::RendererBase3D(int width, int height)
  : width(width), height(height), pixels(width * height), zdepth(width * height)
{
    
}

RendererBase3D::~RendererBase3D()
{}

void RendererBase3D::Create(HWND hWnd)
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

    Init();
}

void RendererBase3D::Show()
{
    std::fill(zdepth.begin(), zdepth.end(), ZMIN);
    std::fill(pixels.begin(), pixels.end(), 0);
    Render();

    SDL_UpdateTexture(texture, NULL, &pixels[0], width * 4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void RendererBase3D::CleanUp()
{
    SDL_DestroyTexture(texture);
    texture = NULL;

    SDL_DestroyRenderer(renderer);
    renderer = NULL;

    SDL_DestroyWindow(wnd);
    wnd = NULL;

    SDL_Quit();
}

void RendererBase3D::Destroy()
{
    PostQuitMessage(0);
}

// Nice reference: http://www.sunshine2k.de/coding/java/TriangleRasterization/generalTriangle.png
void RendererBase3D::DrawFilledTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, const Colour& colour)
{
    // Sort p1, p2, and p3 so that p1 < p2 < p3
    if (y2 < y1) { Swap(x2, y2, z2, x1, y1, z1); }
    if (y3 < y1) { Swap(x3, y3, z3, x1, y1, z1); }
    if (y3 < y2) { Swap(x3, y3, z3, x2, y2, z2); }

    if (Equalf(y2, y3))
    {
        DrawUpperFilledTriangle(x1, y1, z1, x2, y2, z2, x3, y3, z3, colour);
    }
    else if (Equalf(y1, y2))
    {
        DrawLowerFilledTriangle(x1, y1, z1, x2, y2, z2, x3, y3, z3, colour);
    }
    else
    {
        // Find x4, y4, and z4 using similar triangles: (x3-x1)/(y3-y1)=(x4-x1)/(y4-y1), y4=y2
        float x4 = x1 + (y2 - y1) * (x3 - x1) / (y3 - y1);
        float y4 = y2;
        float z4;

        if (z1 > z3) // p1 is farther than p3
        {
            z4 = z3 + (y3 - y4) * (z1 - z3) / (y3 - y1);
        }
        else
        {
            z4 = z3 - (y3 - y4) * (z3 - z1) / (y3 - y1);
        }

        DrawUpperFilledTriangle(x1, y1, z1, x2, y2, z2, x4, y4, z4, colour);
        DrawLowerFilledTriangle(x2, y2, z2, x4, y4, z4, x3, y3, z3, colour);
    }
}

void RendererBase3D::DrawUpperFilledTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, const Colour& colour)
{
    float dy1 = std::fabs(y2 - y1);
    float dy2 = std::fabs(y3 - y1);

    float curx1 = x1;
    float curx2 = x1;
    float xinc1 = (x2 - x1) / dy1;
    float xinc2 = (x3 - x1) / dy2;

    float curz1 = z1;
    float curz2 = z1;
    float zinc1 = (z2 - z1) / dy1;
    float zinc2 = (z3 - z1) / dy2;

    for (int y = y1; y <= y2; ++y)
    {
        DrawLine(curx1, y, curz1, curx2, y, curz2, colour);

        curx1 += xinc1;
        curx2 += xinc2;
        curz1 += zinc1;
        curz2 += zinc2;
    }
}

void RendererBase3D::DrawLowerFilledTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, const Colour& colour)
{
    float dy1 = std::fabs(y3 - y1);
    float dy2 = std::fabs(y3 - y2);

    float curx1 = x3;
    float curx2 = x3;
    float xinc1 = (x3 - x1) / dy1;
    float xinc2 = (x3 - x2) / dy2;

    float curz1 = z3;
    float curz2 = z3;
    float zinc1 = (z3 - z1) / dy1;
    float zinc2 = (z3 - z2) / dy2;

    for (int y = y3; y >= y1; --y)
    {
        DrawLine(curx1, y, curz1, curx2, y, curz2, colour);

        curx1 -= xinc1;
        curx2 -= xinc2;
        curz1 -= zinc1;
        curz2 -= zinc2;
    }
}

void RendererBase3D::DrawWireFrameTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, const Colour& colour)
{
    DrawLine(x1, y1, z1, x2, y2, z2, colour);
    DrawLine(x1, y1, z1, x3, y3, z3, colour);
    DrawLine(x2, y2, z2, x3, y3, z3, colour);
}

// Adapted from https://github.com/ssloy/tinyrenderer/wiki/Lesson-1:-Bresenham%E2%80%99s-Line-Drawing-Algorithm
void RendererBase3D::DrawLine(float x1, float y1, float z1, float x2, float y2, float z2, const Colour& colour)
{
    bool steep = false;

    if (std::fabs(x1 - x2) < std::fabs(y1 - y2))
    {
        Swap(x1, x2, z1, y1, y2, z1);
        steep = true;
    }

    if (x1 > x2)
    {
        Swap(x1, y1, z1, x2, y2, z2);
    }

    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;

    int y = y1;
    int yinc = y2 > y1 ? 1 : -1;
    float derrY = std::fabs(dy) * 2.0;
    float errY = 0.0;

    float z = z1;
    float zinc = dz / std::fabs(dx);

    if (steep)
    {
        for (int x = x1; x <= x2; ++x)
        {
            int offset = x * width + y;
            float depth = 1.0 / z;

            if (zdepth[offset] < depth)
            {
                zdepth[offset] = depth;
                pixels[offset] = colour.argb;
            }

            errY += derrY;
            if (errY > dx)
            {
                y += yinc;
                errY -= dx * 2;
            }

            z += zinc;
        }
    }
    else
    {
        for (int x = x1; x <= x2; ++x)
        {
            int offset = y * width + x;
            float depth = 1.0 / z;

            if (zdepth[offset] < depth)
            {
                zdepth[offset] = depth;
                pixels[offset] = colour.argb;
            }

            errY += derrY;
            if (errY > dx)
            {
                y += yinc;
                errY -= dx * 2;
            }

            z += zinc;
        }
    }
}

bool RendererBase3D::WithinBounds(float x, float y, float z)
{
    return x >= 0.0 && x < width &&
           y >= 0.0 && y < height &&
           z >= 0.0;
}

bool RendererBase3D::Equalf(float f1, float f2)
{
    return (std::fabs(f1 - f2) <= std::numeric_limits<float>::epsilon() * std::fmax(std::fabs(f1), std::fabs(f2)));
}

void RendererBase3D::Swap(float& x1, float& y1, float& z1, float& x2, float& y2, float& z2)
{
    std::swap(x1, x2);
    std::swap(y1, y2);
    std::swap(z1, z2);
}

class TestPrimitives : public RendererBase3D
{
public:
    TestPrimitives(int width, int height);
    ~TestPrimitives();

    void Init();
    void Update();
    void Render();
private:
    float radius;
    float angle, da;
};

TestPrimitives::TestPrimitives(int width, int height)
  : RendererBase3D(width, height)
{}

TestPrimitives::~TestPrimitives()
{}

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
/*
    float centerX = width / 2.0;
    float centerY = height / 2.0;

    float radius = 80.0;
    float dr = 20.0;

    float z = 0.0;
    float dz = 10.0;
    float middlez = 35.0;

    for (int i = 0; i < 7; ++i)
    {
        float angle = 0.0;

        while (angle < 360.0)
        {
            float x = centerX + radius * std::cos(angle);
            float y = centerY + radius * std::sin(angle);

            DrawLine(centerX, centerY, middlez, x, y, z, rainbow[i]);

            angle += 1.0;
        }

        radius += dr;
        z += dz;
    }
*/
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
        app.Create(hWnd);

        if(!SetTimer(hWnd, ID_TIMER, UPDATE_INTERVAL, NULL))
        {
            MessageBox(hWnd, "Could not set timer!", "errYor", MB_OK | MB_ICONEXCLAMATION);
            PostQuitMessage(1);
        }
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
        KillTimer(hWnd, ID_TIMER);
        DestroyWindow(hWnd);
        break;
    }
    case WM_DESTROY:
        app.Destroy();
        break;
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