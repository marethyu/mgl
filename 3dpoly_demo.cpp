/* g++ 3dpoly_demo.cpp -o 3dpoly_demo -std=c++14 -lSDL2 */

/*
TODO:
- test real models from wavefront .obj file
- should i change trackball radius when zooming?
- coloured faces
*/

#include <algorithm>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "linalg.h"

using vec2 = Vector<double, 2>;
using vec3 = Vector<double, 3>;
using mat2x2 = Matrix<double, 2, 2>;
using mat2x3 = Matrix<double, 2, 3>;

/*
Coordinate system:

    y
    |
    |
    +---x
   /
  z

 x - right
 y - up
 z - out the screen
*/

const int canvas_width = 600;
const int canvas_height = 600;

const int centerx = (canvas_width - 1) / 2;
const int centery = (canvas_height - 1) / 2;

const int MAXV = 50;
const int MAXE = 100;

struct WireframePolygon
{
    int vertexes;
    int edges;

    vec3 vertex[MAXV];
    int edge[MAXE][2];
};

/*
Triangular prism

        +5
       / \
      /   \
     +4   +6
    +2    /
   /  \  /
  +1---+3

*/
const WireframePolygon TriangularPrism = {
    6,
    9,
    {
        vec3(-0.5, -0.5, -0.1), // 1
        vec3(0.0, 0.8, -0.1),   // 2
        vec3(0.5, -0.5, -0.1),  // 3
        vec3(-0.5, -0.5, 0.1),  // 4
        vec3(0.0, 0.8, 0.1),    // 5
        vec3(0.5, -0.5, 0.1),   // 6
    },
    {
        {0, 1},
        {1, 2},
        {2, 0},
        {3, 4},
        {4, 5},
        {5, 3},
        {0, 3},
        {1, 4},
        {2, 5},
    }
};

/*
Cube

    +7-------+8
   /         /|
 +6--------+5 |
  |         | |
  | +2      |+3
  |         |/
 +1--------+4

*/
const WireframePolygon Cube = {
    8,
    12,
    {
        vec3(-0.5, -0.5, -0.5), // 1
        vec3(-0.5, -0.5, 0.5),  // 2
        vec3(0.5, -0.5, 0.5),   // 3
        vec3(0.5, -0.5, -0.5),  // 4
        vec3(0.5, 0.5, -0.5),   // 5
        vec3(-0.5, 0.5, -0.5),  // 6
        vec3(-0.5, 0.5, 0.5),   // 7
        vec3(0.5, 0.5, 0.5)     // 8
    },
    {
        {0, 1},
        {1, 2},
        {2, 3},
        {3, 0},
        {3, 4},
        {4, 5},
        {5, 0},
        {5, 6},
        {6, 1},
        {6, 7},
        {7, 2},
        {7, 4}
    }
};

const mat2x3 project2D = {{200, 0,     0},
                          {0,   200,   0}};

const vec3 xaxis = {1, 0, 0};
const vec3 yaxis = {0, 1, 0};
const vec3 zaxis = {0, 0, 1};

/* map s from [a1...a2] to [b1...b2] */
inline double map(double s, double a1, double a2, double b1, double b2) { return b1 + (s - a1) * (b2 - b1) / (a2 - a1); }

inline int mapX(double cx) { return map(cx, -centerx, centery, 0, canvas_width - 1); }
inline int mapY(double cy) { return map(cy, centery, -centery, 0, canvas_height - 1); }

vec3 project(int mx, int my)
{
    const double r = 1.0;

    double x = map(mx, 0, canvas_width - 1, -1, 1);
    double y = map(my, 0, canvas_height - 1, 1, -1);
    double z;

    if (x * x + y * y <= r * r / 2.0)
    {
        z = std::sqrt(r - x * x - y * y);
    }
    else
    {
        z = (r * r / 2.0) / std::sqrt(x * x + y * y);
    }

    return vec3(x, y, z);
}

int main (int argc, char** argv)
{
    SDL_Window* window = SDL_CreateWindow
    (
        "3D Polygon",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        canvas_width,
        canvas_height,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Event event;

    bool quit = false;

#ifdef USE_CUBE
    const int vertexes = Cube.vertexes;
    const int edges = Cube.edges;

    vec3 vertex[vertexes];
    int edge[edges][2];

    std::copy(Cube.vertex, Cube.vertex + vertexes, vertex);
    std::copy(&Cube.edge[0][0], &Cube.edge[0][0] + edges * 2, &edge[0][0]);
#else
    const int vertexes = TriangularPrism.vertexes;
    const int edges = TriangularPrism.edges;

    vec3 vertex[vertexes];
    int edge[edges][2];

    std::copy(TriangularPrism.vertex, TriangularPrism.vertex + vertexes, vertex);
    std::copy(&TriangularPrism.edge[0][0], &TriangularPrism.edge[0][0] + edges * 2, &edge[0][0]);
#endif
    double angle = 0.0; // for continuous counterclockwise rotation about y-axis
    const double dAngle = 0.01;

    bool mousepressed = false;

    vec3 p, q, n;
    double theta;

    Quaternion<double> currentQ(true); // unit quaternion
    Quaternion<double> lastQ(zaxis, M_PI / 4.0); // the polygon is initially rotated 45 degree counterclockwise about z-axis

    double zoomFactor = 1.0;

    while (!quit)
    {
        if (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
            {
                quit = true;
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                mousepressed = true;

                p = project(event.motion.x, event.motion.y);

                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                mousepressed = false;

                lastQ = currentQ * lastQ;
                currentQ = Quaternion<double>(true);

                break;
            }
            case SDL_MOUSEMOTION:
            {
                if (mousepressed)
                {
                    q = project(event.motion.x, event.motion.y);

                    n = p.Cross(q);
                    theta = std::acos(p.Dot(q) / (p.Magnitude() * q.Magnitude()));

                    currentQ = Quaternion<double>(n, theta);
                }
                break;
            }
            case SDL_KEYDOWN:
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_z:
                {
                    zoomFactor += 0.01;
                    if (zoomFactor > 1.5) zoomFactor = 1.5;
                    break;
                }
                case SDLK_x:
                {
                    zoomFactor -= 0.01;
                    if (zoomFactor < 0.5) zoomFactor = 0.5;
                    break;
                }
                }
                break;
            }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        vec2 proj[vertexes];
        int idx = 0;

        for (int i = 0; i < vertexes; ++i)
        {
            Quaternion<double> rotatey(yaxis, angle);
            Quaternion<double> rotation = currentQ * lastQ * rotatey; // rotations can be composed by simply multiplying quaternions!

            vec2 projected = project2D * vertex[i].Rotate3D(rotation);
            mat2x2 zoom = {{zoomFactor, 0}, {0, zoomFactor}};

            proj[idx++] = zoom * projected;
        }

        for (int i = 0; i < edges; ++i)
        {
            vec2 v1 = proj[edge[i][0]];
            vec2 v2 = proj[edge[i][1]];
            SDL_RenderDrawLine(renderer, mapX(v1[0]), mapY(v1[1]), mapX(v2[0]), mapY(v2[1]));
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(20);

        angle += dAngle;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;
}