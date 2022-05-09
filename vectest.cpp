#include <iostream>

#include "linalg.h"

using vec3 = Vector<int, 3>;

int main()
{
    vec3 z;
    std::cout << "z = " << z << '\n';

    auto a = vec3(1, 2, 3);
    auto b = vec3(4, 5, 6);

    std::cout << "a = " << a << '\n';
    std::cout << "b = " << b << '\n';
    std::cout << "a + b = " << a + b << '\n';

    std::cout << "a[0]=" << a[0] << '\n';
    std::cout << "a[1]=" << a[1] << '\n';
    std::cout << "a[2]=" << a[2] << '\n';

    a[1] = 10;
    a = a + b * 2;

    std::cout << "a[0]=" << a[0] << '\n';
    std::cout << "a[1]=" << a[1] << '\n';
    std::cout << "a[2]=" << a[2] << '\n';

    auto c = 2 * b;
    std::cout << "c = " << c << '\n';
  
    auto d = b * 2;
    std::cout << "d = " << d << '\n';
  
    auto e = b / 2;
    std::cout << "e = " << e << '\n';

    std::cout << "a.norm() = " << a.Magnitude() << '\n';
    std::cout << "a.unit() = " << a.Unit() << '\n';
    std::cout << "a.unit().norm() = " << a.Unit().Magnitude() << '\n';

    std::cout << "[4, 8, 10] . [9, 2, 7] = " << vec3({4, 8, 10}).Dot(vec3({9, 2, 7})) << '\n';

    std::cout << "[1, 0, 0] x [0, 1, 0] = " << vec3({1, 0, 0}).Cross(vec3({0, 1, 0})) << '\n';
    std::cout << "[2, 3, 4] x [5, 6, 7] = " << vec3({2, 3, 4}).Cross(vec3({5, 6, 7})) << '\n';

    vec3 x = {1, 1, 1};
    vec3 y = {2, 2, 2};

    vec3 total = x + y + (2 * x);

    std::cout << total << std::endl;

    Vector<float, 2> r = {1.0, 0.0};

    r = r.Rotate2D(M_PI / 4.0); // 45 degree rotation about x-axis

    Vector<float, 3> p = {0.0, 1.0, 0.0};
    Vector<float, 3> axis = {1.0, 0.0, 0.0}; // rotation axis: x-axis

    p = p.Rotate3D(axis, M_PI / 2.0); // 90 degree rotation about x-axis

    std::cout << "Rotated r: " << r << std::endl;
    std::cout << "Rotated p: " << p << std::endl;

    auto f = Vector<int, 4>(1, 2, 3, 4);

    std::cout << f << std::endl;

    return 0;
}