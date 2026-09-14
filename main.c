#include <eadk.h>
#include <stdint.h>
#include <stdbool.h>

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Demineur";
const uint32_t eadk_api_level __attribute__((section(".rodata.eadk_api_level"))) = 0;

#define W 8
#define H 8
#define MINES 10
#define CELL 24
#define GX 64
#define GY 18

static uint8_t mine[H][W];
static uint8_t open_cell[H][W];
static uint8_t flag_cell[H][W];
static uint8_t count_cell[H][W];

static uint32_t rng_state = 0xA341316Cu;

static uint32_t rnd(void) {
  rng_state ^= rng_state << 13;
  rng_state ^= rng_state >> 17;
  rng_state ^= rng_state << 5;
  return rng_state;
}

static void wait_release(void) {
  while (eadk_keyboard_scan() != 0) {
    eadk_timing_msleep(15);
  }
}

static bool inside(int x, int y) {
  return x >= 0 && x < W && y >= 0 && y < H;
}

static void make_board(int first_x, int first_y) {
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      mine[y][x] = 0;
      open_cell[y][x] = 0;
      flag_cell[y][x] = 0;
      count_cell[y][x] = 0;
    }

  int placed = 0;
  while (placed < MINES) {
    int x = (int)(rnd() % W);
    int y = (int)(rnd() % H);
    if (mine[y][x]) continue;
    if (x == first_x && y == first_y) continue;
    mine[y][x] = 1;
    placed++;
  }

  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      int n = 0;
      for (int dy = -1; dy <= 1; dy++)
        for (int dx = -1; dx <= 1; dx++)
          if ((dx || dy) && inside(x + dx, y + dy) && mine[y + dy][x + dx])
            n++;
      count_cell[y][x] = (uint8_t)n;
    }
  }
}

static void draw_cell(int x, int y, bool opened) {
  eadk_rect_t r = {(uint16_t)(GX + x * CELL), (uint16_t)(GY + y * CELL), CELL - 1, CELL - 1};
  if (!opened) {
    eadk_display_push_rect_uniform(r, eadk_color_light_gray);
    if (flag_cell[y][x])
      eadk_display_draw_string("F", (eadk_point_t){(uint16_t)(r.x + 8), (uint16_t)(r.y + 3)}, true,
                               eadk_color_red, eadk_color_light_gray);
    return;
  }

  eadk_display_push_rect_uniform(r, eadk_color_white);
  if (mine[y][x]) {
    eadk_display_draw_string("*", (eadk_point_t){(uint16_t)(r.x + 7), (uint16_t)(r.y + 2)}, true,
                             eadk_color_black, eadk_color_white);
  } else if (count_cell[y][x]) {
    char s[2] = {(char)('0' + count_cell[y][x]), '\0'};
    eadk_display_draw_string(s, (eadk_point_t){(uint16_t)(r.x + 8), (uint16_t)(r.y + 3)}, true,
                             eadk_color_blue, eadk_color_white);
  }
}

static void draw_board(void) {
  eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_white);
  eadk_display_draw_string("DEMINER  8x8  10 mines", (eadk_point_t){8, 2}, true,
                           eadk_color_black, eadk_color_white);

  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      draw_cell(x, y, open_cell[y][x]);

  eadk_display_draw_string("OK: ouvrir   SHIFT: drapeau   BACK: quitter",
                           (eadk_point_t){8, 214}, true, eadk_color_black, eadk_color_white);
}

static void reveal_from(int sx, int sy) {
  int qx[W * H], qy[W * H];
  int head = 0, tail = 0;
  qx[tail] = sx; qy[tail++] = sy;

  while (head < tail) {
    int x = qx[head], y = qy[head++];
    if (!inside(x, y) || open_cell[y][x] || flag_cell[y][x]) continue;
    open_cell[y][x] = 1;
    if (count_cell[y][x] != 0) continue;

    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++)
        if (dx || dy)
          if (inside(x + dx, y + dy) && !open_cell[y + dy][x + dx] && !mine[y + dy][x + dx]) {
            qx[tail] = x + dx;
            qy[tail++] = y + dy;
          }
  }
}

static bool won(void) {
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++)
      if (!mine[y][x] && !open_cell[y][x])
        return false;
  return true;
}

static void show_end(const char *msg) {
  eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_white);
  eadk_display_draw_string(msg, (eadk_point_t){90, 95}, true, eadk_color_black, eadk_color_white);
  eadk_display_draw_string("OK: nouvelle partie   BACK: quitter",
                           (eadk_point_t){35, 125}, true, eadk_color_black, eadk_color_white);
}

int main(void) {
  int cx = 0, cy = 0;

  while (true) {
    make_board(cx, cy);
    bool first = true;
    bool game_over = false;

    while (!game_over) {
      if (first) {
        draw_board();
        eadk_display_draw_string("Choisis la premiere case", (eadk_point_t){75, 200}, true,
                                 eadk_color_black, eadk_color_white);
      } else {
        draw_board();
      }

      eadk_rect_t cursor = {(uint16_t)(GX + cx * CELL), (uint16_t)(GY + cy * CELL), CELL - 1, CELL - 1};
      eadk_display_draw_rect(cursor, eadk_color_red);

      eadk_keyboard_state_t k = eadk_keyboard_scan();

      if (eadk_keyboard_key_down(k, eadk_key_back)) {
        return 0;
      }
      if (eadk_keyboard_key_down(k, eadk_key_left) && cx > 0) { cx--; wait_release(); }
      else if (eadk_keyboard_key_down(k, eadk_key_right) && cx < W - 1) { cx++; wait_release(); }
      else if (eadk_keyboard_key_down(k, eadk_key_up) && cy > 0) { cy--; wait_release(); }
      else if (eadk_keyboard_key_down(k, eadk_key_down) && cy < H - 1) { cy++; wait_release(); }
      else if (eadk_keyboard_key_down(k, eadk_key_shift)) {
        if (!open_cell[cy][cx]) flag_cell[cy][cx] = !flag_cell[cy][cx];
        wait_release();
      }
      else if (eadk_keyboard_key_down(k, eadk_key_ok)) {
        if (!flag_cell[cy][cx]) {
          if (first) {
            make_board(cx, cy);
            first = false;
          }
          if (mine[cy][cx]) {
            for (int y = 0; y < H; y++)
              for (int x = 0; x < W; x++)
                if (mine[y][x]) open_cell[y][x] = 1;
            draw_board();
            show_end("PERDU !");
            game_over = true;
          } else {
            reveal_from(cx, cy);
            first = false;
            if (won()) {
              for (int y = 0; y < H; y++)
                for (int x = 0; x < W; x++)
                  if (mine[y][x]) flag_cell[y][x] = 1;
              draw_board();
              show_end("GAGNE !");
              game_over = true;
            }
          }
        }
        wait_release();
      }

      eadk_timing_msleep(30);
    }

    while (true) {
      eadk_keyboard_state_t k = eadk_keyboard_scan();
      if (eadk_keyboard_key_down(k, eadk_key_back)) return 0;
      if (eadk_keyboard_key_down(k, eadk_key_ok)) {
        wait_release();
        break;
      }
      eadk_timing_msleep(30);
    }
  }
}
