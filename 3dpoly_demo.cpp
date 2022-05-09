/* g++ 3dpoly_demo.cpp -o 3dpoly_demo -std=c++14 -lSDL2 */

/*
TODO:
- verify whether rotations are correct
- test real models from wavefront .obj file
- zoom in/out
- mouse kontrol (trackball control)
- coloured faces
*/

#include <algorithm>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "linalg.h"

using vec2 = Vector<double, 2>;
using vec3 = Vector<double, 3>;
using mat2x3 = Matrix<double, 2, 3>;

/*
Coordinate system:

  y  z
  | /
  |/
  +---x

 x - right
 y - up
 z - into the screen
*/

const int canvas_width = 600;
const int canvas_height = 600;

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
const vec3 xyzaxis = xaxis + yaxis + zaxis;

inline int mapX(double cx) { return canvas_width / 2 + (int) cx; }
inline int mapY(double cy) { return canvas_height / 2 - (int) cy; }

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

    const int vertexes = Cube.vertexes;
    const int edges = Cube.edges;

    vec3 vertex[vertexes];
    int edge[edges][2];

    std::copy(Cube.vertex, Cube.vertex + vertexes, vertex);
    std::copy(&Cube.edge[0][0], &Cube.edge[0][0] + edges * 2, &edge[0][0]);

    double dAngle = 0.03;

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
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        vec2 proj[vertexes];
        int idx = 0;

        for (int i = 0; i < vertexes; ++i)
        {
            vec2 projected = project2D * vertex[i];
            SDL_RenderDrawPoint(renderer, mapX(projected[0]), mapY(projected[1]));
            proj[idx++] = projected;

            // update using rotation matrixes
            // vertex[i] = vertex[i].RotateY3D(dAngle).RotateX3D(dAngle).RotateZ3D(dAngle);

            // update using quaternions
            vertex[i] = vertex[i].Rotate3D(yaxis, dAngle).Rotate3D(xaxis, dAngle).Rotate3D(zaxis, dAngle);
            // vertex[i] = vertex[i].Rotate3D(xyzaxis, dAngle);
        }

        for (int i = 0; i < edges; ++i)
        {
            vec2 v1 = proj[edge[i][0]];
            vec2 v2 = proj[edge[i][1]];
            SDL_RenderDrawLine(renderer, mapX(v1[0]), mapY(v1[1]), mapX(v2[0]), mapY(v2[1]));
        }

        SDL_RenderPresent(renderer);

        SDL_Delay(20);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return EXIT_SUCCESS;

    return 0;
}