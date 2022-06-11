/* g++ rubik.cpp -o rubik -std=c++14 -lSDL2 */
/*
TODO
- reimplement arcball using unproject matrix (no projection to sphere)
- bitmask to represent colour array
- super interactive cube control (ie. improve mouse interaction)
- numerical error accumulation when dealing with floating point values
*/

#include <array>
#include <algorithm>

#include <cstdlib>
#include <ctime>

//debug
#include <iostream>

#include <SDL2/SDL.h>

#include "mygl.h"

// these header files must be placed after mygl.h for technical reasons
#include <Windows.h>
#include <Windowsx.h>

#define ID_TIMER 1

using namespace mygl;

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;
const int SCREEN_SCALE_FACTOR = 1;

struct Cubie
{
    Colour col[6]; // colour for each of the 6 faces
    mat4f position; // represents cube's position in 3D space (points to its center; encodes both translations and rotations)
};

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
        vec4f(-18.0f, -18.0f, -18.0f, 1.0f), // 1
        vec4f(-18.0f, -18.0f, 18.0f, 1.0f),  // 2
        vec4f(18.0f, -18.0f, 18.0f, 1.0f),   // 3
        vec4f(18.0f, -18.0f, -18.0f, 1.0f),  // 4
        vec4f(18.0f, 18.0f, -18.0f, 1.0f),   // 5
        vec4f(-18.0f, 18.0f, -18.0f, 1.0f),  // 6
        vec4f(-18.0f, 18.0f, 18.0f, 1.0f),   // 7
        vec4f(18.0f, 18.0f, 18.0f, 1.0f),    // 8
    },
    {
        // Face 1-2-6-7
        {true, Colour(), {0, 6, 1}}, // 1-7-2
        {true, Colour(), {0, 5, 6}}, // 1-6-7

        // Face 2-3-7-8
        {true, Colour(), {1, 7, 2}}, // 2-8-3
        {true, Colour(), {1, 6, 7}}, // 2-7-8

        // Face 3-4-8-5
        {true, Colour(), {2, 4, 3}}, // 3-5-4
        {true, Colour(), {2, 7, 4}}, // 3-8-5

        // Face 4-1-5-6
        {true, Colour(), {0, 3, 4}}, // 1-4-5
        {true, Colour(), {0, 4, 5}}, // 1-5-6

        // Face 1-2-3-4
        {true, Colour(), {0, 1, 2}}, // 1-2-3
        {true, Colour(), {0, 2, 3}}, // 1-3-4

        // Face 5-6-7-8
        {true, Colour(), {4, 6, 5}}, // 5-7-6
        {true, Colour(), {4, 7, 6}}, // 5-8-7
    }
};

const Colour RUBIK_GREEN(0, 155, 72, 255);

const vec3f xaxis = {1, 0, 0};
const vec3f yaxis = {0, 1, 0};
const vec3f zaxis = {0, 0, 1};

/*
Cubie array indexes for 2x2 cube
    +0-------+1
   /         /|
 +2--------+3 |
  |         | |
  | +4      |+5
  |         |/
 +6--------+7
*/

const int rotation_group[6][4] = {
    /* top and bottom layers */
    {0, 1, 2, 3}, // 0
    {4, 5, 6, 7}, // 1

    /* front and back layers */
    {2, 3, 6, 7}, // 2
    {0, 1, 4, 5}, // 3

    /* left and right layers */
    {0, 2, 4, 6}, // 4
    {1, 3, 5, 7}, // 5
};

/* an index in cubie array corresponds to which rotation group? */
const int group_index[3][8] = {
    // +x/-x axis
    {4, 5, 4, 5, 4, 5, 4, 5},

    // +y/-y axis
    {0, 0, 0, 0, 1, 1, 1, 1},

    // +z/-z axis
    {3, 3, 2, 2, 3, 3, 2, 2},
};

/* normal vector directions */
enum {
    X_AXIS=0,
    N_X_AXIS,
    Y_AXIS,
    N_Y_AXIS,
    Z_AXIS,
    N_Z_AXIS
};

class Rubik : public RendererBase3D
{
public:
    Rubik(int width, int height);
    ~Rubik();

    void Create(HWND hwnd);
    void Show();
    void CleanUp();
    void Destroy();

    void Init();
    void Render();
    void Update();

    void PutPixel(int x, int y, float depth, uint32_t argb) override;

    void StartScramble();

    void HandleMousePress(int mouseX, int mouseY);
    void HandleMouseRelease(int mouseX, int mouseY);
    void HandleMouseMotion(int mouseX, int mouseY);

    void HandleRightMouseButtonPress(int mouseX, int mouseY);
    void HandleRightMouseButtonRelease(int mouseX, int mouseY);
    void HandleMouseMotionR(int mouseX, int mouseY);
private:
    HWND hwnd;

    SDL_Window* wnd;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    Cubie rubik_cube[8];

    int cur_idx;
    int cur_face;
    int flagged_index;
    int flagged_face;
    bool on_cube;

    // position on screen corresponds to which cubie and which face?
    // each element is 8 bit unsigned where higher nibble represents face number and lower nibble represents cube index (for cubie array)
    std::vector<uint8_t> mask;

    //debug
    vec4f normal, origin;

    vec3f light; // direction of light source (from model's pov)

    bool rotating;
    bool mouselock;
    float angle;
    float da;
    vec3f axis;
    int which; // 0-left/right; 1-top/bottom; 2-front/back
    int group;
    int orien;

    bool scrambling;
    bool noaxis;
    int ntimes;

    vec3f p, q;
    Quaternion<float> currentQ, lastQ;

    mat4f trans, modelm, projm;
    mat4f vpTransf;

    mat4f modelmi, trans_projmi;
    // to unproject screen coordinates (x, y, depth), use unprojm*vec4f(x, y, 1/depth, 1.0f)
    // warning: it might not work if perspective projection is used...
    mat4f unprojm;

    float xscale;
    float yscale;

    vec3f ProjectToSphere(int mouseX, int mouseY);
    vec3f Unproject(int mouseX, int mouseY);

    void RotateSwap(int group, int orien);
};

Rubik::Rubik(int width, int height)
  : RendererBase3D(width, height), mask(width * height)
{
    std::fill(mask.begin(), mask.end(), -1); // -1 means index not specified
}

Rubik::~Rubik()
{}

void Rubik::Create(HWND hWnd)
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

    hwnd = hWnd;

    Init();
}

void Rubik::Show()
{
    ClearScreen();
    Render();

    SDL_UpdateTexture(texture, NULL, &pixels[0], width * 4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void Rubik::CleanUp()
{
    SDL_DestroyTexture(texture);
    texture = NULL;

    SDL_DestroyRenderer(renderer);
    renderer = NULL;

    SDL_DestroyWindow(wnd);
    wnd = NULL;

    SDL_Quit();
}

void Rubik::Destroy()
{
    PostQuitMessage(0);
}

void Rubik::Init()
{
    /* top layer */

    /* list of cubies starting from top left to bottom right cubie */
    rubik_cube[0].col[0] = RED;
    rubik_cube[0].col[1] = BLACK;
    rubik_cube[0].col[2] = BLACK;
    rubik_cube[0].col[3] = RUBIK_GREEN;
    rubik_cube[0].col[4] = BLACK;
    rubik_cube[0].col[5] = WHITE;
    rubik_cube[0].position = CreateTranslationMatrix4<float>(-20.0f, 20.0f, -20.0f);

    rubik_cube[1].col[0] = BLACK;
    rubik_cube[1].col[1] = BLACK;
    rubik_cube[1].col[2] = ORANGE;
    rubik_cube[1].col[3] = RUBIK_GREEN;
    rubik_cube[1].col[4] = BLACK;
    rubik_cube[1].col[5] = WHITE;
    rubik_cube[1].position = CreateTranslationMatrix4<float>(20.0f, 20.0f, -20.0f);

    rubik_cube[2].col[0] = RED;
    rubik_cube[2].col[1] = BLUE;
    rubik_cube[2].col[2] = BLACK;
    rubik_cube[2].col[3] = BLACK;
    rubik_cube[2].col[4] = BLACK;
    rubik_cube[2].col[5] = WHITE;
    rubik_cube[2].position = CreateTranslationMatrix4<float>(-20.0f, 20.0f, 20.0f);

    rubik_cube[3].col[0] = BLACK;
    rubik_cube[3].col[1] = BLUE;
    rubik_cube[3].col[2] = ORANGE;
    rubik_cube[3].col[3] = BLACK;
    rubik_cube[3].col[4] = BLACK;
    rubik_cube[3].col[5] = WHITE;
    rubik_cube[3].position = CreateTranslationMatrix4<float>(20.0f, 20.0f, 20.0f);

    /* bottom layer */

    rubik_cube[4].col[0] = RED;
    rubik_cube[4].col[1] = BLACK;
    rubik_cube[4].col[2] = BLACK;
    rubik_cube[4].col[3] = RUBIK_GREEN;
    rubik_cube[4].col[4] = YELLOW;
    rubik_cube[4].col[5] = BLACK;
    rubik_cube[4].position = CreateTranslationMatrix4<float>(-20.0f, -20.0f, -20.0f);

    rubik_cube[5].col[0] = BLACK;
    rubik_cube[5].col[1] = BLACK;
    rubik_cube[5].col[2] = ORANGE;
    rubik_cube[5].col[3] = RUBIK_GREEN;
    rubik_cube[5].col[4] = YELLOW;
    rubik_cube[5].col[5] = BLACK;
    rubik_cube[5].position = CreateTranslationMatrix4<float>(20.0f, -20.0f, -20.0f);

    rubik_cube[6].col[0] = RED;
    rubik_cube[6].col[1] = BLUE;
    rubik_cube[6].col[2] = BLACK;
    rubik_cube[6].col[3] = BLACK;
    rubik_cube[6].col[4] = YELLOW;
    rubik_cube[6].col[5] = BLACK;
    rubik_cube[6].position = CreateTranslationMatrix4<float>(-20.0f, -20.0f, 20.0f);

    rubik_cube[7].col[0] = BLACK;
    rubik_cube[7].col[1] = BLUE;
    rubik_cube[7].col[2] = ORANGE;
    rubik_cube[7].col[3] = BLACK;
    rubik_cube[7].col[4] = YELLOW;
    rubik_cube[7].col[5] = BLACK;
    rubik_cube[7].position = CreateTranslationMatrix4<float>(20.0f, -20.0f, 20.0f);

    flagged_index = -1;
    flagged_face = -1;
    on_cube = false;

    //debug
    normal = vec4f(0.0f, 50.0f, 0.0f, 1.0f);
    origin = vec4f(0.0f, 0.0f, 0.0f, 1.0f);

    light = vec3f(0.0f, 0.0f, 50.0f).Unit(); // (in world coordinates) light comes out behind the screen (normalized)

    rotating = false;
    mouselock = false;
    da = 0.1f;

    scrambling = false;

    currentQ = Quaternion<float>(true);
    lastQ = Quaternion<float>(true);

    trans = CreateTranslationMatrix4<float>(0.0f, 0.0f, -100.0f);
    modelm = trans;
    projm = CreateOrthographic4<float>(-120.0f, 120.0f, -120.0f, 120.0f, 0.0f, 200.0f); // CreateViewingFrustum4<float>(-0.2f, 0.2f, -0.2f, 0.2f, 0.1f, 140.0f);

    // for viewport transform
    mat4f vpScale = CreateScalingMatrix4<float>(width / 2.0f, -height / 2.0f, width / 2.0f); // the minus sign is used to flip y axis; assume that the depth of z is width
    mat4f vpTranslate = CreateTranslationMatrix4<float>(width / 2.0f, height / 2.0f, width / 2.0f + 0.5f); // +0.5 to make sure that z > 0

    vpTransf = vpTranslate * vpScale;

    mat4f vpTransfi = Inverse4<float>(vpTransf);
    mat4f projmi = Inverse4<float>(projm);

    trans_projmi = projmi * vpTransfi;
    modelmi = Inverse4<float>(modelm);
    unprojm = modelmi * trans_projmi;

    xscale = 2.0f / (width - 1.0f);
    yscale = 2.0f / (height - 1.0f);

    std::srand(static_cast<unsigned>(time(NULL)));
}

void Rubik::Render()
{
    std::fill(mask.begin(), mask.end(), -1); // important!

    int trigs = cube.ntrig;

    vec4f vertexes[8][trigs][3]; // preprocessed list of vertexes

    for (int idx = 0; idx < 8; ++idx)
    {
        for (int i = 0; i < trigs; ++i)
        {
            Triangle t = cube.triangle[i];

            vertexes[idx][i][0] = rubik_cube[idx].position * cube.vertex[t.vertex[0]];
            vertexes[idx][i][1] = rubik_cube[idx].position * cube.vertex[t.vertex[1]];
            vertexes[idx][i][2] = rubik_cube[idx].position * cube.vertex[t.vertex[2]];
        }
    }

    if (rotating)
    {
        mat4f rotate = CreateRotationMatrix4<float>(Quaternion<float>(axis, angle));

        // apply rotation to each cubie in rotation group
        for (int j = 0; j < 4; ++j)
        {
            int idx = rotation_group[group][j]; // cubie index

            for (int i = 0; i < trigs; ++i)
            {
                vertexes[idx][i][0] = rotate * vertexes[idx][i][0];
                vertexes[idx][i][1] = rotate * vertexes[idx][i][1];
                vertexes[idx][i][2] = rotate * vertexes[idx][i][2];
            }
        }
    }

    for (int idx = 0; idx < 8; ++idx)
    {
        cur_idx = idx;

        for (int i = 0; i < trigs; ++i)
        {
            cur_face = i / 2;

            Colour col = rubik_cube[cur_idx].col[cur_face];

            // optimization: don't render if the colour matches the background
            if (col.argb == BLACK.argb) continue;

            Triangle t = cube.triangle[i];

            vec4f v1 = modelm * vertexes[idx][i][0];
            vec4f v2 = modelm * vertexes[idx][i][1];
            vec4f v3 = modelm * vertexes[idx][i][2];

            vec3f vert1 = v1.Demote();
            vec3f vert2 = v2.Demote();
            vec3f vert3 = v3.Demote();

            // vector normal to surface
            vec3f n = CrossProduct(vert3 - vert1, vert2 - vert1).Unit();

            // luminance
            float L = n * light;

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

                if (cur_idx == flagged_index && cur_face == flagged_face)
                {
                    col = col.Contrast();
                }

                DrawFilledTriangleBarycentric(v1.Demote(), v2.Demote(), v3.Demote(), col.AdjustBrightness(L));
            }
        }
    }

    //debug
    mat4f vTrans = projm * modelm;
    vec4f n = vTrans * normal;
    vec4f o = vTrans * origin;
    n /= n[3];
    o /= o[3];
    n = vpTransf * n;
    o = vpTransf * o;
    DrawLineDDA(o.Demote(), n.Demote(), RED);
}

void Rubik::Update()
{
    bool done = false;

    if (!scrambling)
    {
        angle += da;

        if (angle >= M_PI_2)
        {
            RotateSwap(group, orien);
            done = true;
        }
    }
    else // scrambling
    {
        if (ntimes == 0)
        {
            done = true;
        }
        else
        {
            if (noaxis)
            {
                orien = std::rand() % 6;

                switch (orien)
                {
                case 0: axis = xaxis; break;
                case 1: axis = -xaxis; break;
                case 2: axis = yaxis; break;
                case 3: axis = -yaxis; break;
                case 4: axis = zaxis; break;
                case 5: axis = -zaxis; break;
                }
                normal = vec4f(axis[0] * 80.0f, axis[1] * 80.0f, axis[2] * 80.0f, 1.0f);

                which = orien / 2;
                group = group_index[which][std::rand() % 8];
                angle = 0.0f;

                noaxis = false;
            }
            else
            {
                angle += da;

                if (angle >= M_PI_2)
                {
                    RotateSwap(group, orien);
                    ntimes--;
                    noaxis = true;
                }
            }
        }
    }

    if (done)
    {
        KillTimer(hwnd, ID_TIMER);
        rotating = false;
        mouselock = false;
        scrambling = false;
        flagged_index = flagged_face = -1;
    }
}

void Rubik::PutPixel(int x, int y, float depth, uint32_t argb)
{
    int offset = y * width + x;

    if (zdepth[offset] < depth)
    {
        zdepth[offset] = depth;
        pixels[offset] = argb;
        mask[offset] = (cur_face << 4) | cur_idx;
    }
}

void Rubik::StartScramble()
{
    scrambling = true;
    noaxis = true;
    mouselock = true;
    rotating = true;
    ntimes = 10;

    if(!SetTimer(hwnd, ID_TIMER, 1, NULL))
    {
        MessageBox(hwnd, "Could not set timer!", "errYor", MB_OK | MB_ICONEXCLAMATION);
        PostQuitMessage(1);
    }
}

void Rubik::HandleMousePress(int mouseX, int mouseY)
{
    if (mouselock) return;

    p = ProjectToSphere(mouseX, mouseY);
}

void Rubik::HandleMouseRelease(int mouseX, int mouseY)
{
    if (mouselock) return;

    lastQ = currentQ * lastQ;
    currentQ = Quaternion<float>(true);
}

void Rubik::HandleMouseMotion(int mouseX, int mouseY)
{
    if (mouselock) return;

    q = ProjectToSphere(mouseX, mouseY);

    vec3f n = CrossProduct(p, q);
    float theta = std::acos((p * q) / (p.Magnitude() * q.Magnitude()));

    currentQ = Quaternion<float>(n, theta);

    mat4f rot = CreateRotationMatrix4<float>(currentQ * lastQ);
    modelm = trans * rot;
    modelmi = Inverse4<float>(modelm);
    unprojm = modelmi * trans_projmi;
}

void Rubik::HandleRightMouseButtonPress(int mouseX, int mouseY)
{
    if (mouselock) return;

    int offset = mouseY * width + mouseX;

    flagged_index = mask[offset] & 0b1111;
    flagged_face = mask[offset] >> 4;

    if ((flagged_index >= 0 && flagged_index < 8) &&
        (flagged_face >= 0 && flagged_face < 6))
        on_cube = true;

    std::cerr << "index=" << flagged_index << ", face=" << flagged_face << std::endl;

    p = Unproject(mouseX, mouseY);
}

void Rubik::HandleRightMouseButtonRelease(int mouseX, int mouseY)
{
    if (mouselock) return;

    flagged_index = flagged_face = -1;
    on_cube = false;
}

void Rubik::HandleMouseMotionR(int mouseX, int mouseY)
{
    if (!on_cube || mouselock) return;

    q = Unproject(mouseX, mouseY);

    vec3f drag = q - p; // drag vector

    if (drag.Magnitude() < 1e-1) return;

    float x = std::fabs(drag[0]);
    float y = std::fabs(drag[1]);
    float z = std::fabs(drag[2]);

    // x is the largest
    if (x > y && x > z) drag[1] = drag[2] = 0.0f;

    // y is the largest
    else if (y > x && y > z) drag[0] = drag[2] = 0.0f;

    // z is the largest
    else drag[0] = drag[1] = 0.0f;

    drag = drag.Unit();

    std::cerr << "p=" << p << ", q=" << q << ", drag=" << drag << std::endl;

    Triangle t = cube.triangle[flagged_face * 2];

    vec4f v1 = rubik_cube[flagged_index].position * cube.vertex[t.vertex[0]];
    vec4f v2 = rubik_cube[flagged_index].position * cube.vertex[t.vertex[1]];
    vec4f v3 = rubik_cube[flagged_index].position * cube.vertex[t.vertex[2]];

    vec3f vert1 = v1.Demote();
    vec3f vert2 = v2.Demote();
    vec3f vert3 = v3.Demote();

    vec3f surface_normal = CrossProduct(vert3 - vert1, vert2 - vert1).Unit(); // normal to the triangle's surface

    x = std::fabs(surface_normal[0]);
    y = std::fabs(surface_normal[1]);
    z = std::fabs(surface_normal[2]);

    // x is the largest
    if (x > y && x > z) surface_normal[1] = surface_normal[2] = 0.0f;

    // y is the largest
    else if (y > x && y > z) surface_normal[0] = surface_normal[2] = 0.0f;

    // z is the largest
    else surface_normal[0] = surface_normal[1] = 0.0f;

    std::cerr << "surface_n=" << surface_normal << std::endl;

    vec3f n = CrossProduct(surface_normal, drag); // normal vector for rotation TODO what to do if it is zero?

    std::cerr << "n=" << n << std::endl;

    normal = vec4f(n[0] * 80.0f, n[1] * 80.0f, n[2] * 80.0f, 1.0f);

    rotating = true;
    mouselock = true;

         if (n == xaxis)  { orien = X_AXIS;   }
    else if (n == -xaxis) { orien = N_X_AXIS; }
    else if (n == yaxis)  { orien = Y_AXIS;   }
    else if (n == -yaxis) { orien = N_Y_AXIS; }
    else if (n == zaxis)  { orien = Z_AXIS;   }
    else if (n == -zaxis) { orien = N_Z_AXIS; }

    which = orien / 2;
    group = group_index[which][flagged_index];

    std::cerr << "which=" << which << ", orien=" << orien << ", group=" << group << std::endl;

    axis = n;
    angle = 0.0f;

    if(!SetTimer(hwnd, ID_TIMER, 1, NULL))
    {
        MessageBox(hwnd, "Could not set timer!", "errYor", MB_OK | MB_ICONEXCLAMATION);
        PostQuitMessage(1);
    }
}

vec3f Rubik::ProjectToSphere(int mouseX, int mouseY)
{
    const float r = 1.0f;

    /* x and y are mapped to [-1, 1] */
    float x = (mouseX * xscale) - 1.0f;
    float y = 1.0f - (mouseY * yscale);
    float z;

    float length2 = x * x + y * y;

    if (length2 <= r * r / 2.0f) // inside the sphere
    {
        z = std::sqrt(r * r - length2);
    }
    else
    {
        z = (r * r / 2.0f) / std::sqrt(length2);
    }

    return vec3f(x, y, z);
}

vec3f Rubik::Unproject(int mouseX, int mouseY)
{
    const float r = 40.0f;

    // it returns world coordinates but we need to fix the z value...
    vec3f v = (unprojm * vec4f((float) mouseX, (float) mouseY, 1.0f / zdepth[mouseY * width + mouseX], 1.0f)).Demote();
/*
    float x = v[0];
    float y = v[1];
    float z;
    float length2 = x * x + y * y;
    if (length2 <= r * r / 2.0f) // inside the sphere
    {
        z = std::sqrt(r * r - length2);
    }
    else
    {
        z = (r * r / 2.0f) / std::sqrt(length2);
    }
*/
    return v;
}

void Rubik::RotateSwap(int group, int orien)
{
    int i = rotation_group[group][0];
    int j = rotation_group[group][1];
    int k = rotation_group[group][2];
    int l = rotation_group[group][3];

    // cubies from top leftmost corner to bottom right most corner must be indexed 0-7 after swapping
    // remember top to bottom, left to right and front to back

    switch (orien)
    {
    case N_X_AXIS:
    case Y_AXIS:
    case Z_AXIS:
    {
        std::cerr << "ccw" << std::endl;

        Cubie tmp1 = rubik_cube[i];
        Cubie tmp2 = rubik_cube[k];

        rubik_cube[i] = rubik_cube[j];
        rubik_cube[j] = rubik_cube[l];
        rubik_cube[k] = tmp1;
        rubik_cube[l] = tmp2;

        break;
    }
    case X_AXIS:
    case N_Y_AXIS:
    case N_Z_AXIS:
    {
        std::cerr << "cw" << std::endl;

        Cubie tmp1 = rubik_cube[i];
        Cubie tmp2 = rubik_cube[j];

        rubik_cube[i] = rubik_cube[k];
        rubik_cube[j] = tmp1;
        rubik_cube[k] = rubik_cube[l];
        rubik_cube[l] = tmp2;

        break;
    }
    }

    mat4f rotate;

    switch (orien)
    {
    case X_AXIS:   rotate = CreateRotationXMatrix4<float>(M_PI_2);  break;
    case N_X_AXIS: rotate = CreateRotationXMatrix4<float>(-M_PI_2); break;
    case Y_AXIS:   rotate = CreateRotationYMatrix4<float>(M_PI_2);  break;
    case N_Y_AXIS: rotate = CreateRotationYMatrix4<float>(-M_PI_2); break;
    case Z_AXIS:   rotate = CreateRotationZMatrix4<float>(M_PI_2);  break;
    case N_Z_AXIS: rotate = CreateRotationZMatrix4<float>(-M_PI_2); break;
    }

    // finally apply rotation to each cubie position
    rubik_cube[i].position = rotate * rubik_cube[i].position;
    rubik_cube[j].position = rotate * rubik_cube[j].position;
    rubik_cube[k].position = rotate * rubik_cube[k].position;
    rubik_cube[l].position = rotate * rubik_cube[l].position;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static Rubik app(SCREEN_WIDTH, SCREEN_HEIGHT);
    static bool bMousePressed = false;
    static bool bLeftButton = false;
    int mouseX, mouseY;

    switch (msg)
    {
    case WM_CREATE:
    {
        app.Create(hWnd);
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
    case WM_KEYDOWN:
    {
        if (wParam == 0x53) // s key
        {
            app.StartScramble();
        }
        break;
    }
    case WM_LBUTTONDOWN:
    {
        mouseX = GET_X_LPARAM(lParam); // client x
        mouseY = GET_Y_LPARAM(lParam); // client y

        app.HandleMousePress(mouseX, mouseY);
        bMousePressed = true;
        bLeftButton = true;

        break;
    }
    case WM_LBUTTONUP:
    {
        mouseX = GET_X_LPARAM(lParam);
        mouseY = GET_Y_LPARAM(lParam);

        app.HandleMouseRelease(mouseX, mouseY);
        bMousePressed = false;
        bLeftButton = false;

        break;
    }
    case WM_RBUTTONDOWN:
    {
        mouseX = GET_X_LPARAM(lParam);
        mouseY = GET_Y_LPARAM(lParam);

        app.HandleRightMouseButtonPress(mouseX, mouseY);
        bMousePressed = true;

        InvalidateRect(hWnd, NULL, FALSE);

        break;
    }
    case WM_RBUTTONUP:
    {
        mouseX = GET_X_LPARAM(lParam);
        mouseY = GET_Y_LPARAM(lParam);

        app.HandleRightMouseButtonRelease(mouseX, mouseY);
        bMousePressed = false;

        InvalidateRect(hWnd, NULL, FALSE);

        break;
    }
    case WM_MOUSEMOVE:
    {
        mouseX = GET_X_LPARAM(lParam);
        mouseY = GET_Y_LPARAM(lParam);

        if ((wParam & MK_LBUTTON) != 0 && bMousePressed) // the left mouse button is pressed
        {
            app.HandleMouseMotion(mouseX, mouseY);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        else if ((wParam & MK_RBUTTON) != 0 && bMousePressed)
        {
            app.HandleMouseMotionR(mouseX, mouseY);
            InvalidateRect(hWnd, NULL, FALSE);
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

    AdjustWindowRectEx(&rcClient, style, FALSE, 0); // change the third parameter to TRUE if you plan to add menubar

    hWnd = CreateWindow(szClassName,
        TEXT("Rubik's Cube"),
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
        MessageBox(NULL, TEXT("Window Creation Failed!"), TEXT("error!"),
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