#ifndef VEC2_H
#define VEC2_H

const float RadianToAngle = 180.0f / M_PI;

struct Vec2
{
    float x;
    float y;

    constexpr Vec2(float x, float y) : x(x), y(y) {}

    Vec2& operator*=(float real)
    {
        this->x *= real;
        this->y *= real;
        return *this;
    }

    Vec2& operator+=(Vec2 other)
    {
        this->x += other.x;
        this->y += other.y;
        return *this;
    }

    Vec2& operator-=(Vec2 other)
    {
        this->x -= other.x;
        this->y -= other.y;
        return *this;
    }
};

Vec2 operator+(Vec2 lhs, Vec2 rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

Vec2 operator-(Vec2 lhs, Vec2 rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

Vec2 operator*(Vec2 vec, float real)
{
    vec.x *= real;
    vec.y *= real;
    return vec;
}

Vec2 operator*(float real, Vec2 vec)
{
    return vec * real;
}

inline float DotProduct(Vec2 a, Vec2 b)
{
    auto result = a.x * b.x + a.y * b.y;
    return result;
}

inline float Magnitude(Vec2 v)
{
    auto result = sqrtf(DotProduct(v, v));
    return result;
}

inline Vec2 Normalize(Vec2 v)
{
    auto length = Magnitude(v);
    if (length == 0) 
    {
        return Vec2(0, 0);
    }
    Vec2 result = Vec2(v.x / length, v.y / length);
    return result;
}

float Angle(Vec2 a, Vec2 b)
{
    auto normalizedA = Normalize(a);
    auto normalizedB = Normalize(b);
    auto result = acosf(DotProduct(normalizedA, normalizedB));
    return result * RadianToAngle;
}

float Distance(Vec2 a, Vec2 b)
{
    return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2));
}

#endif