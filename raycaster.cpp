#include <SDL2/SDL.h>
#include "vec2.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

template<typename T, int N>
constexpr int ArrayCount(const T(&)[N])
{
    return N;
}

struct ScreenBuffer
{
    SDL_Texture *texture;
    int width;
    int height;
    int pitch;
    int bytesPerPixel;
    Uint8 *memory;
};
struct Player
{
    Vec2 pixelPosition;
    Vec2 dimensions;
    float facingAngle;
    float speed;
    float fov;
};

struct RayHits
{
    struct Data
    {
        float distanceFromPlayer;
        bool wasHit;
        Uint32 color;
    } 
    data[480];
};

struct Texture
{
    int width;
    int height;
    int bytesPerPixel;
    Uint8* data;
};

constexpr const Vec2 FpsViewDimsInPixels = Vec2(480, 480);
constexpr const Vec2 WindowSize = Vec2(960, 480);
constexpr const Vec2 MapDimsInPixels = Vec2(480, 480);
constexpr const Vec2 MapDimsInTiles = Vec2(10, 10);
constexpr const Vec2 TileDimsInPixels = Vec2(MapDimsInPixels.x / MapDimsInTiles.x, MapDimsInPixels.y / MapDimsInTiles.y);

constexpr Vec2 TileToPixelPosition(Vec2 tileIndex, Vec2 tileDims, Vec2 offset = Vec2(0,0))
{
    return Vec2((tileIndex.x * tileDims.x) + offset.x, (tileIndex.y * tileDims.y) + offset.y);
}

constexpr Uint32 PackColorABGR(Uint8 alpha, Uint8 blue, Uint8 green, Uint8 red)
{ 
    return ((alpha << 24) | (blue << 16) | (green << 8) | red);
}

void UnpackColorABGR(Uint32 color, Uint8 *alpha, Uint8 *blue, Uint8 *green, Uint8 *red)
{
    *red = color & 0x000000FF;
    *green = (color & 0x0000FF00) >> 8;
    *blue = (color & 0x00FF0000) >> 16;
    *alpha = (color & 0xFF000000) >> 24;
}

const Uint32 White = PackColorABGR(255, 255, 255, 255);
const Uint32 Red = PackColorABGR(255, 0, 0, 128);
const Uint32 Green = PackColorABGR(255, 0, 128, 0);
const Uint32 Blue = PackColorABGR(255, 128, 0, 0);
const Uint32 Grey = PackColorABGR(255, 128, 128, 128);
const Uint32 Black = PackColorABGR(255, 0, 0, 0);

const float AngleToRadian = M_PI / 180.0f;
const float RayLength = 300.0f;

enum TileType
{
    _ = 0,
    A = 1,
    B = 2,
    C = 3
};

const TileType Map[] = 
{
    A, A, A, A, A, A, A, A, A, A,
    A, _, _, _, B, _, _, _, _, A,
    A, _, _, _, B, _, _, _, _, A,
    A, _, _, _, C, _, _, _, _, A,
    A, _, B, B, B, B, _, _, _, A,
    A, _, _, _, _, B, B, B, _, A,
    A, _, _, _, _, _, _, B, _, A,
    A, _, _, _, _, _, _, B, _, A,
    A, B, _, _, _, _, _, _, _, A,
    A, A, A, A, A, A, A, A, A, A,
};

bool done;

Player player = 
{
    .pixelPosition = TileToPixelPosition(Vec2(2, 2), TileDimsInPixels),
    .dimensions = Vec2 (5, 5),
    .facingAngle = 90.0f,
    .speed = 2.0f,
    .fov = 45
};

void Update(SDL_Window *window, SDL_Renderer *renderer, ScreenBuffer buffer)
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_QUIT)
        {
            done = true;
            return;
        }

        if (e.type == SDL_KEYDOWN)
        {
            if (e.key.keysym.sym == SDLK_ESCAPE)
            {
                done = true;
            }
            if (e.key.keysym.sym == SDLK_s)
            {
                player.facingAngle -= player.speed;
            }
            if (e.key.keysym.sym == SDLK_a)
            {
                player.facingAngle += player.speed;
            }
            if (e.key.keysym.sym == SDLK_LEFT)
            {
                float direction = (player.facingAngle - 90.f) * AngleToRadian;
                Vec2 offset(cosf(direction), sinf(direction));
                player.pixelPosition += offset;
            }
            if (e.key.keysym.sym == SDLK_RIGHT)
            {
                float direction = (player.facingAngle + 90.f) * AngleToRadian;
                Vec2 offset(cosf(direction), sinf(direction));
                player.pixelPosition += offset;
            }
            if (e.key.keysym.sym == SDLK_UP)
            {
                float direction = player.facingAngle * AngleToRadian;
                Vec2 offset(cosf(direction), sinf(direction));
                player.pixelPosition += offset;
            }
            if (e.key.keysym.sym == SDLK_DOWN)
            {
                float direction = player.facingAngle * AngleToRadian;
                Vec2 offset(cosf(direction), sinf(direction));
                player.pixelPosition -= offset;       
            }
            return;
        }
    }

    SDL_UpdateTexture(buffer.texture, NULL, buffer.memory, buffer.pitch);
    SDL_RenderCopy(renderer, buffer.texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


inline void SetPixelColor(ScreenBuffer buffer, int x, int y, Uint32 color)
{
    Uint32* pixel = (Uint32*) (buffer.memory + (x + y * buffer.width) * buffer.bytesPerPixel);
    *pixel = color;
}

inline Uint32 GetPixelColorAt(ScreenBuffer buffer, Vec2 pixelPos)
{   
    int x = pixelPos.x;
    int y = pixelPos.y;
    Uint32* pixel = (Uint32*) (buffer.memory + (x + y * buffer.width) * buffer.bytesPerPixel);
    return *pixel;
}

inline void SetPixelColor(ScreenBuffer buffer, Vec2 pixelPosition, Uint32 color)
{
    SetPixelColor(buffer, pixelPosition.x, pixelPosition.y, color);
}

void FillTileWithColor(ScreenBuffer buffer, int tileX, int tileY, int width, int height, Uint32 color)
{
    for (int i = tileX * width; i < (tileX + 1) * width; ++i)
    {
        for (int j = tileY * height; j < (tileY + 1) * height; ++j)
        {
            SetPixelColor(buffer, i, j, color);
        }
    }
}

void DrawRect(ScreenBuffer buffer, Vec2 topLeft, Vec2 dimensions, Uint32 color)
{
    for (int i = 0; i < dimensions.x; ++i)
    {
        for (int j = 0; j < dimensions.y; ++j)
        {
            SetPixelColor(buffer, topLeft.x + i, topLeft.y + j, color);
        }
    }
}

void DrawMap(ScreenBuffer buffer)
{
    const int MaxTileY = MapDimsInTiles.y;
    const int MaxTileX = MapDimsInTiles.x;
    const int tileWidthToPixel = MapDimsInPixels.x / MapDimsInTiles.y;
    const int tileHeightToPixel = MapDimsInPixels.y / MapDimsInTiles.x;

    for (int y = 0; y < MaxTileY; ++y)
    {
        for (int x = 0; x < MaxTileX; ++x)
        {
            TileType tile = Map[x + y * MaxTileY];

            switch (tile)
            {
            case _:
                FillTileWithColor(buffer, x, y, tileWidthToPixel, tileHeightToPixel, Grey);
                break;
            case A:
                FillTileWithColor(buffer, x, y, tileWidthToPixel, tileHeightToPixel, Black);
                break;
            case B:
                FillTileWithColor(buffer, x, y, tileWidthToPixel, tileHeightToPixel, Red);
                break;
            case C:
                FillTileWithColor(buffer, x, y, tileWidthToPixel, tileHeightToPixel, Blue);
                break;
            default:
                break;
            }
        }
    }
}

Vec2 PixelToTilePosition(ScreenBuffer buffer, int x, int y)
{
    const int tileWidthToPixel = MapDimsInPixels.x / MapDimsInTiles.y;
    const int tileHeightToPixel = MapDimsInPixels.y / MapDimsInTiles.x;

    int tileX = x / tileWidthToPixel;
    int tileY = y / tileHeightToPixel;

    return Vec2(tileX, tileY);
}

TileType GetTileValue(Vec2 tilePosition)
{
    int tileIndex = tilePosition.x + tilePosition.y * MapDimsInTiles.y;
    return Map[tileIndex];
}

void DrawRays(ScreenBuffer buffer, RayHits* hits)
{
    int rayCount = FpsViewDimsInPixels.x;

    for (int rayIndex = 0; rayIndex < rayCount; ++rayIndex)
    {
        float rayScaler = (float)rayIndex / (float)rayCount;
        float angle = (player.fov * -0.5f) + (rayScaler * player.fov); 
        float cos = cosf((player.facingAngle + angle) * AngleToRadian);
        float sin = sinf((player.facingAngle + angle) * AngleToRadian);

        for (int i = 0; i < RayLength; i += 2)
        {
            Vec2 rayPixelPosition(player.pixelPosition.x + cos * i, player.pixelPosition.y + sin * i);
            Vec2 tilePosition = PixelToTilePosition(buffer, rayPixelPosition.x, rayPixelPosition.y);
            TileType tile = GetTileValue(tilePosition);

            if (tile == _)
            {
                SetPixelColor(buffer, rayPixelPosition.x, rayPixelPosition.y, White);
            }
            else
            {
                float distance = Distance(player.pixelPosition, rayPixelPosition);
                auto *hitData = &hits->data[rayIndex];
                hitData->wasHit = true;
                hitData->distanceFromPlayer = cosf((angle) * AngleToRadian) * distance;
                hitData->color = GetPixelColorAt(buffer, rayPixelPosition);
                break;
            }
        }
    }
}

void DrawPlayer(ScreenBuffer buffer)
{
    DrawRect(buffer, player.pixelPosition, player.dimensions, Black);
}

void DrawFpsView(ScreenBuffer buffer, RayHits *hits)
{
    DrawRect(buffer, Vec2(MapDimsInPixels.x, 0), FpsViewDimsInPixels, Grey);

    for (int i = 0; i < ArrayCount(hits->data); ++i)
    {
        auto ray = hits->data[i];
        if (ray.wasHit)
        {
            float lineSize = 1 - (ray.distanceFromPlayer / RayLength);
            int halfHeight = (buffer.height / 2);
            int lineTopY = halfHeight - (lineSize * halfHeight);
            int lineBottomY = halfHeight + (lineSize * halfHeight);
            
            for (int y = halfHeight; y > lineTopY; --y)
            {
                // float alphaScale =  1.0f - (float) lineTopY / y;
                // Uint8 alpha = (Uint8)(alphaScale * 255);

                SetPixelColor(buffer, Vec2(MapDimsInPixels.x + i, y), ray.color);
            }

            for (int y = halfHeight; y < lineBottomY; ++y)
            {
                // float alphaScale = 1.0f - (float) y / lineBottomY;
                // Uint8 alpha = (Uint8)(alphaScale * 255);

                SetPixelColor(buffer, Vec2(MapDimsInPixels.x + i, y), ray.color);
            }
        }
    }
}

void ClearHits(RayHits *hits)
{
    for (int hitIndex = 0; hitIndex < ArrayCount(hits->data); ++hitIndex)
    {
        auto* hit = hits->data + hitIndex;
        hit->wasHit = false;
        hit->distanceFromPlayer = 0;
        hit->color = Grey;
    }
}

void DrawTexture(ScreenBuffer buffer, Texture texture)
{
    for (int x = 0; x < texture.width; ++x)
    {
        for (int y = 0; y < texture.height; ++y)
        {
            Uint32* color = (Uint32*)(texture.data + (x + y * texture.width) * texture.bytesPerPixel);
            SetPixelColor(buffer, Vec2(x, y), *color);
        }
    }
}


int main(int argc, char *argv[])
{
    static_assert(ArrayCount(Map) == MapDimsInTiles.y * MapDimsInTiles.x, "Invalid array size.");
    
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init fail : %s\n", SDL_GetError());
        return 1;
    }

    auto *window = SDL_CreateWindow("Raycaster", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WindowSize.x, WindowSize.y, 0);
    if (!window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation fail : %s\n", SDL_GetError());
        return 1;
    }
    auto *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Render creation for surface fail : %s\n", SDL_GetError());
        return 1;
    }

    SDL_RenderClear(renderer);

    auto *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, WindowSize.x, WindowSize.y);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    const int bytesPerPixel = 4;

    ScreenBuffer buffer = {0};
    buffer.texture = texture;
    buffer.bytesPerPixel = bytesPerPixel;
    buffer.width = WindowSize.x;
    buffer.height = WindowSize.y;
    buffer.pitch = WindowSize.x * bytesPerPixel;
    buffer.memory = (Uint8 *) malloc(WindowSize.x * WindowSize.y * bytesPerPixel);

    int imgWidth;
    int imgHeight;
    int compsPerPixel;

    Texture imgTexture = {0};

    Uint8 *data = stbi_load("walltext.png", &imgWidth, &imgHeight, &compsPerPixel, STBI_rgb_alpha);
    if (data)
    {
        imgTexture.width = imgWidth;
        imgTexture.height = imgHeight;
        imgTexture.bytesPerPixel = compsPerPixel;
        imgTexture.data = data;
    }

    RayHits hits = {0};

    done = false;

    while (!done)
    {
        DrawMap(buffer);
        ClearHits(&hits);
        DrawRays(buffer, &hits);
        DrawPlayer(buffer);
        DrawFpsView(buffer, &hits);
        DrawTexture(buffer, imgTexture);
        Update(window, renderer, buffer);
    }

    SDL_Quit();
    return 0;
}