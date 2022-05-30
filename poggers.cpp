/* g++ poggers.cpp -o poggers -std=c++14 -lSDL2 */

#include <SDL2/SDL.h>

#include "mygl.h"

// these header files must be placed after mygl.h for technical reasons
#include <Windows.h>
#include <Windowsx.h>

#define ID_TIMER 1

using namespace mygl;

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
const int SCREEN_SCALE_FACTOR = 1;

/*
Cube

    +6-------+5
   /         /|
 +7--------+8 |
  |         | |
  | +1      |+4
  |         |/
 +2--------+3

*/

const Model cube = {
    8,
    12,
    {
        vec4d(-50.0, -50.0, -50.0, 1.0), // 1
        vec4d(-50.0, -50.0, 50.0, 1.0),  // 2
        vec4d(50.0, -50.0, 50.0, 1.0),   // 3
        vec4d(50.0, -50.0, -50.0, 1.0),  // 4
        vec4d(50.0, 50.0, -50.0, 1.0),   // 5
        vec4d(-50.0, 50.0, -50.0, 1.0),  // 6
        vec4d(-50.0, 50.0, 50.0, 1.0),   // 7
        vec4d(50.0, 50.0, 50.0, 1.0),    // 8
    },
    {
        // Face 1-2-6-7
        {true, RED, {0, 1, 6}}, // 1-2-7
        {true, RED, {0, 5, 6}}, // 1-6-7

        // Face 2-3-7-8
        {true, YELLOW, {1, 2, 7}}, // 2-3-8
        {true, YELLOW, {1, 6, 7}}, // 2-7-8

        // Face 3-4-8-5
        {true, INDIGO, {2, 3, 4}}, // 3-4-5
        {true, INDIGO, {2, 7, 4}}, // 3-8-5

        // Face 4-1-5-6
        {true, GREEN, {0, 3, 4}}, // 1-4-5
        {true, GREEN, {0, 5, 4}}, // 1-6-5

        // Face 1-2-3-4
        {true, BLUE, {0, 1, 2}}, // 1-2-3
        {true, BLUE, {0, 3, 2}}, // 1-4-3

        // Face 5-6-7-8
        {true, ORANGE, {4, 5, 6}}, // 5-6-7
        {true, ORANGE, {4, 7, 6}}, // 5-8-7
    }
};

const vec3d xaxis = {1, 0, 0};
const vec3d yaxis = {0, 1, 0};
const vec3d zaxis = {0, 0, 1};

/* map s from [a1...a2] to [b1...b2] */
inline double map(double s, double a1, double a2, double b1, double b2) { return b1 + (s - a1) * (b2 - b1) / (a2 - a1); }

class Poggers : public RendererBase3D
{
public:
    Poggers(int width, int height);
    ~Poggers();

    void Create(HWND hwnd, int updateInterval);
    void Show();
    void CleanUp();
    void Destroy();

    void Init();
    void Update();
    void Render();

    void HandleMousePress(int mouseX, int mouseY);
    void HandleMouseRelease(int mouseX, int mouseY);
    void HandleMouseMotion(int mouseX, int mouseY);
private:
    HWND hwnd;

    SDL_Window* wnd;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    double angle; // for continuous counterclockwise rotation about y-axis
    vec3d p, q, n;
    double theta;
    Quaternion<double> currentQ;
    Quaternion<double> lastQ;

    const double dAngle = 0.02;

    vec3d Project(int mx, int my);
};

Poggers::Poggers(int width, int height)
  : RendererBase3D(width, height)
{}

Poggers::~Poggers()
{}

void Poggers::Create(HWND hWnd, int updateInterval)
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

void Poggers::Show()
{
    ClearScreen();
    Render();

    SDL_UpdateTexture(texture, NULL, &pixels[0], width * 4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void Poggers::CleanUp()
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

void Poggers::Destroy()
{
    PostQuitMessage(0);
}

void Poggers::Init()
{
    angle = 0.0;
    currentQ[0] = 1.0;
    lastQ = Quaternion<double>(zaxis, M_PI / 4.0); // the polygon is initially rotated 45 degree counterclockwise about z-axis
}

void Poggers::Update()
{
    angle += dAngle;
}

void Poggers::Render()
{
    Quaternion<double> rotatey(yaxis, angle);
    Quaternion<double> rotation = currentQ * lastQ * rotatey; // rotations can be composed by simply multiplying quaternions!

    mat4d rot = CreateRotationMatrix4<double>(rotation);
    mat4d trans = CreateTranslationMatrix4<double>(0.0, 0.0, -120.0);
    //mat4d proj = CreateOrthographic4<double>(-120.0, 120.0, -120.0, 120.0, 0.0, 200.0);
    mat4d proj = CreateViewingFrustum4<double>(-0.2, 0.2, -0.2, 0.2, 0.1, 140.0);

    mat4d vertexTransf = proj * trans * rot;

    // for viewport transform
    mat4d vpScale = CreateScalingMatrix4<double>(width / 2.0, height / 2.0, 1.0);
    mat4d vpTranslate = CreateTranslationMatrix4<double>(width / 2.0, height / 2.0, 0.0);

    mat4d vpTransf = vpTranslate * vpScale;

    int trigs = cube.ntrig;

    for (int i = 0; i < trigs; ++i)
    {
        Triangle t = cube.triangle[i];

        vec4d v1 = vertexTransf * cube.vertex[t.vertex[0]];
        vec4d v2 = vertexTransf * cube.vertex[t.vertex[1]];
        vec4d v3 = vertexTransf * cube.vertex[t.vertex[2]];

        // perspective division
        v1 /= v1[3];
        v2 /= v2[3];
        v3 /= v3[3];

        v1 = vpTransf * v1;
        v2 = vpTransf * v2;
        v3 = vpTransf * v3;

        v1[1] = height - v1[1];
        v2[1] = height - v2[1];
        v3[1] = height - v3[1];

        if (t.filled)
        {
            DrawFilledTriangle(v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], v3[0], v3[1], v3[2], t.colour);
        }
        else
        {
            DrawWireframeTriangle(v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], v3[0], v3[1], v3[2], t.colour);
        }
    }
}

void Poggers::HandleMousePress(int mouseX, int mouseY)
{
    p = Project(mouseX, mouseY);
}

void Poggers::HandleMouseRelease(int mouseX, int mouseY)
{
    lastQ = currentQ * lastQ;
    currentQ = Quaternion<double>(true);
}

void Poggers::HandleMouseMotion(int mouseX, int mouseY)
{
    q = Project(mouseX, mouseY);

    n = CrossProduct(p, q);
    theta = std::acos(p.Dot(q) / (p.Magnitude() * q.Magnitude()));

    currentQ = Quaternion<double>(n, theta);
}

vec3d Poggers::Project(int mx, int my)
{
    const double r = 1.0;

    double x = map(mx, 0, width - 1, -1, 1);
    double y = map(my, 0, height - 1, 1, -1);
    double z;

    if (x * x + y * y <= r * r / 2.0)
    {
        z = std::sqrt(r - x * x - y * y);
    }
    else
    {
        z = (r * r / 2.0) / std::sqrt(x * x + y * y);
    }

    return vec3d(x, y, z);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static Poggers app(SCREEN_WIDTH, SCREEN_HEIGHT);
    static bool bMousePressed = false;
    int mouseX, mouseY;

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
    case WM_LBUTTONDOWN:
    {
        mouseX = GET_X_LPARAM(lParam); // client x
        mouseY = GET_Y_LPARAM(lParam); // client y

        app.HandleMousePress(mouseX, mouseY);
        bMousePressed = true;

        break;
    }
    case WM_LBUTTONUP:
    {
        mouseX = GET_X_LPARAM(lParam);
        mouseY = GET_Y_LPARAM(lParam);

        app.HandleMouseRelease(mouseX, mouseY);
        bMousePressed = false;

        break;
    }
    case WM_MOUSEMOVE:
    {
        if ((wParam & MK_LBUTTON) != 0 && bMousePressed) // the left mouse button is pressed
        {
            mouseX = GET_X_LPARAM(lParam);
            mouseY = GET_Y_LPARAM(lParam);

            app.HandleMouseMotion(mouseX, mouseY);
        }
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
        TEXT("Poggers"),
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