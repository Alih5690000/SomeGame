#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#define ACTIVE_CHECK() if (!active) return;

SDL_FRect mouseRect={0,0,10,10};

template <typename T>
struct Vec2{
    T x,y;
};
using Vec2f=Vec2<float>;

Vec2f GetSpeed(SDL_FRect& object, const SDL_FPoint& destination, float speed)
{
    float centerX = object.x + object.w * 0.5f;
    float centerY = object.y + object.h * 0.5f;

    float dx = destination.x - centerX;
    float dy = destination.y - centerY;

    float distance = sqrtf(dx * dx + dy * dy);

    if (distance <= 0.001f)
        return {0.f, 0.f};

    dx /= distance;
    dy /= distance;

    dx *= speed;
    dy *= speed;

    return {dx, dy};
}

