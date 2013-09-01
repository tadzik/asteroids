#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#define WIDTH 1600
#define HEIGHT 900
#define BULLET_COUNT 32
#define ASTEROID_COUNT 32

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
    int x;
    int y;
    int vel;
    int rot;
    int age;
};

struct Asteroid {
    double x;
    double y;
    int vel;
    int rot;
    int size;
};

void draw_asteroid(struct Asteroid *a, struct SDL_Surface *s)
{
    if (a->x == -1) return;
    ellipseRGBA(s, a->x, a->y, a->size, a->size, 255, 255, 255, 255);
}

void move_asteroid(struct Asteroid *a)
{
    if (a->x == -1) return;
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
        src->x = -1;
        return;
    }
    dst->size = src->size;
    dst->x = src->x;
    dst->y = src->y;
    dst->vel = src->vel;
    printf("Old rotation: %d\n", src->rot);
    int diff = rand() % 50 + 10;
    dst->rot = src->rot - diff;
    printf("Dst rotation: %d\n", dst->rot);
    src->rot += diff;
    printf("New rotation: %d\n", src->rot);
}

void move_bullet(struct Bullet *b)
{
    if (b->x == -1) return;
    if (b->age++ > 100) {
        b->x = -1;
        b->y = -1;
        b->vel = 0;
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
    b->x = s->x + cos(RAD(s->rot)) * s->size;
    b->y = s->y + sin(RAD(s->rot)) * s->size;
    b->vel = s->vel + 10;
    b->rot = s->rot;
    b->age = 0;
}

void draw_spaceship(struct Spaceship *ship, SDL_Surface *surf)
{
    int x1, x2, x3, y1, y2, y3;
    x1 = cos(RAD(ship->rot))       * ship->size;
    y1 = sin(RAD(ship->rot))       * ship->size;
    x2 = cos(RAD(ship->rot + 120 % 360)) * ship->size / 2;
    y2 = sin(RAD(ship->rot + 120 % 360)) * ship->size / 2;
    x3 = cos(RAD(ship->rot + 240 % 360)) * ship->size / 2;
    y3 = sin(RAD(ship->rot + 240 % 360)) * ship->size / 2;
    trigonRGBA(surf, ship->x + x1, ship->y + y1,
                     ship->x + x2, ship->y + y2,
                     ship->x + x3, ship->y + y3,
                     255, 255, 255, 255);
}

int timer_cb(int interval, void *p)
{
    SDL_Event event;
    event.type = SDL_USEREVENT;
    SDL_PushEvent(&event);
    return interval;
    (void)p;
}

int main(void)
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    srand(time(0));

    SDL_Surface* screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, SDL_HWSURFACE | SDL_DOUBLEBUF);
    SDL_WM_SetCaption("Asteroids", 0);
    SDL_EnableKeyRepeat(1, SDL_DEFAULT_REPEAT_INTERVAL);

    SDL_Event event;
    int gameRunning = 1;

    struct Spaceship player = { 400, 300, 40, 0, 0 };
    struct Bullet bullets[BULLET_COUNT];
    struct Asteroid asteroids[ASTEROID_COUNT];
    int bullet_iter = 0;
    int asteroid_iter = 0;
    for (int i = 0; i < BULLET_COUNT; i++) {
        bullets[i].x = -1;
        bullets[i].y = -1;
        bullets[i].vel = 0;
    }
    for (int i = 0; i < ASTEROID_COUNT; i++) {
        asteroids[i].x = -1;
    }
    asteroids[0].x = 600;
    asteroids[0].y = 600;
    asteroids[0].vel = 5;
    asteroids[0].rot = 35;
    asteroids[0].size = 80;

    SDL_AddTimer(16, (SDL_NewTimerCallback)timer_cb, NULL);

    while (gameRunning) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_USEREVENT:
                for (int i = 0; i < BULLET_COUNT; i++) {
                    move_bullet(&bullets[i]);
                    if (bullets[i].x == -1) continue;
                    for (int j = 0; j < ASTEROID_COUNT; j++) {
                        if (asteroids[j].x == -1) continue;
                        int dist = (int)sqrt(
                            pow(bullets[i].x - asteroids[j].x, 2)
                            + pow(bullets[i].y - asteroids[j].y, 2)
                        );
                        if (dist < asteroids[j].size) {
                            asteroid_iter++;
                            asteroid_iter %= ASTEROID_COUNT;
                            split_asteroid(&asteroids[j], &asteroids[asteroid_iter]);
                            bullets[i].x = -1;
                            continue;
                        }
                    }
                }
                for (int i = 0; i < ASTEROID_COUNT; i++) {
                    move_asteroid(asteroids + i);
                }
                move_spaceship(&player);
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_LEFT)
                    player.rot -= 10;
                if (event.key.keysym.sym == SDLK_RIGHT)
                    player.rot += 10;
                if (event.key.keysym.sym == SDLK_UP)
                    if (player.vel < 10) player.vel++;
                if (event.key.keysym.sym == SDLK_DOWN)
                    if (player.vel > -10) player.vel--;
                break;
            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_SPACE) {
                    fire_bullet(&player, bullets + bullet_iter++);
                    bullet_iter %= BULLET_COUNT;
                }
                break;
            case SDL_QUIT:
                gameRunning = 0;
                break;
            }
        }
        player.rot %= 360;

        SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));

        draw_spaceship(&player, screen);
        for (int i = 0; i < BULLET_COUNT; i++) {
            if (bullets[i].x == -1) continue;
            filledEllipseRGBA(screen, bullets[i].x, bullets[i].y, 2, 2, 255, 255, 255, 255);
        }
        for (int i = 0; i < ASTEROID_COUNT; i++) {
            draw_asteroid(&asteroids[i], screen);
        }

        SDL_Flip(screen);
    }

    SDL_Quit();

    return 0;
}
