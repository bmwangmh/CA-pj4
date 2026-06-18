#ifndef __GAME_H
#define __GAME_H

#include "lcd/lcd.h"

/* ------------------------------------------------------------------ *
 *  Playfield geometry
 *  Screen is 160(W) x 80(H).  Top 16px = status bar (one 8x16 line).
 *  Playfield: 20 cols x 8 rows of 8x8 cells starting at y=16.
 * ------------------------------------------------------------------ */
#define CELL      8
#define GRID_W    20
#define GRID_H    8
#define FIELD_X0  0
#define FIELD_Y0  16

#define MAX_SNAKE (GRID_W * GRID_H)
#define MAX_ITEMS 16

/* Directions (clockwise order so "turn right" == (dir+1)&3) */
enum { DIR_UP = 0, DIR_RIGHT = 1, DIR_DOWN = 2, DIR_LEFT = 3 };

/* Levels (minor index doubles as "difficulty" for the scoreboard) */
enum { LV_1_1 = 1, LV_1_2 = 2, LV_1_3 = 3 };

/* Item kinds */
enum { IT_NONE = 0, IT_GOLD = 1, IT_GEM = 2 };

typedef struct { int x, y; } Point;

typedef struct {
    Point body[MAX_SNAKE];
    int   len;
    int   dir;       /* current heading */
    int   pdir;      /* pending heading from input */
} Snake;

typedef struct {
    u8  active;
    u8  type;        /* IT_GOLD / IT_GEM */
    int x, y;
    u32 spawn_ms;    /* time this item appeared */
    u32 life_ms;     /* how long it lives */
} Item;

typedef struct {
    int x, y;        /* portal cell */
    int exit_dir;    /* heading the snake leaves with */
} Portal;

/* ------------------------------------------------------------------ *
 *  Timing helpers (built on the N200 mtime counter)
 * ------------------------------------------------------------------ */
u32  millis(void);

/* ------------------------------------------------------------------ *
 *  RNG (xorshift32)
 * ------------------------------------------------------------------ */
void rng_seed(u32 s);
u32  rng_next(void);
int  rng_range(int n);          /* 0 .. n-1 */

/* ------------------------------------------------------------------ *
 *  Input edge detection
 * ------------------------------------------------------------------ */
int  btn_pressed(int ch);       /* 1 only on the frame the button goes down */

/* ------------------------------------------------------------------ *
 *  Rendering primitives
 * ------------------------------------------------------------------ */
void draw_cell(int cx, int cy, u16 color);
void clear_field(void);
void draw_status(int level, int score);

/* ------------------------------------------------------------------ *
 *  High level screens / game loop.  Each returns the final score and
 *  writes the level played through *out_level (for the scoreboard).
 * ------------------------------------------------------------------ */
int  menu_loop(void);                       /* returns chosen level   */
int  play_level(int level);                 /* returns score on death */
void death_screen(int level, int score);    /* handles CTR/SW1 + board */

#endif /* __GAME_H */
