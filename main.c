#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#define WIDTH 1600
#define HEIGHT 900
#define BULLET_COUNT 32
#define ASTEROID_COUNT 32
#define DIFFICULTY 4

#define GO_UP    1
#define GO_DOWN  2
#define GO_LEFT  4
#define GO_RIGHT 8

#define BAILOUT_IF(x) { if (x) { fprintf(stderr, "%s\n", SDL_GetError()); return 1; } }
#define RAD(x) ((x)*M_PI/180)

struct Spaceship {
    int x;
    int y;
    int size;
    int rot;
    double vel;
};

struct Bullet {
    int alive;
    int x;
    int y;
    int vel;
    int rot;
    int age;
};

struct Asteroid {
    int alive;
    double x;
    double y;
    int vel;
    int rot;
    int size;
};

void draw_asteroid(struct Asteroid *a, struct SDL_Surface *s)
{
    if (!a->alive) return;
    ellipseRGBA(s, a->x, a->y, a->size, a->size, 255, 255, 255, 255);
}

void move_asteroid(struct Asteroid *a)
{
    if (!a->alive) return;
    a->x += cos(RAD(a->rot)) * (double)a->vel;
    a->y += sin(RAD(a->rot)) * (double)a->vel;
    if (a->x > WIDTH) a->x -= WIDTH;
    if (a->y > HEIGHT) a->y -= HEIGHT;
    if (a->x < 0) a->x += WIDTH;
    if (a->y < 0) a->y += HEIGHT;
}

void split_asteroid(struct Asteroid *src, struct Asteroid *dst)
{
    src->size /= 2;
    if (src->size < 20) {
        src->alive = 0;
        return;
    }
    dst->alive = 1;
    dst->size = src->size;
    dst->x = src->x;
    dst->y = src->y;
    dst->vel = src->vel;
    int diff = rand() % 50 + 10;
    dst->rot = src->rot - diff;
    src->rot += diff;
}

void move_bullet(struct Bullet *b)
{
    if (!b->alive) return;
    if (b->age++ > 50) {
        b->alive = 0;
    } else {
        b->x += cos(RAD(b->rot)) * b->vel;
        b->y += sin(RAD(b->rot)) * b->vel;
        b->x %= WIDTH;
        b->y %= HEIGHT;
        if (b->x < 0) b->x += WIDTH;
        if (b->y < 0) b->y += HEIGHT;
    }
}

void move_spaceship(struct Spaceship *s)
{
    s->x += cos(RAD(s->rot)) * (int)s->vel;
    s->y += sin(RAD(s->rot)) * (int)s->vel;
    s->x %= WIDTH;
    s->y %= HEIGHT;
    if (s->x < 0) s->x += WIDTH;
    if (s->y < 0) s->y += HEIGHT;
    if (s->vel > 0) s->vel -= 0.04;
    if (s->vel < 0) s->vel += 0.04;
}

void fire_bullet(struct Spaceship *s, struct Bullet *b)
{
    b->alive = 1;
    b->x = s->x + cos(RAD(s->rot)) * s->size;
    b->y = s->y + sin(RAD(s->rot)) * s->size;
    b->vel = s->vel + 10;
    b->rot = s->rot;
    b->age = 0;
}

int point_in_asteroid(int x, int y, struct Asteroid *a)
{
    return sqrt(pow(x - a->x, 2) + pow(y - a->y, 2)) < a->size;
}

void ship_vertices(struct Spaceship *ship, int *x1, int *y1,
                   int *x2, int *y2, int *x3, int *y3)
{
    *x1 = cos(RAD(ship->rot))       * ship->size;
    *y1 = sin(RAD(ship->rot))       * ship->size;
    *x2 = cos(RAD(ship->rot + 120 % 360)) * ship->size / 2;
    *y2 = sin(RAD(ship->rot + 120 % 360)) * ship->size / 2;
    *x3 = cos(RAD(ship->rot + 240 % 360)) * ship->size / 2;
    *y3 = sin(RAD(ship->rot + 240 % 360)) * ship->size / 2;
}

void draw_spaceship(struct Spaceship *ship, SDL_Surface *surf)
{
    int x1, x2, x3, y1, y2, y3;
    ship_vertices(ship, &x1, &y1, &x2, &y2, &x3, &y3);
    trigonRGBA(surf, ship->x + x1, ship->y + y1,
                     ship->x + x2, ship->y + y2,
                     ship->x + x3, ship->y + y3,
                     255, 255, 255, 255);
}

int col_spaceship_asteroid(struct Spaceship *ship, struct Asteroid *a)
{
    int x1, x2, x3, y1, y2, y3;
    if (!a->alive) return 0;
    ship_vertices(ship, &x1, &y1, &x2, &y2, &x3, &y3);
    return point_in_asteroid(ship->x + x1, ship->y + y1, a)
        || point_in_asteroid(ship->x + x2, ship->y + y2, a)
        || point_in_asteroid(ship->x + x3, ship->y + y3, a);
}

void draw_screen(SDL_Surface *screen, struct Spaceship *player,
                 struct Bullet bullets[], struct Asteroid asteroids[])
{
    SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

    draw_spaceship(player, screen);
    for (int i = 0; i < BULLET_COUNT; i++) {
        if (!bullets[i].alive) continue;
        filledEllipseRGBA(screen, bullets[i].x, bullets[i].y, 2, 2, 255, 255, 255, 255);
    }
    for (int i = 0; i < ASTEROID_COUNT; i++) {
        draw_asteroid(&asteroids[i], screen);
    }
}

int timer_cb(int interval)
{
    SDL_Event event;
    event.type = SDL_USEREVENT;
    SDL_PushEvent(&event);
    return interval;
}

int main(void)
{
    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    TTF_Font *font = TTF_OpenFont("font.ttf", 24);
    if (!font) {
        fprintf(stderr, "TTF_OpentFont: %s\n", TTF_GetError());
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    srand(time(0));

    SDL_Surface* screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, SDL_HWSURFACE | SDL_DOUBLEBUF);
    SDL_WM_SetCaption("Asteroids", 0);
    SDL_EnableKeyRepeat(1, SDL_DEFAULT_REPEAT_INTERVAL);

    Mix_Init(0);
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {
        fprintf(stderr, "Mix_OpenAudio: %s\n", Mix_GetError());
        return 1;
    }
    Mix_Chunk *bum = Mix_LoadWAV("bum.wav");
    Mix_Chunk *trach = Mix_LoadWAV("trach.wav");

    if (!bum || !trach) {
        fprintf(stderr, "Mix_LoadWAV: %s\n", Mix_GetError());
        return 1;
    }

    SDL_Event event;
    int gameRunning = 0;
    int keyboard_state = 0;
    int won = 0;

    struct Spaceship player = { 400, 300, 40, 0, 0 };
    struct Bullet bullets[BULLET_COUNT];
    struct Asteroid asteroids[ASTEROID_COUNT];
    for (int i = 0; i < BULLET_COUNT; i++) {
        bullets[i].alive = 0;
    }
    int bullet_iter = 0;
    for (int i = 0; i < DIFFICULTY; i++) {
        asteroids[i].alive = 1;
        asteroids[i].size = 80;
        asteroids[i].vel = 5;
        asteroids[i].rot = rand() % 360;
        do {
            asteroids[i].x = rand() % WIDTH;
            asteroids[i].y = rand() % HEIGHT;
        } while (col_spaceship_asteroid(&player, &asteroids[i]));
    }
    for (int i = DIFFICULTY; i < ASTEROID_COUNT; i++) {
        asteroids[i].alive = 0;
    }
    int asteroid_iter = DIFFICULTY;

    SDL_AddTimer(16, (SDL_NewTimerCallback)timer_cb, NULL);

    SDL_Color white = { 255, 255, 255, 255};
    SDL_Surface *text = TTF_RenderText_Solid(font, "Press space to start", white);
    if (!text) {
        fprintf(stderr, "TTF_RenderText_Solid: %s\n", TTF_GetError());
        goto quit;
    }
    SDL_Rect textrect = { screen->w / 2 - text->w / 2, screen->h / 2 - text->h / 2,
                          screen->w, screen->h };
    while (!gameRunning) {
        draw_screen(screen, &player, bullets, asteroids);
        SDL_BlitSurface(text, NULL, screen, &textrect);
        BAILOUT_IF(SDL_WaitEvent(&event) == 0);
        if (event.type == SDL_KEYUP && event.key.keysym.sym == SDLK_SPACE) {
            gameRunning = 1;
        }
        if (event.type == SDL_QUIT) {
            goto quit;
        }
        SDL_Flip(screen);
    }
    while (gameRunning) {
        BAILOUT_IF(SDL_WaitEvent(&event) == 0);
        switch (event.type) {
        case SDL_USEREVENT: // timer
            if (keyboard_state & GO_UP) {
                if (player.vel < 10) player.vel++;
            }
            if (keyboard_state & GO_DOWN) {
                if (player.vel > -10) player.vel--;
            }
            if (keyboard_state & GO_LEFT) {
                player.rot -= 10;
            }
            if (keyboard_state & GO_RIGHT) {
                player.rot += 10;
            }
            for (int i = 0; i < BULLET_COUNT; i++) {
                move_bullet(&bullets[i]);
                if (!bullets[i].alive) continue;
                for (int j = 0; j < ASTEROID_COUNT; j++) {
                    if (!asteroids[j].alive) continue;
                    if (point_in_asteroid(bullets[i].x, bullets[i].y, &asteroids[j])) {
                        asteroid_iter++;
                        asteroid_iter %= ASTEROID_COUNT;
                        split_asteroid(&asteroids[j], &asteroids[asteroid_iter]);
                        bullets[i].alive = 0;
                        Mix_PlayChannel(-1, trach, 0);
                        continue;
                    }
                }
            }
            move_spaceship(&player);
            for (int i = 0; i < ASTEROID_COUNT; i++) {
                move_asteroid(&asteroids[i]);
                if (col_spaceship_asteroid(&player, &asteroids[i])) {
                    gameRunning = 0;
                }
            }
            break;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_LEFT:
                keyboard_state |= GO_LEFT;
                break;
            case SDLK_RIGHT:
                keyboard_state |= GO_RIGHT;
                break;
            case SDLK_UP:
                keyboard_state |= GO_UP;
                break;
            case SDLK_DOWN:
                keyboard_state |= GO_DOWN;
                break;
            default:
                /* nothing. Fuck off, clang */
                break;
            }
            break;
        case SDL_KEYUP:
            switch (event.key.keysym.sym) {
            case SDLK_SPACE:
                fire_bullet(&player, &bullets[bullet_iter++]);
                Mix_PlayChannel(-1, bum, 0);
                bullet_iter %= BULLET_COUNT;
                break;
            case SDLK_LEFT:
                keyboard_state &= ~GO_LEFT;
                break;
            case SDLK_RIGHT:
                keyboard_state &= ~GO_RIGHT;
                break;
            case SDLK_UP:
                keyboard_state &= ~GO_UP;
                break;
            case SDLK_DOWN:
                keyboard_state &= ~GO_DOWN;
                break;
            default:
                /* nothing. Fuck off, clang */
                break;
            }
            break;
        case SDL_QUIT:
            gameRunning = 0;
            break;
        }

        player.rot %= 360;

        int i;
        for (i = 0; i < ASTEROID_COUNT; i++) {
            if (asteroids[i].alive) {
                break;
            }
        }
        if (i == ASTEROID_COUNT) {
            won = 1;
            gameRunning = 0;
        }
        draw_screen(screen, &player, bullets, asteroids);

        SDL_Flip(screen);
    }

    SDL_FreeSurface(text);
    if (won) {
        text = TTF_RenderText_Solid(font, "You won. You'll make your mom proud", white);
    } else {
        text = TTF_RenderText_Solid(font, "You crashed, lol", white);
    }
    if (!text) {
        fprintf(stderr, "TTF_RenderText_Solid: %s\n", TTF_GetError());
        goto quit;
    }
    textrect.x = screen->w / 2 - text->w / 2;
    textrect.y = screen->h / 2 - text->h / 2;
    for (;;) {
        draw_screen(screen, &player, bullets, asteroids);
        BAILOUT_IF(SDL_BlitSurface(text, NULL, screen, &textrect));
        BAILOUT_IF(SDL_WaitEvent(&event) == 0);
        if (event.type == SDL_QUIT) {
            break;
        }
        SDL_Flip(screen);
    }

quit:
    TTF_CloseFont(font);
    Mix_FreeChunk(bum);
    Mix_FreeChunk(trach);
    Mix_CloseAudio();
    Mix_Quit();
    SDL_FreeSurface(text);
    SDL_FreeSurface(screen);
    SDL_Quit();

    return 0;
}
