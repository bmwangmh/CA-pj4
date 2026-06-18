#include "game.h"
#include "utils.h"
#include "scoreboard.h"
#include "gd32vf103.h"   /* get_timer_value(), SystemCoreClock */

/* ------------------------------------------------------------------ *
 *  Colours
 * ------------------------------------------------------------------ */
#define COL_BG        BLACK
#define COL_SNAKE     GREEN
#define COL_HEAD      CYAN
#define COL_ENEMY     RED
#define COL_ENEMY_HD  BRRED
#define COL_GOLD      YELLOW
#define COL_GEM       MAGENTA
#define COL_WALL      GRAY
#define COL_PORTAL_A  GBLUE
#define COL_PORTAL_B  BLUE
#define COL_TEXT      WHITE
#define COL_HILITE    YELLOW

/* ------------------------------------------------------------------ *
 *  Tunables
 * ------------------------------------------------------------------ */
#define FRAME_MS       30      /* fixed frame period (3.2 fixed FPS)   */
#define MOVE_MS        180     /* snake step interval (normal speed)   */
#define GOLD_LIFE_MS   10000
#define GEM_LIFE_MS    5000

/* ------------------------------------------------------------------ *
 *  Module state
 * ------------------------------------------------------------------ */
static Snake  snake;
static Snake  enemy;
static int    enemy_alive;
static Item   items[MAX_ITEMS];
static u8     wmap[GRID_H][GRID_W];   /* 1 = static wall              */
static Portal portals[2];
static int    n_portals;
static int    score;
static int    cur_level;

static const int dx[4] = { 0,  1,  0, -1 };  /* UP, RIGHT, DOWN, LEFT */
static const int dy[4] = {-1,  0,  1,  0 };

#define OPP(d)   (((d) + 2) & 3)
#define RIGHTOF(d) (((d) + 1) & 3)
#define LEFTOF(d)  (((d) + 3) & 3)

/* ================================================================== *
 *  Timing
 * ================================================================== */
u32 millis(void) {
    /* delay_1ms shows 1ms == SystemCoreClock/4000 mtime ticks */
    uint64_t ticks = get_timer_value();
    return (u32)(ticks / (SystemCoreClock / 4000));
}

/* ================================================================== *
 *  RNG -- xorshift32
 * ================================================================== */
static u32 rng_state = 2463534242u;

void rng_seed(u32 s) { if (s == 0) s = 1; rng_state = s; }

u32 rng_next(void) {
    u32 x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

int rng_range(int n) { return (int)(rng_next() % (u32)n); }

/* ================================================================== *
 *  Input edge detection (for menus)
 * ================================================================== */
int btn_pressed(int ch) {
    static int prev[7] = {0};
    int cur = Get_Button(ch);
    int hit = cur && !prev[ch];
    prev[ch] = cur;
    return hit;
}

/* ================================================================== *
 *  Rendering primitives
 * ================================================================== */
void draw_cell(int cx, int cy, u16 color) {
    int x = FIELD_X0 + cx * CELL;
    int y = FIELD_Y0 + cy * CELL;
    LCD_Fill(x, y, x + CELL - 1, y + CELL - 1, color);
}

void clear_field(void) {
    LCD_Fill(FIELD_X0, FIELD_Y0, FIELD_X0 + GRID_W * CELL - 1,
             FIELD_Y0 + GRID_H * CELL - 1, COL_BG);
}

void draw_status(int level, int sc) {
    LCD_Fill(0, 0, LCD_W - 1, 15, COL_BG);
    LCD_ShowString(0, 0, (const u8 *)"1-", COL_TEXT);
    LCD_ShowNum(16, 0, level, 1, COL_TEXT);
    LCD_ShowString(56, 0, (const u8 *)"SCORE:", COL_TEXT);
    LCD_ShowNum(104, 0, sc, 3, COL_TEXT);
}

static u16 item_color(int type) { return type == IT_GEM ? COL_GEM : COL_GOLD; }

/* ================================================================== *
 *  Item helpers
 * ================================================================== */
static int count_items(void) {
    int n = 0;
    for (int i = 0; i < MAX_ITEMS; i++) if (items[i].active) n++;
    return n;
}

/* is (x,y) occupied by anything that blocks an item spawn? */
static int cell_taken(int x, int y) {
    if (wmap[y][x]) return 1;
    for (int i = 0; i < snake.len; i++)
        if (snake.body[i].x == x && snake.body[i].y == y) return 1;
    if (enemy_alive)
        for (int i = 0; i < enemy.len; i++)
            if (enemy.body[i].x == x && enemy.body[i].y == y) return 1;
    for (int i = 0; i < n_portals; i++)
        if (portals[i].x == x && portals[i].y == y) return 1;
    for (int i = 0; i < MAX_ITEMS; i++)
        if (items[i].active && items[i].x == x && items[i].y == y) return 1;
    return 0;
}

static int find_free_slot(void) {
    for (int i = 0; i < MAX_ITEMS; i++) if (!items[i].active) return i;
    return -1;
}

/* spawn one randomly placed item (gem only allowed in level 1-2) */
static void spawn_item(void) {
    int slot = find_free_slot();
    if (slot < 0) return;

    int x = 0, y = 0, found = 0;
    for (int tries = 0; tries < 256 && !found; tries++) {
        x = rng_range(GRID_W);
        y = rng_range(GRID_H);
        if (!cell_taken(x, y)) found = 1;
    }
    if (!found) {                       /* fall back to a linear scan */
        for (y = 0; y < GRID_H && !found; y++)
            for (x = 0; x < GRID_W && !found; x++)
                if (!cell_taken(x, y)) found = 1;
        if (!found) return;
        x--; y--;
    }

    int type = IT_GOLD;
    if (cur_level == LV_1_2 && (rng_next() & 1)) type = IT_GEM;

    items[slot].active   = 1;
    items[slot].type     = type;
    items[slot].x        = x;
    items[slot].y        = y;
    items[slot].spawn_ms = millis();
    items[slot].life_ms  = (type == IT_GEM) ? GEM_LIFE_MS : GOLD_LIFE_MS;
    draw_cell(x, y, item_color(type));
}

/* force a gold coin at an exact cell (enemy death drop) */
static void drop_gold(int x, int y) {
    if (cell_taken(x, y)) return;
    int slot = find_free_slot();
    if (slot < 0) return;
    items[slot].active   = 1;
    items[slot].type     = IT_GOLD;
    items[slot].x        = x;
    items[slot].y        = y;
    items[slot].spawn_ms = millis();
    items[slot].life_ms  = GOLD_LIFE_MS;
    draw_cell(x, y, COL_GOLD);
}

static void remove_item(int i) {
    items[i].active = 0;
    draw_cell(items[i].x, items[i].y, COL_BG);
}

/* drop expired items, then guarantee at least one coin is on the field */
static void update_items(void) {
    u32 now = millis();
    for (int i = 0; i < MAX_ITEMS; i++)
        if (items[i].active && now - items[i].spawn_ms >= items[i].life_ms)
            remove_item(i);
    if (count_items() == 0) spawn_item();
}

/* index of nearest gold coin to (hx,hy), or -1 */
static int nearest_gold(int hx, int hy) {
    int best = -1, bestd = 1 << 30;
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (!items[i].active || items[i].type != IT_GOLD) continue;
        int d = abs(items[i].x - hx) + abs(items[i].y - hy);
        if (d < bestd) { bestd = d; best = i; }
    }
    return best;
}

/* ================================================================== *
 *  Geometry helpers
 * ================================================================== */
static int in_bounds(int x, int y) {
    return x >= 0 && x < GRID_W && y >= 0 && y < GRID_H;
}

static int hits_snake(Snake *s, int x, int y, int ignore_tail) {
    int n = ignore_tail ? s->len - 1 : s->len;
    for (int i = 0; i < n; i++)
        if (s->body[i].x == x && s->body[i].y == y) return 1;
    return 0;
}

/* advance a snake: drop tail unless growing, repaint only changed cells */
static void snake_step(Snake *s, int nx, int ny, int grow,
                       u16 body_col, u16 head_col) {
    Point old_head = s->body[0];
    Point old_tail = s->body[s->len - 1];

    if (grow && s->len < MAX_SNAKE) s->len++;
    for (int i = s->len - 1; i > 0; i--) s->body[i] = s->body[i - 1];
    s->body[0].x = nx;
    s->body[0].y = ny;

    if (!grow && !(old_tail.x == nx && old_tail.y == ny))
        draw_cell(old_tail.x, old_tail.y, COL_BG);
    draw_cell(old_head.x, old_head.y, body_col);
    draw_cell(nx, ny, head_col);
}

/* ================================================================== *
 *  Enemy (level 1-3) AI
 * ================================================================== */
/* pick the enemy's next heading: head toward a gold coin if one exists,
 * otherwise cruise; always avoid running off the map, preferring a
 * right turn when forced to turn. Player body is NOT treated as a wall. */
static int enemy_choose_dir(void) {
    int hx = enemy.body[0].x, hy = enemy.body[0].y;
    int cur = enemy.dir;

    int g = nearest_gold(hx, hy);
    if (g >= 0) {
        int tx = items[g].x, ty = items[g].y;
        int order[3] = { RIGHTOF(cur), cur, LEFTOF(cur) };   /* right priority */
        int best = -1, bestd = 1 << 30;
        for (int k = 0; k < 3; k++) {
            int d = order[k];
            int nx = hx + dx[d], ny = hy + dy[d];
            if (!in_bounds(nx, ny)) continue;
            int dist = abs(nx - tx) + abs(ny - ty);
            if (dist < bestd) { bestd = dist; best = d; }
        }
        if (best >= 0) return best;
    }

    /* cruise straight; turn (right first) only when a wall is 1 cell ahead */
    if (in_bounds(hx + dx[cur], hy + dy[cur])) return cur;
    int tryd[3] = { RIGHTOF(cur), LEFTOF(cur), OPP(cur) };
    for (int k = 0; k < 3; k++)
        if (in_bounds(hx + dx[tryd[k]], hy + dy[tryd[k]])) return tryd[k];
    return cur;
}

/* advance the enemy one cell; handles eating coins and its own death */
static void enemy_step(void) {
    enemy.dir = enemy_choose_dir();
    int nx = enemy.body[0].x + dx[enemy.dir];
    int ny = enemy.body[0].y + dy[enemy.dir];
    if (!in_bounds(nx, ny)) return;            /* safety: never leave map */

    /* enemy head into player body -> enemy dies, drops 3 coins */
    if (hits_snake(&snake, nx, ny, 0)) {
        Point b[3];
        for (int i = 0; i < enemy.len; i++) b[i] = enemy.body[i];
        for (int i = 0; i < enemy.len; i++)
            draw_cell(enemy.body[i].x, enemy.body[i].y, COL_BG);
        enemy_alive = 0;
        for (int i = 0; i < 3; i++) drop_gold(b[i].x, b[i].y);
        return;
    }

    /* enemy eats a gold coin (does not grow) */
    for (int i = 0; i < MAX_ITEMS; i++)
        if (items[i].active && items[i].x == nx && items[i].y == ny) {
            remove_item(i);
            break;
        }

    snake_step(&enemy, nx, ny, 0, COL_ENEMY, COL_ENEMY_HD);
}

/* ================================================================== *
 *  Level setup
 * ================================================================== */
static void draw_snake_full(Snake *s, u16 body_col, u16 head_col) {
    for (int i = s->len - 1; i >= 1; i--)
        draw_cell(s->body[i].x, s->body[i].y, body_col);
    draw_cell(s->body[0].x, s->body[0].y, head_col);
}

static void add_wall(int x, int y) {
    wmap[y][x] = 1;
    draw_cell(x, y, COL_WALL);
}

static void level_init(int level) {
    cur_level = level;
    score = 0;
    enemy_alive = 0;
    n_portals = 0;
    for (int i = 0; i < MAX_ITEMS; i++) items[i].active = 0;
    for (int y = 0; y < GRID_H; y++)
        for (int x = 0; x < GRID_W; x++) wmap[y][x] = 0;

    /* player snake: length 3, heading right, mid-left of the field */
    snake.len  = 3;
    snake.dir  = DIR_RIGHT;
    snake.pdir = DIR_RIGHT;
    snake.body[0].x = 3; snake.body[0].y = GRID_H / 2;
    snake.body[1].x = 2; snake.body[1].y = GRID_H / 2;
    snake.body[2].x = 1; snake.body[2].y = GRID_H / 2;

    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, COL_BG);
    draw_status(level, score);

    if (level == LV_1_2) {
        /* four static walls (must not overlap snake / portals / spawns) */
        add_wall(8, 1);
        add_wall(8, 6);
        add_wall(13, 2);
        add_wall(13, 5);
        /* a connected pair of portals */
        portals[0].x = 6;  portals[0].y = 1; portals[0].exit_dir = DIR_DOWN;
        portals[1].x = 15; portals[1].y = 6; portals[1].exit_dir = DIR_UP;
        n_portals = 2;
        draw_cell(portals[0].x, portals[0].y, COL_PORTAL_A);
        draw_cell(portals[1].x, portals[1].y, COL_PORTAL_B);
    }

    if (level == LV_1_3) {
        /* enemy at the point symmetric to the player's start */
        enemy_alive = 1;
        enemy.len = 3;
        enemy.dir = DIR_LEFT;
        int ex = GRID_W - 1 - snake.body[0].x;
        int ey = GRID_H - 1 - snake.body[0].y;
        enemy.body[0].x = ex;     enemy.body[0].y = ey;
        enemy.body[1].x = ex + 1; enemy.body[1].y = ey;
        enemy.body[2].x = ex + 2; enemy.body[2].y = ey;
        draw_snake_full(&enemy, COL_ENEMY, COL_ENEMY_HD);
    }

    draw_snake_full(&snake, COL_SNAKE, COL_HEAD);
    spawn_item();
}

/* ================================================================== *
 *  Main per-level game loop. Returns score on death, -1 on SW1 abort.
 * ================================================================== */
int play_level(int level) {
    level_init(level);
    rng_seed(millis() ^ 0x9E3779B9u);

    u32 last_move = millis();

    for (;;) {
        u32 frame_start = millis();

        /* ---- input ---- */
        if (Get_Button(BUTTON_1)) return -1;            /* forced exit  */

        if (Get_Button(JOY_UP)    && snake.dir != DIR_DOWN)  snake.pdir = DIR_UP;
        if (Get_Button(JOY_DOWN)  && snake.dir != DIR_UP)    snake.pdir = DIR_DOWN;
        if (Get_Button(JOY_LEFT)  && snake.dir != DIR_RIGHT) snake.pdir = DIR_LEFT;
        if (Get_Button(JOY_RIGHT) && snake.dir != DIR_LEFT)  snake.pdir = DIR_RIGHT;

        u32 step = Get_Button(BUTTON_2) ? (MOVE_MS / 2) : MOVE_MS;  /* SW2 = 2x */

        /* coins expire regardless of move cadence */
        update_items();

        /* ---- movement tick ---- */
        if (frame_start - last_move >= step) {
            last_move = frame_start;

            if (snake.pdir != OPP(snake.dir)) snake.dir = snake.pdir;

            int nx = snake.body[0].x + dx[snake.dir];
            int ny = snake.body[0].y + dy[snake.dir];

            /* portal teleport (level 1-2) */
            for (int i = 0; i < n_portals; i++) {
                if (nx == portals[i].x && ny == portals[i].y) {
                    Portal *o = &portals[i ^ 1];
                    snake.dir = o->exit_dir;
                    nx = o->x + dx[o->exit_dir];
                    ny = o->y + dy[o->exit_dir];
                    break;
                }
            }

            /* death: wall / boundary / self / enemy body */
            int dead = 0;
            if (!in_bounds(nx, ny))               dead = 1;
            else if (wmap[ny][nx])                dead = 1;
            else if (hits_snake(&snake, nx, ny, 1)) dead = 1;
            else if (enemy_alive && hits_snake(&enemy, nx, ny, 0)) dead = 1;
            if (dead) return score;

            /* eat? */
            int grow = 0;
            for (int i = 0; i < MAX_ITEMS; i++) {
                if (items[i].active && items[i].x == nx && items[i].y == ny) {
                    score += (items[i].type == IT_GEM) ? 2 : 1;
                    items[i].active = 0;        /* will be overdrawn by head */
                    grow = 1;
                    draw_status(level, score);
                    break;
                }
            }

            snake_step(&snake, nx, ny, grow, COL_SNAKE, COL_HEAD);

            if (level == LV_1_3 && enemy_alive) enemy_step();

            update_items();                     /* may respawn a coin */
        }

        /* ---- fixed FPS pacing (3.2): hold each frame to FRAME_MS ---- */
        while (millis() - frame_start < FRAME_MS) { /* busy wait */ }
    }
}

/* ================================================================== *
 *  Menu
 * ================================================================== */
static void draw_menu_option(int i, int selected) {
    int y = 16 + i * 16;
    char txt[4]; txt[0] = '1'; txt[1] = '-'; txt[2] = (char)('1' + i); txt[3] = 0;
    if (selected) {
        LCD_Fill(40, y, 119, y + 15, COL_HILITE);
        BACK_COLOR = COL_HILITE;
        LCD_ShowString(56, y, (const u8 *)txt, BLACK);
        BACK_COLOR = COL_BG;
    } else {
        LCD_Fill(40, y, 119, y + 15, COL_BG);
        LCD_ShowString(56, y, (const u8 *)txt, COL_TEXT);
    }
}

int menu_loop(void) {
    int sel = 0;
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, COL_BG);
    LCD_ShowString(32, 0, (const u8 *)"SNAKE", COL_HILITE);
    for (int i = 0; i < 3; i++) draw_menu_option(i, i == sel);

    for (;;) {
        if (btn_pressed(JOY_UP))   { sel = (sel + 2) % 3; for (int i = 0; i < 3; i++) draw_menu_option(i, i == sel); }
        if (btn_pressed(JOY_DOWN)) { sel = (sel + 1) % 3; for (int i = 0; i < 3; i++) draw_menu_option(i, i == sel); }
        if (btn_pressed(JOY_CTR))  return sel + 1;
        delay_1ms(FRAME_MS);
    }
}

/* ================================================================== *
 *  Death settlement
 * ================================================================== */
void death_screen(int level, int sc) {
    /* always update the persistent scoreboard on death (spec 1.1) */
    scoreboard_update(level, sc);

    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, COL_BG);
    LCD_ShowString(32, 0,  (const u8 *)"GAME OVER", RED);
    LCD_ShowString(0, 24,  (const u8 *)"SCORE:", COL_TEXT);
    LCD_ShowNum(48, 24, sc, 4, COL_TEXT);
    LCD_ShowString(0, 48,  (const u8 *)"CTR:rank SW1:menu", COL_HILITE);

    for (;;) {
        if (btn_pressed(JOY_CTR)) { scoreboard_show(); return; }  /* view board */
        if (btn_pressed(BUTTON_1)) return;                         /* to menu    */
        delay_1ms(FRAME_MS);
    }
}
