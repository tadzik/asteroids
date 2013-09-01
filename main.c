#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <math.h>
#define WIDTH 1600
#define HEIGHT 900
#define BULLET_COUNT 32

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

void move_bullet(struct Bullet *b)
{
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

    SDL_Surface* screen = SDL_SetVideoMode(WIDTH, HEIGHT, 0, SDL_HWSURFACE | SDL_DOUBLEBUF);
    SDL_WM_SetCaption("Asteroids", 0);
    SDL_EnableKeyRepeat(1, SDL_DEFAULT_REPEAT_INTERVAL);

    SDL_Event event;
    int gameRunning = 1;

    struct Spaceship player = { 400, 300, 40, 0, 0 };
    struct Bullet bullets[BULLET_COUNT];
    int bullet_iter = 0;
    for (int i = 0; i < BULLET_COUNT; i++) {
        bullets[i].x = -1;
        bullets[i].y = -1;
        bullets[i].vel = 0;
    }

    SDL_AddTimer(16, (SDL_NewTimerCallback)timer_cb, NULL);

    while (gameRunning) {
        if (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_USEREVENT:
                for (int i = 0; i < BULLET_COUNT; i++) {
                    move_bullet(bullets + i);
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
            filledEllipseRGBA(screen, bullets[i].x, bullets[i].y, 2, 2, 255, 255, 255, 255);
        }

        SDL_Flip(screen);
    }

    SDL_Quit();

    return 0;
}
