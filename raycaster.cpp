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

constexpr IntVec2 MapDimensions = IntVec2 {10, 10};
const IntVec2 WindowSize = IntVec2 {640, 480};
const float AngleToRadian = M_PI / 180.0f;

bool done;

Player player = 
{
    .pixelPosition = IntVec2 {21, 20},
    .dimensions = IntVec2 {5, 5},
    .facingAngle = 90.0f,
    .speed = 2.0f,
    .fov = 10
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

constexpr Uint32 PackColor(Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255)
{ 
    return ((alpha << 24) | (red << 16) | (green << 8) | blue);
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
    const int tileWidthToPixel = buffer.width / MapDimensions.y;
    const int tileHeightToPixel = buffer.height / MapDimensions.x;

    const Uint32 blockColor = PackColor(128, 0, 0);
    const Uint32 tileColor = PackColor(128, 128, 128);

    for (int y = 0; y < MapDimensions.y; ++y)
    {
        for (int x = 0; x < MapDimensions.x; ++x)
        {
            const int tile = map[x + y * MapDimensions.y];

            if (tile == 1)
            {
                FillTileWithColor(buffer, x, y, tileWidthToPixel, tileHeightToPixel, blockColor);
            }
            else if (tile == 0)
            {
                FillTileWithColor(buffer, x, y, tileWidthToPixel, tileHeightToPixel, tileColor);
            }
        }
    }
}

IntVec2 PixelToTilePosition(ScreenBuffer buffer, int x, int y)
{
    const int tileWidthToPixel = buffer.width / MapDimensions.y;
    const int tileHeightToPixel = buffer.height / MapDimensions.x;

    int tileX = x / tileWidthToPixel;
    int tileY = y / tileHeightToPixel;

    IntVec2 result = {tileX, tileY};

    return result;
}

int GetTileValue(IntVec2 tilePosition)
{
    return map[tilePosition.x + tilePosition.y * MapDimensions.y];
}

void DrawPlayer(ScreenBuffer buffer)
{
    const Uint32 playerColor = PackColor(0, 0, 0);
    const Uint32 headColor = PackColor(64, 64, 64);
    const Uint32 viewColor = PackColor(255, 255, 255);

    DrawRect(buffer, player.pixelPosition, player.dimensions, playerColor);
    SetPixelColor(buffer, player.pixelPosition.x + (player.dimensions.x / 2), player.pixelPosition.y, headColor);
    
    for (int angle = -player.fov/2; angle < player.fov; ++angle)
    {
        float cos = cosf((player.facingAngle + angle) * AngleToRadian);
        float sin = sinf((player.facingAngle + angle) * AngleToRadian);

        for (int i = 0; i < 300; i += 2)
        {
            float x = player.pixelPosition.x + cos * i;
            float y = player.pixelPosition.y + sin * i;
            int pixelLocationX = (int)x;
            int pixelLocationY = (int)y;
            auto tilePosition = PixelToTilePosition(buffer, pixelLocationX, pixelLocationY);
            auto tile = GetTileValue(tilePosition);
            if (tile == 1)
            {
                SetPixelColor(buffer, x, y, viewColor);
            }
            else
            {
                break;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    static_assert(ArrayCount(map) == MapDimensions.y * MapDimensions.x, "Invalid array size.");
    
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

    done = false;

    while (!done)
    {
        DrawMap(buffer);
        DrawPlayer(buffer);
        Update(window, renderer, buffer);
    }

    SDL_Quit();
    return 0;
}