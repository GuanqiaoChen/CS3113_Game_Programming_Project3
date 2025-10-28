/**
* Author: Guanqiao Chen
* Assignment: Lunar Lander
* Date due: 2025-10-13, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include "CS3113/Entity.h"

struct GameState {
    Entity *spaceship;
    Map    *map;
};
GameState gState;

constexpr int   SCREEN_WIDTH  = 1600;
constexpr int   SCREEN_HEIGHT = 1000;
constexpr int   FPS           = 120;

constexpr char  BG_COLOUR[] = "#f6f2f1ff";
constexpr Vector2 ORIGIN    = { SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f };

constexpr float FIXED_TIMESTEP          = 1.0f / 60.0f;
constexpr float ACCELERATION_OF_GRAVITY = 100.0f;  
constexpr float AX_INC                  = 60.0f;  
constexpr float MAIN_THRUST             = -220.0f; 
constexpr float DRAG_K                  = 1.2f;    
constexpr float TILE_DIMENSION          = 75.0f;

AppStatus gAppStatus   = RUNNING;
float     gPrevTicks   = 0.0f;
float     gAccumulator = 0.0f;

bool  gGameOver = false;
bool  gWin      = false;

Texture2D gTexWin;
Texture2D gTexFail;

struct MovingPlatform {
    Vector2 pos;        
    Vector2 size;      
    Rectangle uv;      
    float speed;        
    float leftX, rightX;
    int dir;            
    bool winning;       
};
MovingPlatform gPlat;   

float gFuel = 100.0f;
float gFuel = FUEL_MAX;   

constexpr int   HUD_PAD     = 24;
constexpr int   HUD_BAR_W   = 320;
constexpr int   HUD_BAR_H   = 20;
const float H_THRUST = 320.0f;       
const float DAMPING  = 0.90f;     

constexpr Vector2 SHIP_ATLAS_DIM = { 1, 4 };

constexpr int LEVEL_WIDTH  = 20,
              LEVEL_HEIGHT = 12;
constexpr unsigned int LEVEL_DATA[] = {
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 0, 3, 2, 2, 0, 0, 0, 0, 0, 0,
   2, 0, 2, 2, 0, 0, 0, 2, 3, 3, 2, 3, 3, 3, 0, 0, 0, 2, 0, 2,
   3, 2, 3, 3, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 2, 2, 0, 3, 2, 3,
   3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2, 3, 3, 3,
};

static inline float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : (v > hi) ? hi : v;
}

inline bool worldToTile(float worldX, float worldY, int *tx, int *ty, const Map* m) {
    if (worldX < m->getLeftBoundary()  || worldX > m->getRightBoundary() ||
        worldY < m->getTopBoundary()   || worldY > m->getBottomBoundary()) return false;
    *tx = (int)floorf((worldX - m->getLeftBoundary()) / m->getTileSize());
    *ty = (int)floorf((worldY - m->getTopBoundary())  / m->getTileSize());
    if (*tx < 0 || *tx >= m->getMapColumns() || *ty < 0 || *ty >= m->getMapRows()) return false;
    return true;
}

inline unsigned int tileAt(int tx, int ty, const Map* m) {
    return m->getLevelData()[ty * m->getMapColumns() + tx];
}

enum ShipAnim { SHIP_IDLE=3 }; 
void setShipFrame(int idx) { gState.spaceship->setAtlasSingleFrame(idx); }

void initialise() {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Lunar Lander");
    SetTargetFPS(FPS);

    gTexWin  = LoadTexture("assets/game/mission_accomplished.png");
    gTexFail = LoadTexture("assets/game/mission_failed.png");

    gState.map = new Map(
        LEVEL_WIDTH, LEVEL_HEIGHT,
        (unsigned int*)LEVEL_DATA,
        "assets/game/tileset.png",
        TILE_DIMENSION,
        4, 1,                
        ORIGIN
    ); 
    
    int      tileIndex0 = 3 - 1; 
    Texture2D atlas     = gState.map->getTextureAtlas();
    Rectangle uv        = getUVRectangle(&atlas, tileIndex0,
                                         gState.map->getTextureRows(),
                                         gState.map->getTextureColumns());

    float midY = ORIGIN.y + 200.0f;    
    float midX = ORIGIN.x;           

    gPlat.uv     = uv;
    gPlat.size   = { TILE_DIMENSION, TILE_DIMENSION };
    gPlat.pos    = { midX - 200.0f, midY }; 
    gPlat.leftX  = midX - 300.0f;
    gPlat.rightX = midX + 300.0f;
    gPlat.speed  = 120.0f;   
    gPlat.dir    = +1;
    gPlat.winning = true;    

    std::map<Direction, std::vector<int>> shipAtlas = {
        {LEFT,  {0}}, {UP, {1}}, {RIGHT, {2}}, {DOWN, {3}}
    };
    gState.spaceship = new Entity(
        { ORIGIN.x - 500.0f, ORIGIN.y - 250.0f }, 
        { 100.0f, 120.0f },                       
        "assets/game/spaceship.png",
        ATLAS, SHIP_ATLAS_DIM, shipAtlas, PLAYER
    );
    gState.spaceship->setColliderDimensions({ 40.0f, 40.0f });
    gState.spaceship->setAcceleration({ 0.0f, ACCELERATION_OF_GRAVITY });
    setShipFrame(SHIP_IDLE);
}


void processInput() {
    if (IsKeyPressed(KEY_Q) || WindowShouldClose()) { gAppStatus = TERMINATED; return; }
    if (gGameOver) return;

    Vector2 a = gState.spaceship->getAcceleration();
    
    a.x = 0.0f;

    if (IsKeyPressed(KEY_A) && gFuel > 0.0f) { a.x -= AX_INC; gFuel -= 1.0f; }
    if (IsKeyPressed(KEY_D) && gFuel > 0.0f) { a.x += AX_INC; gFuel -= 1.0f; }
    a.x = clampf(a.x, -H_THRUST, H_THRUST);

    if (IsKeyDown(KEY_A) && gFuel > 0.0f) a.x -= AX_INC;
    if (IsKeyDown(KEY_D) && gFuel > 0.0f) a.x += AX_INC;
    a.x = clampf(a.x, -H_THRUST, H_THRUST);

    if (IsKeyPressed(KEY_W) && gFuel > 0.0f) { gFuel -= 1.0f; }
    if (IsKeyDown(KEY_W) && gFuel > 0.0f)    { a.y = ACCELERATION_OF_GRAVITY + MAIN_THRUST; setShipFrame(0); }
    else                                      { a.y = ACCELERATION_OF_GRAVITY; setShipFrame(SHIP_IDLE); }

    if (gFuel <= 0.0f) a.y = ACCELERATION_OF_GRAVITY;

    gState.spaceship->setAcceleration(a);

    gFuel = clampf(gFuel, 0.0f, FUEL_MAX);
}

void update() {
    float now   = (float)GetTime();
    float frame = now - gPrevTicks;
    gPrevTicks  = now;

    frame += gAccumulator;
    if (frame < FIXED_TIMESTEP) { gAccumulator = frame; return; }

    while (frame >= FIXED_TIMESTEP) {
        if (!gGameOver) {
            Vector2 v = gState.spaceship->getVelocity();
            Vector2 a = gState.spaceship->getAcceleration();
            a.x += (-DRAG_K) * v.x;
            gState.spaceship->setAcceleration(a);

            a.x = clampf(a.x, -H_THRUST, H_THRUST); 
            gState.spaceship->setAcceleration(a);

            gPlat.pos.x += gPlat.dir * gPlat.speed * FIXED_TIMESTEP;
            if (gPlat.pos.x <= gPlat.leftX)  { gPlat.pos.x = gPlat.leftX;  gPlat.dir = +1; }
            if (gPlat.pos.x >= gPlat.rightX) { gPlat.pos.x = gPlat.rightX; gPlat.dir = -1; }
        }

        gState.spaceship->update(FIXED_TIMESTEP, nullptr, gState.map, nullptr, 0);

        {
            Vector2 pos  = gState.spaceship->getPosition();
            Vector2 half = { gState.spaceship->getColliderDimensions().x/2.0f,
                            gState.spaceship->getColliderDimensions().y/2.0f };

            Rectangle shipBox = {
                pos.x - half.x, pos.y - half.y,
                gState.spaceship->getColliderDimensions().x,
                gState.spaceship->getColliderDimensions().y
            };

            Rectangle platBox = {
                gPlat.pos.x - gPlat.size.x/2.0f,
                gPlat.pos.y - gPlat.size.y/2.0f,
                gPlat.size.x, gPlat.size.y
            };

            bool overlapX = (shipBox.x < platBox.x + platBox.width) && (shipBox.x + shipBox.width > platBox.x);
            bool overlapY = (shipBox.y < platBox.y + platBox.height) && (shipBox.y + shipBox.height > platBox.y);

            if (gState.spaceship->isCollidingTop())    { gGameOver = true; gWin = false; }
            if (gState.spaceship->isCollidingLeft())   { gGameOver = true; gWin = false; }
            if (gState.spaceship->isCollidingRight())  { gGameOver = true; gWin = false; }

            if (!gGameOver && gState.spaceship->isCollidingBottom()) {
                int tx, ty;
                if (worldToTile(pos.x, pos.y + half.y - 1.0f, &tx, &ty, gState.map)) {
                    unsigned int t = tileAt(tx, ty, gState.map);
                    if (t == 4) { gGameOver = true; gWin = true;  }
                    else        { gGameOver = true; gWin = false; }
                }
            }

            if (!gGameOver && overlapX && overlapY) {

                float shipBottom = shipBox.y + shipBox.height;
                float platTop    = platBox.y;

                Vector2 vel = gState.spaceship->getVelocity();

                if (vel.y > 0.0f && (shipBottom - platTop) <= (gPlat.size.y * 0.6f)) {

                    gState.spaceship->setPosition({ pos.x, platTop - half.y });
                    vel.y = 0.0f;
                    gState.spaceship->setVelocity(vel);

                    gGameOver = true;
                    gWin      = gPlat.winning;
                } else {

                    gGameOver = true;
                    gWin      = false;
                }
            }
        }

        frame -= FIXED_TIMESTEP;
    }
    gAccumulator = frame;
}

static void drawFuelHUD() {
    Color panel = ColorAlpha(BLACK, 0.15f);
    DrawRectangle(HUD_PAD - 8, HUD_PAD - 8, HUD_BAR_W + 120, HUD_BAR_H + 28, panel);

    DrawText("FUEL", HUD_PAD, HUD_PAD - 2, 22, DARKGRAY);

    DrawRectangle(HUD_PAD, HUD_PAD + 20, HUD_BAR_W, HUD_BAR_H, DARKGRAY);

    float pct     = (FUEL_MAX > 0.0f) ? (gFuel / FUEL_MAX) : 0.0f;
    int   fillW   = (int)(pct * HUD_BAR_W);

    Color fillCol = (pct > 0.5f) ? GREEN : (pct > 0.2f ? ORANGE : RED);
    DrawRectangle(HUD_PAD, HUD_PAD + 20, fillW, HUD_BAR_H, fillCol);

    DrawRectangleLines(HUD_PAD, HUD_PAD + 20, HUD_BAR_W, HUD_BAR_H, BLACK);

    int percent = (int)roundf(pct * 100.0f);
    const char* txt = TextFormat("%d%%", percent);
    int tw = MeasureText(txt, 22);
    DrawText(txt, HUD_PAD + HUD_BAR_W + 12, HUD_PAD + 14, 22, BLACK);
}

void render() {
    BeginDrawing();
    ClearBackground(ColorFromHex(BG_COLOUR));

    gState.map->render();
    gState.spaceship->render();

    {
        Texture2D atlas = gState.map->getTextureAtlas();
        Rectangle dest  = {
            gPlat.pos.x - gPlat.size.x/2.0f, 
            gPlat.pos.y - gPlat.size.y/2.0f, 
            gPlat.size.x,
            gPlat.size.y
        };

        DrawTexturePro(
            atlas,
            gPlat.uv,
            dest,
            {0.0f, 0.0f}, 
            0.0f,
            WHITE
        );
    }


    if (gGameOver) {
        Texture2D overlay = gWin ? gTexWin : gTexFail;


        float scaleX = (float)SCREEN_WIDTH  / overlay.width;
        float scaleY = (float)SCREEN_HEIGHT / overlay.height;
        float scale  = fminf(scaleX, scaleY) * 0.8f; 

        Rectangle src  = { 0, 0, (float)overlay.width, (float)overlay.height };
        Rectangle dest = {
            ORIGIN.x - (overlay.width * scale)/2.0f,
            ORIGIN.y - (overlay.height* scale)/2.0f,
            overlay.width  * scale,
            overlay.height * scale
        };
        DrawTexturePro(overlay, src, dest, {0,0}, 0.0f, WHITE);
    }

    drawFuelHUD();
    EndDrawing();
}

void shutdown() {
    delete gState.spaceship;
    delete gState.map;

    UnloadTexture(gTexWin);
    UnloadTexture(gTexFail);

    CloseWindow();
}


int main() {
    initialise();
    while (gAppStatus == RUNNING) { processInput(); update(); render(); }
    shutdown();
    return 0;
}
