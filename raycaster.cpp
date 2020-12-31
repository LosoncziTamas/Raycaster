#include <SDL2/SDL.h>

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

struct Vec2
{
    float x;
    float y;

    constexpr Vec2(float x, float y) : x(x), y(y) {}
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

constexpr const Vec2 FpsViewDimsInPixels = Vec2(480, 480);
constexpr const Vec2 WindowSize = Vec2(960, 480);
constexpr const Vec2 MapDimsInPixels = Vec2(480, 480);
constexpr const Vec2 MapDimsInTiles = Vec2(10, 10);
constexpr const Vec2 TileDimsInPixels = Vec2(MapDimsInPixels.x / MapDimsInTiles.x, MapDimsInPixels.y / MapDimsInTiles.y);

constexpr Vec2 TileToPixelPosition(Vec2 tileIndex, Vec2 tileDims, Vec2 offset = Vec2(0,0))
{
    return Vec2((tileIndex.x * tileDims.x) + offset.x, (tileIndex.y * tileDims.y) + offset.y);
}

constexpr Uint32 PackColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255)
{ 
    return ((alpha << 24) | (red << 16) | (green << 8) | blue);
}

const Uint32 White = PackColor(255, 255, 255);
const Uint32 Red = PackColor(128, 0, 0);
const Uint32 Green = PackColor(0, 128, 0);
const Uint32 Blue = PackColor(0, 0, 128);
const Uint32 Grey = PackColor(128, 128, 128);
const Uint32 Black = PackColor(0, 0, 0);

const float AngleToRadian = M_PI / 180.0f;
const float RadianToAngle = 180.0f / M_PI;
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
    .fov = 15
};

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
                // player.pixelPosition.x -= player.speed; 
            }
            if (e.key.keysym.sym == SDLK_RIGHT)
            {
                // player.pixelPosition.x += player.speed; 
            }
            if (e.key.keysym.sym == SDLK_UP)
            {
                player.pixelPosition.x = player.pixelPosition.x - cosf(player.facingAngle);
                player.pixelPosition.y = player.pixelPosition.y - sinf(player.facingAngle);
            }
            if (e.key.keysym.sym == SDLK_DOWN)
            {
                player.pixelPosition.x = player.pixelPosition.x + cosf(player.facingAngle);
                player.pixelPosition.y = player.pixelPosition.y + sinf(player.facingAngle);         
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

float Distance(Vec2 a, Vec2 b)
{
    return sqrtf(powf(a.x - b.x, 2) + powf(a.y - b.y, 2));
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
            Vec2 rayPixelPosition = Vec2(player.pixelPosition.x + cos * i, player.pixelPosition.y + sin * i);
            Vec2 tilePosition = PixelToTilePosition(buffer, rayPixelPosition.x, rayPixelPosition.y);
            TileType tile = GetTileValue(tilePosition);

            if (tile == _)
            {
                SetPixelColor(buffer, rayPixelPosition.x, rayPixelPosition.y, White);
            }
            else
            {
                auto *hitData = &hits->data[rayIndex];
                hitData->wasHit = true;
                hitData->distanceFromPlayer = Distance(player.pixelPosition, rayPixelPosition);
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

int main(int argc, char *argv[])
{

    auto angle1 = Angle(Vec2(0, 1), Vec2(0, 0));
    auto angle2 = Angle(Vec2(-1, 0), Vec2(0, 0));
    auto angle3 = Angle(Vec2(1, 0), Vec2(0, 0));
    auto angle4 = Angle(Vec2(-1, 0), Vec2(0, 0));


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

    auto *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WindowSize.x, WindowSize.y);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    const int bytesPerPixel = 4;

    ScreenBuffer buffer = {0};
    buffer.texture = texture;
    buffer.bytesPerPixel = bytesPerPixel;
    buffer.width = WindowSize.x;
    buffer.height = WindowSize.y;
    buffer.pitch = WindowSize.x * bytesPerPixel;
    buffer.memory = (Uint8 *) malloc(WindowSize.x * WindowSize.y * bytesPerPixel);

    RayHits hits = {0};

    done = false;

    while (!done)
    {
        DrawMap(buffer);
        ClearHits(&hits);
        DrawRays(buffer, &hits);
        DrawPlayer(buffer);
        DrawFpsView(buffer, &hits);
        Update(window, renderer, buffer);
    }

    SDL_Quit();
    return 0;
}