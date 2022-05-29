#include <iostream>

#include "linalg.h"

int main()
{
    vec3i z;
    std::cout << "z = " << z << '\n';

    auto a = vec3i(1, 2, 3);
    auto b = vec3i(4, 5, 6);

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

    std::cout << "a.Magnitude() = " << a.Magnitude() << '\n';
    std::cout << "a.Unit() = " << a.Unit() << '\n';
    std::cout << "a.Unit().Magnitude() = " << a.Unit().Magnitude() << '\n';

    auto b2 = vec3f(3.0f, 4.0f, 5.0f);
    std::cout << "b2.Unit() = " << b2.Unit() << '\n';

    std::cout << "[4, 8, 10] . [9, 2, 7] = " << vec3i({4, 8, 10}).Dot(vec3i({9, 2, 7})) << '\n';

    std::cout << "[1, 0, 0] x [0, 1, 0] = " << CrossProduct(vec3i({1, 0, 0}), vec3i({0, 1, 0})) << '\n';
    std::cout << "[2, 3, 4] x [5, 6, 7] = " << CrossProduct(vec3i({2, 3, 4}), vec3i({5, 6, 7})) << '\n';

    vec3i x = {1, 1, 1};
    vec3i y = {2, 2, 2};

    vec3i total = x + y + (2 * x);

    std::cout << total << std::endl;

    vec2f r = {1.0, 0.0};
    mat2f rot = CreateRotationMatrix2<float>(M_PI / 4.0); // 45 degree rotation about x-axis

    std::cerr << rot[0][1] << std::endl;
    r = rot * r;

    vec3f p = {0.0, 1.0, 0.0};
    vec3f axis = {1.0, 0.0, 0.0}; // rotation axis: x-axis
    Quaternion<float> q(axis, M_PI / 2.0); // 90 degree rotation about x-axis

    p = Rotate3D(p, q);

    std::cout << "Rotated r: " << r << std::endl;
    std::cout << "Rotated p: " << p << std::endl;

    auto f = Vector<int, 4>(1, 2, 3, 4);

    std::cout << f << std::endl;

    return 0;
}