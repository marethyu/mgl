#include <iostream>

#include "linalg.h"

int main()
{
    Matrix<int, 3, 3> A = {{1, 2, 1}, {0, 1, 0}, {2, 3, 4}};
    Matrix<int, 3, 2> B = {{2, 5}, {6, 7}, {1, 8}};
    Matrix<int, 3, 2> C = A * B;

    C = 2 * C;

    std::cout << C << std::endl;
    std::cout << C.Transpose() << std::endl;

    C[1][1] = 4;

    std::cout << C << std::endl;

    Matrix<int, 3, 4> D = {{1, 2, 1, 1}, {0, 1, 0, 1}, {2, 3, 4, 1}};
    Vector<int, 4> v = {2, 6, 1, 1};

    Vector<int, 3> result = D * v;

    std::cout << result << std::endl;

    Matrix<double, 3, 3> id(true);

    std::cout << id << std::endl;

    id /= 0.0;

    std::cout << id << std::endl;

    return 0;
}