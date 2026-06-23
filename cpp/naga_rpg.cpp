// NĀGA RPG - Suwarnadwipa Chronicles
// Pusaka Media - Belajar-coding
// C++17 / raylib 5.0
// Build native:  g++ -std=c++17 naga_rpg.cpp -lraylib -lm -lpthread -ldl -o naga_rpg
// Build WASM: emcc naga_rpg.cpp -o naga_rpg.html -s USE_GLFW=3 -s ASYNCIFY -lraylib -std=c++17 -O3

#include "raylib.h"
#include <vector>
#include <cmath>
#include <string>
#include <cstdlib>

struct Player {
    Vector2 pos{780, 560};
    Vector2 vel{0,0};
    float dir = 0.0f;
    int hp=120, maxHp=120;
    int level=1, xp=0, xpNeed=100;
    int atk=24, def=8;
    float speed = 3.25f;
    int gold=0, gems=0, kills=0, potions=3;
    int fireLv=1;
    int invul=0, dashCd=0;
};

struct Enemy {
    Vector2 pos;
    int hp, maxHp, atk;
    float speed;
    int type; // 0 Buto, 1 Leak, 2 Rangda
    int hurt=0;
};

struct Projectile {
    Vector2 pos, vel;
    int life=30;
    int dmg=40;
    bool fire=true;
};

static const int WORLD_W = 72, WORLD_H = 52, TILE = 40;
int world[WORLD_H][WORLD_W];

bool isWalkable(float x, float y){
    int tx = (int)(x / TILE);
    int ty = (int)(y / TILE);
    if(tx < 0 || ty < 0 || tx >= WORLD_W || ty >= WORLD_H) return false;
    return world[ty][tx] != 1; // 1 = water
}

int main(){
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "NAGA RPG - Suwarnadwipa Chronicles | Pusaka Media | C++ / raylib");
    SetTargetFPS(60);

    // Generate Nusantara temple map
    for(int y=0;y<WORLD_H;y++) for(int x=0;x<WORLD_W;x++){
        int v = 0;
        if(GetRandomValue(0,1000) < 28) v = 1; // water
        if(GetRandomValue(0,1000) < 32) v = 2; // sand
        if(x>28 && x<44 && y>19 && y<33 && GetRandomValue(0,100) > 36) v = 3;
        if(x>=33 && x<=39 && y>=23 && y<=29) v = 4; // candi
        world[y][x] = v;
    }

    Player pl;
    std::vector<Enemy> enemies;
    for(int i=0;i<18;i++){
        Enemy e;
        e.pos = {(float)GetRandomValue(120, WORLD_W*TILE-120), (float)GetRandomValue(120, WORLD_H*TILE-120)};
        e.type = GetRandomValue(0,100) < 55 ? 0 : GetRandomValue(0,100) < 65 ? 1 : 2;
        e.maxHp = e.hp = e.type==0?36:e.type==1?58:95;
        e.atk = e.type==0?9:e.type==1?15:22;
        e.speed = e.type==0?1.45f:e.type==1?1.1f:0.82f;
        enemies.push_back(e);
    }

    std::vector<Projectile> shots;
    Camera2D cam{}; cam.zoom = 1.0f;

    while(!WindowShouldClose()){
        // ---- Update (C++ game logic) ----
        Vector2 input{0,0};
        if(IsKeyDown(KEY_W)) input.y -= 1;
        if(IsKeyDown(KEY_S)) input.y += 1;
        if(IsKeyDown(KEY_A)) input.x -= 1;
        if(IsKeyDown(KEY_D)) input.x += 1;
        if(Vector2Length(input) > 0){
            input = Vector2Normalize(input);
            pl.dir = atan2f(input.y, input.x);
        }
        pl.vel.x = pl.vel.x * 0.78f + input.x * pl.speed * 0.22f;
        pl.vel.y = pl.vel.y * 0.78f + input.y * pl.speed * 0.22f;
        Vector2 npos = { pl.pos.x + pl.vel.x * 2.65f, pl.pos.y + pl.vel.y * 2.65f };
        if(isWalkable(npos.x, pl.pos.y)) pl.pos.x = npos.x;
        if(isWalkable(pl.pos.x, npos.y)) pl.pos.y = npos.y;

        // Fire breath - E
        static int fireCd = 0;
        if(fireCd>0) fireCd--;
        if(IsKeyPressed(KEY_E) && fireCd==0){
            fireCd = 95;
            for(int k=-2;k<=2;k++){
                float a = pl.dir + k*0.18f;
                shots.push_back({{pl.pos.x + cosf(a)*34, pl.pos.y + sinf(a)*34},
                                 {cosf(a)*7.2f, sinf(a)*7.2f}, 34, pl.atk+18+pl.fireLv*6, true});
            }
        }

        // Melee - Space
        if(IsKeyPressed(KEY_SPACE)){
            for(auto it = enemies.begin(); it != enemies.end(); ){
                float dist = Vector2Distance(it->pos, pl.pos);
                if(dist < 72.0f){
                    it->hp -= pl.atk + GetRandomValue(-2,5);
                    it->hurt = 9;
                    if(it->hp <= 0){
                        pl.xp += it->type==0?18:it->type==1?34:62;
                        pl.gold += GetRandomValue(3,12);
                        pl.kills++;
                        it = enemies.erase(it); continue;
                    }
                }
                ++it;
            }
        }

        // Enemies AI
        for(auto &e : enemies){
            Vector2 toPl = Vector2Subtract(pl.pos, e.pos);
            float d = Vector2Length(toPl);
            if(d < 420 && d > 1){
                toPl = Vector2Scale(Vector2Normalize(toPl), e.speed);
                Vector2 ep = { e.pos.x + toPl.x, e.pos.y + toPl.y };
                if(isWalkable(ep.x, e.pos.y)) e.pos.x = ep.x;
                if(isWalkable(e.pos.x, ep.y)) e.pos.y = ep.y;
            }
            if(e.hurt>0) e.hurt--;
        }

        // Projectiles
        for(auto it = shots.begin(); it != shots.end(); ){
            it->pos = Vector2Add(it->pos, it->vel);
            it->life--;
            bool hit=false;
            for(auto eit = enemies.begin(); eit != enemies.end(); ++eit){
                if(Vector2Distance(eit->pos, it->pos) < 28){
                    eit->hp -= it->dmg; hit=true; break;
                }
            }
            if(it->life<=0 || hit) it = shots.erase(it); else ++it;
        }

        // Level up
        if(pl.xp >= pl.xpNeed){
            pl.xp -= pl.xpNeed;
            pl.level++; pl.xpNeed = (int)(pl.xpNeed * 1.48f);
            pl.maxHp += 22; pl.hp = pl.maxHp;
            pl.atk += 5; pl.def += 2; pl.speed += 0.065f;
        }

        cam.target = pl.pos;
        cam.offset = {(float)screenWidth/2, (float)screenHeight/2};

        // ---- Draw ----
        BeginDrawing();
        ClearBackground((Color){11,7,20,255});
        BeginMode2D(cam);

        // draw tiles
        for(int y=0;y<WORLD_H;y++) for(int x=0;x<WORLD_W;x++){
            Color col = (Color){24,50,32,255};
            if(world[y][x]==1) col = (Color){19,39,64,255};
            if(world[y][x]==2) col = (Color){197,167,106,255};
            if(world[y][x]==3) col = (Color){90,90,106,255};
            if(world[y][x]==4) col = (Color){126,109,79,255};
            DrawRectangle(x*TILE, y*TILE, TILE, TILE, col);
        }

        // enemies
        for(auto &e : enemies){
            DrawCircleV(e.pos, e.type==2?18:14, e.type==0?(Color){185,68,68,255} : e.type==1?(Color){90,58,138,255} : (Color){42,42,42,255});
        }
        // player - golden naga top-down
        DrawCircleV(pl.pos, 26, (Color){245,201,74,255});
        Vector2 head = { pl.pos.x + cosf(pl.dir)*24, pl.pos.y + sinf(pl.dir)*24 };
        DrawCircleV(head, 13, (Color){255,219,100,255});

        // shots
        for(auto &s : shots) DrawCircleV(s.pos, 6, (Color){255,240,176,255});

        EndMode2D();

        // HUD
        DrawText(TextFormat("LV %d  NAGA EMAS  HP %d/%d  ATK %d  GOLD %d  KILL %d", pl.level, pl.hp, pl.maxHp, pl.atk, pl.gold, pl.kills), 20, 18, 20, (Color){255,226,138,255});
        DrawText("WASD Move | SPACE Attack | E Fire Breath | Q Dash | H Potion", 20, 46, 16, (Color){205,179,122,255});
        DrawText("NAGA RPG - C++ / raylib - Pusaka Media", 20, screenHeight-28, 16, (Color){191,160,100,255});
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
