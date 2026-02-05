#pragma once
#include <SDL3/SDL.h>

#define BUBBLE_SIZE 256.0
#define NBUBBLES 16

struct _app {
    int i;
    float w, h;
    SDL_Texture *textures[3];
    struct {
        SDL_FRect pos;
        float vx, vy;
        SDL_Texture *color;
    } bubbles[NBUBBLES];
};

bool collides(struct _app *app, int a, int b) {
    float dx = app->bubbles[a].pos.x - app->bubbles[b].pos.x;
    float dy = app->bubbles[a].pos.y - app->bubbles[b].pos.y;
    float dist = SDL_sqrtf(dx * dx + dy * dy);
    return dist <= BUBBLE_SIZE;
}

bool has_collision(struct _app *app, int cur) {
    for (int i = 0; i < app->i && i != cur; ++i) {
        if (collides(app, cur, i))
            return true;
    }
    return false;
}

void resolve_collision(struct _app *app, int cur) {
    for (int i = 0; i < app->i && i != cur; ++i) {
        if (collides(app, cur, i)) {
            // elastic collision resolution of two circles of equal mass
            float dx = app->bubbles[cur].pos.x - app->bubbles[i].pos.x;
            float dy = app->bubbles[cur].pos.y - app->bubbles[i].pos.y;
            float dist = SDL_sqrtf(dx * dx + dy * dy);

            if (dist < 0.1f) dist = 0.1f;

            float nx = dx / dist;
            float ny = dy / dist;

            float rvx = app->bubbles[cur].vx - app->bubbles[i].vx;
            float rvy = app->bubbles[cur].vy - app->bubbles[i].vy;

            float normal_vel = rvx * nx + rvy * ny;

            // avoid double collision
            if (normal_vel > 0) continue;

            app->bubbles[cur].vx -= normal_vel * nx;
            app->bubbles[cur].vy -= normal_vel * ny;
            app->bubbles[i].vx += normal_vel * nx;
            app->bubbles[i].vy += normal_vel * ny;
        }
    }
}