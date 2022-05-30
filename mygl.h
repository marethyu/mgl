#ifndef _MY_GL_H_
#define _MY_GL_H_

#include <vector>
#include <algorithm>
#include <limits>

#include "linalg.h"

namespace mygl
{
    struct Colour
    {
        uint32_t argb;

        Colour()
        {}

        Colour(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
        {
            argb = (a << 24) | (r << 16) | (g << 8) | b;
        }
    };

    const int MAXV = 50;
    const int MAXTRI = 101;
    const int MAXT = 101;

/*
    When you describe a 3D model, it is more convenient to write coordinates relative global origin (0, 0, 0) (see below for reference), don't worry about any linear transformations

    Coordinate system

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
    struct Triangle
    {
        bool filled;
        Colour colour;
        int vertex[3]; // index to vertex array in model (saving vertex itself is wasteful)
    };

    struct Model
    {
        int nvert;
        int ntrig;

        vec4d vertex[MAXV];
        Triangle triangle[MAXTRI];
    };

    const Colour RED(255, 0, 0, 255);
    const Colour ORANGE(255, 127, 0, 255);
    const Colour YELLOW(255, 255, 0, 255);
    const Colour GREEN(0, 255, 0, 255);
    const Colour BLUE(0, 0, 255, 255);
    const Colour INDIGO(75, 0, 130, 255);
    const Colour VIOLET(148, 0, 211, 255);

    const float ZMAX = std::numeric_limits<float>::max();

    // Platform indepentent base class for programs that use 3D graphics
    class RendererBase3D
    {
    public:
        RendererBase3D(int width, int height);
        ~RendererBase3D();

        // Must be overriden
        virtual void Init() = 0;
        virtual void Update() = 0;
        virtual void Render() = 0;
    protected:
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
        void DrawWireframeTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, const Colour& colour);
        void DrawLine(float x1, float y1, float z1, float x2, float y2, float z2, const Colour& colour);

        void ClearScreen();
        bool WithinBounds(float x, float y); // for clipping
        bool Equalf(float f1, float f2);
        void Swap(float& x1, float& y1, float& z1, float& x2, float& y2, float& z2);
    };

    RendererBase3D::RendererBase3D(int width, int height)
      : width(width), height(height), pixels(width * height), zdepth(width * height)
    {
        
    }

    RendererBase3D::~RendererBase3D()
    {}

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

    void RendererBase3D::DrawWireframeTriangle(float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, const Colour& colour)
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
                if (!WithinBounds(x, y))
                {
                    continue;
                }

                int offset = x * width + y;
                float depth = z; // TODO 1/z

                if (zdepth[offset] > depth)
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
                if (!WithinBounds(x, y))
                {
                    continue;
                }

                int offset = y * width + x;
                float depth = z; // TODO 1/z

                if (zdepth[offset] > depth)
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

    void RendererBase3D::ClearScreen()
    {
        std::fill(zdepth.begin(), zdepth.end(), ZMAX);
        std::fill(pixels.begin(), pixels.end(), 0);
    }

    bool RendererBase3D::WithinBounds(float x, float y)
    {
        return x >= 0.0 && x < width &&
               y >= 0.0 && y < height;
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
}

#endif /* _MY_GL_H_ */