#ifndef _LINALG_H_
#define _LINALG_H_

#include <ostream>
#include <stdexcept>

#define _USE_MATH_DEFINES
#include <cmath>

template<typename T, size_t N>              class Vector;
template<typename T, size_t M, size_t N>    class Matrix;
template<typename T>                        class Quaternion;

template<typename T, size_t N>
class Vector
{
public:
    Vector() { _init_data(); }
    Vector(std::initializer_list<T> l);
    template<typename ...Args> Vector(Args... args) : Vector({args...}) {}
    Vector(const Vector& v);
    ~Vector() { delete[] data_; }

    Vector& operator=(const Vector& v);
    Vector& operator+=(const Vector& v);
    Vector& operator-=(const Vector& v);
    Vector& operator*=(T s);
    Vector& operator/=(T s);

    T operator[](int index) const;
    T& operator[](int index);

    Vector Unit() const;
    Vector Cross(const Vector& v) const;

    T Dot(const Vector& v) const;
    T Magnitude() const;

    // Find the component (or scalar projection) of v onto this vector
    T Component(const Vector& v) const;

    // Find the vector projection of v onto this vector
    Vector Project(const Vector& v) const;

    // Rotate this 2D vector about x axis in counterclockwise direction
    // Note that the angle must be in radians!
    Vector Rotate2D(T angle) const;

    Vector RotateX3D(T angle) const;
    Vector RotateY3D(T angle) const;
    Vector RotateZ3D(T angle) const;

    // Rotate this 3D vector about arbitrary axis and angle
    Vector Rotate3D(const Vector<T, 3>& axis, T angle) const;
    // Same as Rotate3D. Note that q must be a unit quaternion.
    Vector Rotate3D(const Quaternion<T>& q) const;

    size_t Dimensions() const { return N; }
private:
    T* data_;

    void _init_data();
};

template<typename T, size_t N>
Vector<T, N>::Vector(std::initializer_list<T> l)
{
    if (l.size() != N)
    {
        throw std::length_error("wrong number of arguments");
    }

    _init_data();

    int i = 0;

    for (auto elem : l)
    {
        data_[i++] = elem;
    }
}

template<typename T, size_t N>
Vector<T, N>::Vector(const Vector<T, N>& v)
{
    _init_data();
    *this = v;
}

template<typename T, size_t N>
Vector<T, N>& Vector<T, N>::operator=(const Vector<T, N>& v)
{
    for (int i = 0; i < N; ++i)
    {
        data_[i] = v[i];
    }

    return *this;
}

template<typename T, size_t N>
Vector<T, N>& Vector<T, N>::operator+=(const Vector<T, N>& v)
{
    for (int i = 0; i < N; ++i)
    {
        data_[i] += v[i];
    }

    return *this;
}

template<typename T, size_t N>
Vector<T, N>& Vector<T, N>::operator-=(const Vector<T, N>& v)
{
    for (int i = 0; i < N; ++i)
    {
        data_[i] -= v[i];
    }

    return *this;
}

template<typename T, size_t N>
Vector<T, N>& Vector<T, N>::operator*=(T s)
{
    for (int i = 0; i < N; ++i)
    {
        data_[i] *= s;
    }

    return *this;
}

template<typename T, size_t N>
Vector<T, N>& Vector<T, N>::operator/=(T s)
{
    if (s == 0)
    {
        throw std::logic_error("division by zero");
    }

    for (int i = 0; i < N; ++i)
    {
        data_[i] /= s;
    }

    return *this;
}

template<typename T, size_t N>
T Vector<T, N>::operator[](int index) const
{
    if (index < 0 || index >= N) throw std::out_of_range("index is out of bounds");
    else return data_[index];
}

template<typename T, size_t N>
T& Vector<T, N>::operator[](int index)
{
    if (index < 0 || index >= N) throw std::out_of_range("index is out of bounds");
    else return data_[index];
}

template<typename T, size_t N>
Vector<T, N> Vector<T, N>::Unit() const
{
    return *this / Magnitude();
}

template<typename T, size_t N>
Vector<T, N> Vector<T, N>::Cross(const Vector<T, N>& v) const
{
    // TODO calculate cross product for 7D vectors
    // see https://math.stackexchange.com/q/720813
    if (N != 3)
    {
        throw std::invalid_argument("Cross product is applicable to 3D vectors only");
    }

    Vector n;

    n[0] = data_[1] * v[2] - data_[2] * v[1];
    n[1] = data_[2] * v[0] - data_[0] * v[2];
    n[2] = data_[0] * v[1] - data_[1] * v[0];

    return n;
}

template<typename T, size_t N>
T Vector<T, N>::Dot(const Vector<T, N>& v) const
{
    T total = 0;

    for (int i = 0; i < N; ++i)
    {
        total += data_[i] * v[i];
    }

    return total;
}

template<typename T, size_t N>
T Vector<T, N>::Magnitude() const
{
    return std::sqrt(Dot(*this));
}

template<typename T, size_t N>
T Vector<T, N>::Component(const Vector<T, N>& v) const
{
    return Dot(v) / Magnitude();
}

template<typename T, size_t N>
Vector<T, N> Vector<T, N>::Project(const Vector<T, N>& v) const
{
    return Component(v) * Unit();
}

template<typename T, size_t N>
Vector<T, N> Vector<T, N>::Rotate2D(T angle) const
{
    if (N != 2)
    {
        throw std::invalid_argument("Rotate2D is applicable to 2D vectors only");
    }

    Matrix<T, 2, 2> R = {{std::cos(angle), -std::sin(angle)},
                         {std::sin(angle),  std::cos(angle)}};

    return R * (*this);
}

template<typename T, size_t N>
Vector<T, N> Vector<T, N>::RotateX3D(T angle) const
{
    if (N != 3)
    {
        throw std::invalid_argument("RotateX3D is applicable to 3D vectors only");
    }

    Matrix<T, 3, 3> RX = {{1,               0,                0},
                          {0, std::cos(angle), -std::sin(angle)},
                          {0, std::sin(angle),  std::cos(angle)}};

    return RX * (*this);
}

template<typename T, size_t N>
Vector<T, N> Vector<T, N>::RotateY3D(T angle) const
{
    if (N != 3)
    {
        throw std::invalid_argument("RotateY3D is applicable to 3D vectors only");
    }

    Matrix<T, 3, 3> RY = {{std::cos(angle),  0, std::sin(angle)},
                          {0,                1,               0},
                          {-std::sin(angle), 0, std::cos(angle)}};

    return RY * (*this);
}

template<typename T, size_t N>
Vector<T, N> Vector<T, N>::RotateZ3D(T angle) const
{
    if (N != 3)
    {
        throw std::invalid_argument("RotateZ3D is applicable to 3D vectors only");
    }

    Matrix<T, 3, 3> RZ = {{std::cos(angle), -std::sin(angle), 0},
                          {std::sin(angle), std::cos(angle),  0},
                          {0,                             0,  1}};

    return RZ * (*this);
}

template<typename T, size_t N>
Vector<T, N> Vector<T, N>::Rotate3D(const Vector<T, 3>& axis, T angle) const
{
    if (N != 3)
    {
        throw std::invalid_argument("Rotate3D is applicable to 3D vectors only");
    }

    return Rotate3D(Quaternion<T>(axis, angle));
}

template<typename T, size_t N>
Vector<T, N> Vector<T, N>::Rotate3D(const Quaternion<T>& q) const
{
    if (N != 3)
    {
        throw std::invalid_argument("Rotate3D is applicable to 3D vectors only");
    }

    Quaternion<T> p(0, *this); // pure quaternion for this vector

    p = q * p * q.Conjugate(); // conjugate and inverse are the same in this case

    return p.VectorComponent();
}

template<typename T, size_t N>
void Vector<T, N>::_init_data()
{
    data_ = new T[N];

    for (int i = 0; i < N; ++i)
    {
        data_[i] = 0;
    }
}

template<typename T, size_t N> Vector<T, N> operator+(const Vector<T, N>& v) { return v; }
template<typename T, size_t N> Vector<T, N> operator-(const Vector<T, N>& v) { return Vector<T, N>() - v; }

template<typename T, size_t N> Vector<T, N> operator+(Vector<T, N> l, const Vector<T, N>& r) { return l += r; }
template<typename T, size_t N> Vector<T, N> operator-(Vector<T, N> l, const Vector<T, N>& r) { return l -= r; }

template<typename T, typename U, size_t N> Vector<T, N> operator*(Vector<T, N> v, U s) { return v *= s; }
template<typename T, typename U, size_t N> Vector<T, N> operator*(U s, Vector<T, N> v) { return v *= s; }

template<typename T, typename U, size_t N> Vector<T, N> operator/(Vector<T, N> v, U s) { return v /= s; }

template<typename T, size_t N>
std::ostream& operator<<(std::ostream& os, const Vector<T, N>& v)
{
    os << '[';

    for (int i = 0; i < N; ++i)
    {
        os << v[i] << ((i == N - 1) ? "]" : ", ");
    }

    return os;
}

template<typename T, size_t M, size_t N>
class Matrix {
public:
    Matrix(bool identity = false) { _init_vecs(identity); }
    Matrix(const Vector<T, N> vecs);
    Matrix(std::initializer_list<std::initializer_list<T>> l);
    Matrix(const Matrix& m);
    ~Matrix() { delete[] vecs_; }

    Matrix& operator=(const Matrix& m);
    Matrix& operator+=(const Matrix& m);
    Matrix& operator-=(const Matrix& m);
    Matrix& operator*=(T c);
    Matrix& operator/=(T c);

    Vector<T, N> operator[](int row) const;
    Vector<T, N>& operator[](int row);

    Vector<T, M> operator*(const Vector<T, N>& b) const;
    template<size_t P> Matrix<T, M, P> operator*(const Matrix<T, N, P>& b) const;

    Matrix<T, N, M> Transpose() const;

    // TODO impl inverse, gaussian elimination, solve systems of eqns, determinant, etc...

    size_t Rows() const { return M; }
    size_t Columns() const { return N; }
private:
    Vector<T, N>* vecs_;

    void _init_vecs(bool identity);
};

template<typename T, size_t M, size_t N>
Matrix<T, M, N>::Matrix(const Vector<T, N> vecs)
{
    _init_vecs(false);

    for (int i = 0; i < M; ++i)
    {
        this->vecs_[i] = vecs[i];
    }
}

template<typename T, size_t M, size_t N>
Matrix<T, M, N>::Matrix(std::initializer_list<std::initializer_list<T>> l)
{
    if (l.size() != M)
    {
        throw std::out_of_range("row count does not match");
    }

    if (l.begin()->size() != N)
    {
        throw std::out_of_range("column count does not match");
    }

    _init_vecs(false);

    int i = 0, j = 0;

    for (auto row : l)
    {
        for (auto elem : row)
        {
            vecs_[i][j++] = elem;
        }

        i++;
        j = 0;
    }
}

template<typename T, size_t M, size_t N>
Matrix<T, M, N>::Matrix(const Matrix<T, M, N>& m)
{
    _init_vecs(false);
    *this = m;
}

template<typename T, size_t M, size_t N>
Matrix<T, M, N>& Matrix<T, M, N>::operator=(const Matrix<T, M, N>& m)
{
    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            vecs_[i][j] = m[i][j];
        }
    }
    return *this;
}

template<typename T, size_t M, size_t N>
Matrix<T, M, N>& Matrix<T, M, N>::operator+=(const Matrix<T, M, N>& m)
{
    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            vecs_[i][j] += m[i][j];
        }
    }
    return *this;
}

template<typename T, size_t M, size_t N>
Matrix<T, M, N>& Matrix<T, M, N>::operator-=(const Matrix<T, M, N>& m)
{
    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            vecs_[i][j] -= m[i][j];
        }
    }
    return *this;
}

template<typename T, size_t M, size_t N>
Matrix<T, M, N>& Matrix<T, M, N>::operator*=(T c)
{
    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            vecs_[i][j] *= c;
        }
    }
    return *this;
}

template<typename T, size_t M, size_t N>
Matrix<T, M, N>& Matrix<T, M, N>::operator/=(T c)
{
    if (c == 0)
    {
        throw std::logic_error("division by zero");
    }

    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            vecs_[i][j] /= c;
        }
    }
    return *this;
}

template<typename T, size_t M, size_t N>
Vector<T, N> Matrix<T, M, N>::operator[](int row) const
{
    if (row < 0 || row >= M)
    {
        throw std::out_of_range("const Matrix subscript out of bounds");
    }
    return vecs_[row];
}

template<typename T, size_t M, size_t N>
Vector<T, N>& Matrix<T, M, N>::operator[](int row)
{
    if (row < 0 || row >= M)
    {
        throw std::out_of_range("const Matrix subscript out of bounds");
    }
    return vecs_[row];
}

template<typename T, size_t M, size_t N>
Vector<T, M> Matrix<T, M, N>::operator*(const Vector<T, N>& b) const
{
    Vector<T, M> c;

    for (int i = 0; i < M; ++i)
    {
        c[i] = vecs_[i].Dot(b);
    }

    return c;
}

template<typename T, size_t M, size_t N> template <size_t P>
Matrix<T, M, P> Matrix<T, M, N>::operator*(const Matrix<T, N, P>& b) const
{
    Matrix<T, M, P> c;

    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < P; ++j)
        {
            for (int k = 0; k < N; ++k)
            {
                c[i][j] += vecs_[i][k] * b[k][j];
            }
        }
    }

    return c;
}

template<typename T, size_t M, size_t N>
Matrix<T, N, M> Matrix<T, M, N>::Transpose() const
{
    Matrix<T, N, M> t;

    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            t[j][i] = vecs_[i][j];
        }
    }

    return t;
}

template<typename T, size_t M, size_t N>
void Matrix<T, M, N>::_init_vecs(bool identity)
{
    vecs_ = new Vector<T, N>[M];

    for (int i = 0; i < M; ++i)
    {
        for (int j = 0; j < N; ++j)
        {
            if (identity && i == j)
            {
                vecs_[i][j] = 1;
            }
        }
    }
}

template<typename T, size_t M, size_t N> Matrix<T, M, N> operator+(const Matrix<T, M, N>& m) { return m; }
template<typename T, size_t M, size_t N> Matrix<T, M, N> operator-(const Matrix<T, M, N>& m) { return Matrix<T, M, N>() - m; }

template<typename T, size_t M, size_t N> Matrix<T, M, N> operator+(const Matrix<T, M, N>& a, const Matrix<T, M, N>& b) { return a += b; }
template<typename T, size_t M, size_t N> Matrix<T, M, N> operator-(const Matrix<T, M, N>& a, const Matrix<T, M, N>& b) { return a -= b; }

template<typename T, typename U, size_t M, size_t N> Matrix<T, M, N> operator*(Matrix<T, M, N> m, U c) { return m *= c; }
template<typename T, typename U, size_t M, size_t N> Matrix<T, M, N> operator*(U c, Matrix<T, M, N> m) { return m *= c; }

template<typename T, typename U, size_t M, size_t N> Matrix<T, M, N> operator/(Matrix<T, M, N> m, U c) { return m /= c; }

template<typename T, size_t M, size_t N>
std::ostream& operator<<(std::ostream& os, const Matrix<T, M, N>& m)
{
    os << '[';

    for (int i = 0; i < M; ++i)
    {
        os << m[i] << ((i == M - 1) ? "]" : ", ");
    }

    return os;
}

template<typename T>
class Quaternion
{
public:
    Quaternion(bool unit = false) : s_(unit) {}
    Quaternion(T s, const Vector<T, 3>& v) : s_(s), v_(v) {}
    Quaternion(const Vector<T, 3>& axis, T angle);
    Quaternion(const Quaternion& q) { *this = q; }
    ~Quaternion() {}

    Quaternion& operator=(const Quaternion& q);
    Quaternion& operator+=(const Quaternion& q);
    Quaternion& operator-=(const Quaternion& q);
    Quaternion& operator*=(T r);
    Quaternion& operator*=(const Quaternion& q);
    Quaternion& operator/=(T r);
    Quaternion& operator/=(const Quaternion& q);

    T operator[](int index) const;
    T& operator[](int index);

    Quaternion Conjugate() const;
    Quaternion Inverse() const;
    Quaternion Unit() const;

    // TODO impl convert to matrix?

    T InnerProduct(const Quaternion& q) const;
    T Magnitude() const;

    T ScalarComponent() const { return s_; }
    Vector<T, 3> VectorComponent() const { return v_; }
private:
    T s_;
    Vector<T, 3> v_;
};

template<typename T>
Quaternion<T>::Quaternion(const Vector<T, 3>& axis, T angle)
{
    s_ = std::cos(angle / 2);
    v_ = axis.Unit() * std::sin(angle / 2);
}

template<typename T>
Quaternion<T>& Quaternion<T>::operator=(const Quaternion<T>& q)
{
    s_ = q.s_;
    v_ = q.v_;
    return *this;
}

template<typename T>
Quaternion<T>& Quaternion<T>::operator+=(const Quaternion<T>& q)
{
    s_ += q.s_;
    v_ += q.v_;
    return *this;
}

template<typename T>
Quaternion<T>& Quaternion<T>::operator-=(const Quaternion<T>& q)
{
    s_ -= q.s_;
    v_ -= q.v_;
    return *this;
}

template<typename T>
Quaternion<T>& Quaternion<T>::operator*=(T r)
{
    s_ *= r;
    v_ *= r;
    return *this;
}

template<typename T>
Quaternion<T>& Quaternion<T>::operator*=(const Quaternion<T>& q)
{
    T s = s_ * q.s_ - v_.Dot(q.v_);
    Vector<T, 3> v = s_ * q.v_ + q.s_ * v_ + v_.Cross(q.v_);

    s_ = s;
    v_ = v;

    return *this;
}

template<typename T>
Quaternion<T>& Quaternion<T>::operator/=(T r)
{
    if (r == 0)
    {
        throw std::logic_error("division by zero");
    }

    s_ /= r;
    v_ /= r;
    return *this;
}

template<typename T>
Quaternion<T>& Quaternion<T>::operator/=(const Quaternion<T>& q)
{
    return *this * Inverse();
}

template<typename T>
T Quaternion<T>::operator[](int index) const
{
    switch (index)
    {
    case 0: return s_;
    case 1: return v_[0];
    case 2: return v_[1];
    case 3: return v_[2];
    default: throw std::out_of_range("index is out of bounds");
    }
}

template<typename T>
T& Quaternion<T>::operator[](int index)
{
    switch (index)
    {
    case 0: return s_;
    case 1: return v_[0];
    case 2: return v_[1];
    case 3: return v_[2];
    default: throw std::out_of_range("index is out of bounds");
    }
}

template<typename T>
Quaternion<T> Quaternion<T>::Conjugate() const
{
    return Quaternion<T>(s_, -v_);
}

template<typename T>
Quaternion<T> Quaternion<T>::Inverse() const
{
    return Conjugate() / InnerProduct(*this);
}

template<typename T>
Quaternion<T> Quaternion<T>::Unit() const
{
    return *this / Magnitude();
}

template<typename T>
T Quaternion<T>::InnerProduct(const Quaternion<T>& q) const
{
    return s_ * q.s_ + v_.Dot(q.v_);
}

template<typename T>
T Quaternion<T>::Magnitude() const
{
    return std::sqrt(InnerProduct(*this));
}

template<typename T> Quaternion<T> operator+(const Quaternion<T>& q) { return q; }
template<typename T> Quaternion<T> operator-(const Quaternion<T>& q) { return Quaternion<T>() - q; }

template<typename T> Quaternion<T> operator+(Quaternion<T> l, const Quaternion<T>& r) { return l += r; }
template<typename T> Quaternion<T> operator-(Quaternion<T> l, const Quaternion<T>& r) { return l -= r; }
template<typename T> Quaternion<T> operator*(Quaternion<T> l, const Quaternion<T>& r) { return l *= r; }

template<typename T, typename U> Quaternion<T> operator*(Quaternion<T> q, U s) { return q *= s; }
template<typename T, typename U> Quaternion<T> operator*(U s, Quaternion<T> q) { return q *= s; }

template<typename T, typename U> Quaternion<T> operator/(Quaternion<T> q, U s) { return q /= s; }

template<typename T>
std::ostream& operator<<(std::ostream& os, const Quaternion<T>& q)
{
    os << '[' << q.ScalarComponent() << ", " << q.VectorComponent() << ']';
    return os;
}

#endif /* _LINALG_H_ */