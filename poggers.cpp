/* g++ poggers.cpp -o poggers -std=c++14 -lSDL2 */
/*
TODO
- shaders
- camera
- amp.h to exploit gpu
- unproject https://dondi.lmu.build/share/cg/unproject-explained.pdf
- gif encoder
*/

#include <SDL2/SDL.h>

#include "mygl.h"

// these header files must be placed after mygl.h for technical reasons
#include <Windows.h>
#include <Windowsx.h>

//debug
//#include <iostream>

#define ID_TIMER 1

using namespace mygl;

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;
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
        vec4f(-50.0f, -50.0f, -50.0f, 1.0f), // 1
        vec4f(-50.0f, -50.0f, 50.0f, 1.0f),  // 2
        vec4f(50.0f, -50.0f, 50.0f, 1.0f),   // 3
        vec4f(50.0f, -50.0f, -50.0f, 1.0f),  // 4
        vec4f(50.0f, 50.0f, -50.0f, 1.0f),   // 5
        vec4f(-50.0f, 50.0f, -50.0f, 1.0f),  // 6
        vec4f(-50.0f, 50.0f, 50.0f, 1.0f),   // 7
        vec4f(50.0f, 50.0f, 50.0f, 1.0f),    // 8
    },
    {
        // Face 1-2-6-7
        {true, RED, {0, 6, 1}},   // 1-7-2
        {true, RED, {0, 5, 6}},   // 1-6-7

        // Face 2-3-7-8
        {true, YELLOW, {1, 7, 2}}, // 2-8-3
        {true, YELLOW, {1, 6, 7}}, // 2-7-8

        // Face 3-4-8-5
        {true, INDIGO, {2, 4, 3}}, // 3-5-4
        {true, INDIGO, {2, 7, 4}}, // 3-8-5

        // Face 4-1-5-6
        {true, GREEN, {0, 3, 4}}, // 1-4-5
        {true, GREEN, {0, 4, 5}}, // 1-5-6

        // Face 1-2-3-4
        {true, BLUE, {0, 1, 2}},  // 1-2-3
        {true, BLUE, {0, 2, 3}},  // 1-3-4

        // Face 5-6-7-8
        {true, ORANGE, {4, 6, 5}}, // 5-7-6
        {true, ORANGE, {4, 7, 6}}, // 5-8-7
    }
};

const vec3f xaxis = {1, 0, 0};
const vec3f yaxis = {0, 1, 0};
const vec3f zaxis = {0, 0, 1};

/* map s from [a1...a2] to [b1...b2] */
inline float map(float s, float a1, float a2, float b1, float b2) { return b1 + (s - a1) * (b2 - b1) / (a2 - a1); }

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

    float angle; // for continuous counterclockwise rotation about y-axis
    const float dAngle = 0.02f;

    vec3f light; // direction of light source

    vec3f p, q, n;
    Quaternion<float> currentQ, lastQ;
    Quaternion<float> rotatey;
    mat4f trans, modelm, projm;
    mat4f vpTransf;

    vec3f Project(int mx, int my);
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
    // TODO why the fucking fuck the coordinates for defining light position seems to be reversed
    light = vec3f(0.0f, 0.0f, -50.0f).Unit(); // (in world coordinates) light comes out behind the screen (normalized)

    angle = 0.0f;
    rotatey = Quaternion<float>(true);

    currentQ = Quaternion<float>(true);
    lastQ = Quaternion<float>(zaxis, M_PI / 4.0f); // the cube is initially rotated 45 degree counterclockwise about z-axis

    mat4f rot = CreateRotationMatrix4<float>(lastQ);

    trans = CreateTranslationMatrix4<float>(0.0f, 0.0f, -100.0f);
    modelm = trans * rot;
    projm = CreateOrthographic4<float>(-120.0f, 120.0f, -120.0f, 120.0f, 0.0f, 200.0f); // CreateViewingFrustum4<float>(-0.2f, 0.2f, -0.2f, 0.2f, 0.1f, 140.0f);

    // for viewport transform
    mat4f vpScale = CreateScalingMatrix4<float>(width / 2.0f, -height / 2.0f, width / 2.0f); // the minus sign is used to flip y axis; assume that the depth of z is width
    mat4f vpTranslate = CreateTranslationMatrix4<float>(width / 2.0f, height / 2.0f, width / 2.0f + 0.5f); // +0.5 to make sure that z > 0

    vpTransf = vpTranslate * vpScale;
}

void Poggers::Update()
{
    angle += dAngle;

    rotatey = Quaternion<float>(yaxis, angle);
    mat4f rot = CreateRotationMatrix4<float>(currentQ * lastQ * rotatey);

    modelm = trans * rot;
}

void Poggers::Render()
{
    int trigs = cube.ntrig;

    //debug
    //int drawn = 0;

    for (int i = 0; i < trigs; ++i)
    {
        Triangle t = cube.triangle[i];

        vec4f v1 = modelm * cube.vertex[t.vertex[0]];
        vec4f v2 = modelm * cube.vertex[t.vertex[1]];
        vec4f v3 = modelm * cube.vertex[t.vertex[2]];

        vec3f vert1 = v1.Demote();
        vec3f vert2 = v2.Demote();
        vec3f vert3 = v3.Demote();

        // vector normal to surface
        vec3f n = CrossProduct(vert2 - vert1, vert3 - vert1).Unit();

        // luminance
        float L = n.Dot(light);

        //debug
        //std::cerr << "Triangle #" << i << ": Luminance " << L << std::endl;

        // L <= 0 means the triangle is hidden from the view
        if (L > 0.0f)
        {
            v1 = projm * v1;
            v2 = projm * v2;
            v3 = projm * v3;

            // perspective division
            v1 /= v1[3];
            v2 /= v2[3];
            v3 /= v3[3];

            v1 = vpTransf * v1;
            v2 = vpTransf * v2;
            v3 = vpTransf * v3;

            if (t.filled)
            {
                DrawFilledTriangleBarycentric(v1.Demote(), v2.Demote(), v3.Demote(), t.colour.AdjustBrightness(L));
            }
            else
            {
                DrawWireframeTriangleDDA(v1.Demote(), v2.Demote(), v3.Demote(), t.colour);
            }

            //drawn++;
        }
    }

    //std::cerr << "Number of triangles drawn: " << drawn << std::endl;
}

void Poggers::HandleMousePress(int mouseX, int mouseY)
{
    p = Project(mouseX, mouseY);
}

void Poggers::HandleMouseRelease(int mouseX, int mouseY)
{
    lastQ = currentQ * lastQ;
    currentQ = Quaternion<float>(true);
}

void Poggers::HandleMouseMotion(int mouseX, int mouseY)
{
    q = Project(mouseX, mouseY);

    n = CrossProduct(p, q);
    float theta = std::acos(p.Dot(q) / (p.Magnitude() * q.Magnitude()));

    currentQ = Quaternion<float>(n, theta);

    mat4f rot = CreateRotationMatrix4<float>(currentQ * lastQ * rotatey);
    modelm = trans * rot;
}

vec3f Poggers::Project(int mx, int my)
{
    const float r = 1.0f;

    float x = map(mx, 0, width - 1, -1, 1);
    float y = map(my, 0, height - 1, 1, -1);
    float z;

    if (x * x + y * y <= r * r / 2.0f)
    {
        z = std::sqrt(r - x * x - y * y);
    }
    else
    {
        z = (r * r / 2.0f) / std::sqrt(x * x + y * y);
    }

    return vec3f(x, y, z);
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