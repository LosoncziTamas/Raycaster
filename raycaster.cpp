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

struct IntVec2
{
    int x;
    int y;
};

struct Player
{
    IntVec2 pixelPosition;
    IntVec2 dimensions;
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
    } 
    data[480];
};

constexpr Uint32 PackColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255)
{ 
    return ((alpha << 24) | (red << 16) | (green << 8) | blue);
}

constexpr const IntVec2 MapDimsInTiles = IntVec2 {10, 10};

const Uint32 White = PackColor(255, 255, 255);
const Uint32 Red = PackColor(128, 0, 0);
const Uint32 Grey = PackColor(128, 128, 128);
const Uint32 Black = PackColor(0, 0, 0);
const float AngleToRadian = M_PI / 180.0f;
const float RayLength = 300.0f;

const IntVec2 FpsViewDimsInPixels = IntVec2 {480, 480};
const IntVec2 WindowSize = IntVec2 {960, 480};
const IntVec2 MapDimsInPixels = IntVec2 {480, 480};

bool done;

Player player = 
{
    .pixelPosition = IntVec2 {21, 20},
    .dimensions = IntVec2 {5, 5},
    .facingAngle = 90.0f,
    .speed = 2.0f,
    .fov = 15
};

int map[] = 
{
    1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 0, 1, 1, 1, 1, 1,
    1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 0, 0, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
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
                player.pixelPosition.x -= player.speed; 
            }
            if (e.key.keysym.sym == SDLK_RIGHT)
            {
                player.pixelPosition.x += player.speed; 
            }
            if (e.key.keysym.sym == SDLK_UP)
            {
                player.pixelPosition.y -= player.speed;
            }
            if (e.key.keysym.sym == SDLK_DOWN)
            {
                player.pixelPosition.y += player.speed;
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

inline void SetPixelColor(ScreenBuffer buffer, IntVec2 pixelPosition, Uint32 color)
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

void DrawRect(ScreenBuffer buffer, IntVec2 topLeft, IntVec2 dimensions, Uint32 color)
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
    const int tileWidthToPixel = MapDimsInPixels.x / MapDimsInTiles.y;
    const int tileHeightToPixel = MapDimsInPixels.y / MapDimsInTiles.x;

    for (int y = 0; y < MapDimsInTiles.y; ++y)
    {
        for (int x = 0; x < MapDimsInTiles.x; ++x)
        {
            const int tile = map[x + y * MapDimsInTiles.y];

            if (tile == 1)
            {
                FillTileWithColor(buffer, x, y, tileWidthToPixel, tileHeightToPixel, Red);
            }
            else if (tile == 0)
            {
                FillTileWithColor(buffer, x, y, tileWidthToPixel, tileHeightToPixel, Grey);
            }
        }
    }
}

IntVec2 PixelToTilePosition(ScreenBuffer buffer, int x, int y)
{
    const int tileWidthToPixel = MapDimsInPixels.x / MapDimsInTiles.y;
    const int tileHeightToPixel = MapDimsInPixels.y / MapDimsInTiles.x;

    int tileX = x / tileWidthToPixel;
    int tileY = y / tileHeightToPixel;

    IntVec2 result = {tileX, tileY};

    return result;
}

int GetTileValue(IntVec2 tilePosition)
{
    return map[tilePosition.x + tilePosition.y * MapDimsInTiles.y];
}

float Distance(IntVec2 a, IntVec2 b)
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
            float x = player.pixelPosition.x + cos * i;
            float y = player.pixelPosition.y + sin * i;
            auto rayPixelPosition = IntVec2{(int)x, (int)y};
            auto tilePosition = PixelToTilePosition(buffer, rayPixelPosition.x, rayPixelPosition.y);
            auto tile = GetTileValue(tilePosition);

            if (tile == 1)
            {
                SetPixelColor(buffer, rayPixelPosition.x, rayPixelPosition.y, White);
            }
            else
            {
                hits->data[rayIndex].wasHit = true;
                hits->data[rayIndex].distanceFromPlayer = Distance(player.pixelPosition, rayPixelPosition);
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
    DrawRect(buffer, IntVec2{MapDimsInPixels.x, 0}, FpsViewDimsInPixels, Red);

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
                float alphaScale =  1.0f - (float) lineTopY / y;
                Uint8 alpha = (Uint8)(alphaScale * 255);
                
                SetPixelColor(buffer, IntVec2{MapDimsInPixels.x + i, y}, PackColor(128, 128, 128, alpha));
            }

            for (int y = halfHeight; y < lineBottomY; ++y)
            {
                float alphaScale = 1.0f - (float) y / lineBottomY;
                Uint8 alpha = (Uint8)(alphaScale * 255);
                SetPixelColor(buffer, IntVec2{MapDimsInPixels.x + i, y}, PackColor(128, 128, 128, alpha));
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
    }
}

int main(int argc, char *argv[])
{
    static_assert(ArrayCount(map) == MapDimsInTiles.y * MapDimsInTiles.x, "Invalid array size.");
    
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init fail : %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("Raycaster", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WindowSize.x, WindowSize.y, 0);
    if (!window)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Window creation fail : %s\n", SDL_GetError());
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Render creation for surface fail : %s\n", SDL_GetError());
        return 1;
    }

    SDL_RenderClear(renderer);

    const int bytesPerPixel = 4;
    auto *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WindowSize.x, WindowSize.y);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

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