#include <array>
#include <algorithm>
#include <cstdint>
#include <random>
#include <iterator>
#include <cassert>
#include <span>

#include "gba.hpp"
#include "build/heck.hpp"

// I can't properly express how terrible the vast majority of this code is
// It works though (also was made in the span of 3 days)

// [[gnu::section(".ewram")]] std::array<std::uint32_t, 20000> buffer;

// This all obv. needs to be better abstracted later but this is fine for now
void set_obj_tile_info(int obj_no, int palette_number, int tile_no)
{
   volatile std::uint16_t* addr = (std::uint16_t*)(0x7000000 + 0x08 * obj_no);
   *(addr + 2) = (palette_number << 12) | tile_no;
}

void set_obj_x_y(int obj_no, int x, int y)
{
   volatile std::uint16_t* addr = (std::uint16_t*)(0x7000000 + 0x08 * obj_no);
   auto val1 = *addr;
   val1 &= 0xFF00;
   val1 |= y;
   *addr = val1;
   auto val = *(addr + 1);
   val &= 0b1111'1110'0000'0000;
   val |= x;
   *(addr + 1) = val;
}

enum class obj_size {
   s8_8,
   s16_16,
   s32_32,
   s64_64
};

void set_obj_size(int obj_no, obj_size size, bool hflip)
{
   volatile std::uint16_t* addr = (std::uint16_t*)(0x7000000 + 0x08 * obj_no);
   auto val = *(addr + 1);
   val &= 0b0010'1111'1111'1111;
   val |= (static_cast<int>(size) << 14) | (hflip << 12);
   *(addr + 1) = val;
}

enum class style {
   disable,
   enable,
   quarter_size
};

void set_obj_display(int obj_no, style s)
{
   volatile std::uint16_t* addr = (std::uint16_t*)(0x7000000 + 0x08 * obj_no);
   switch (s) {
   case style::disable: *addr = 0b0000'0010'0000'0000; break;
   case style::enable: *addr = 0b0000'0000'0000'0000; break;
   case style::quarter_size: *addr = 0b0000'0001'0000'0000; break;
   }
}

void wait_for_vblank()
{
   // wait for vblank to end
   while ((*(volatile std::uint16_t*)(0x4000004) & 1)) {}
   // wait for vblank to start
   while (!(*(volatile std::uint16_t*)(0x4000004) & 1)) {}
}

struct keypad_pressed_ {
   void update()
   {
      wait_for_vblank();
      raw_val_prev = raw_val;
      raw_val = *(volatile std::uint16_t*)(0x4000130);
   }

   bool a() { return impl(0); }
   bool b() { return impl(1); }
   bool select() { return impl(2); }
   bool start() { return impl(3); }
   bool right() { return impl(4); }
   bool left() { return impl(5); }
   bool up() { return impl(6); }
   bool down() { return impl(7); }
   bool r() { return impl(8); }
   bool l() { return impl(9); }

   bool impl(int bit_no) {
      const auto r_held = (raw_val & (1 << 8)) == 0;
      const auto l_held = (raw_val & (1 << 9)) == 0;
      const std::uint16_t mask = 1 << bit_no;
      // if previously not pressed but now pressed
      return (((raw_val_prev & mask) != 0) || r_held || l_held) && ((raw_val & mask) == 0);
   }

   std::uint16_t raw_val_prev{0xFFFF};
   std::uint16_t raw_val{0xFFFF};
};

keypad_pressed_ keypad_pressed;

bool title(const std::uint16_t* graphics, bool show_options);

constexpr std::uint8_t field_data[] {
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
   1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1,
   1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1,
   1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1,
   1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1,
   1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1,
   1, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1,
   1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 1,
   1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1,
   1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

enum class display_sprite : std::uint8_t {
   snake,
   angry_snake,
   something,
   art_itself,
   none
};

struct move {
   std::array<std::uint8_t, 9> name;
   std::uint16_t power;
   std::uint8_t max_ammo;
};

constexpr std::array moves {
   move{"", 0, 0},
   move{"hiss", 30, 39},
   move{"big hiss", 55, 22},
   move{"agryhiss", 80, 13},
   move{"horror", 500, 3},
   move{"destroy", 1000, 2},
   move{"punch", 300, 20},
   move{"lines", 100, 5},
   move{"dots", 50, 40},
   move{"confse", 57, 23},
   move{"art", 999, 1}
};

struct base_dude {
   std::array<std::uint8_t, 9> name;
   std::uint8_t hp_mult;
   std::uint8_t attack_mult;
   std::uint8_t defense_mult;
   std::uint8_t speed_mult;
   std::array<std::pair<std::uint8_t, std::uint8_t>, 6> moves;
};

constexpr std::array base_dudes {
   base_dude{
      "snake",
      60, 60, 60, 60,
      {{{0, 1}, {10, 2}}}
   },
   base_dude{
      "agry snk",
      40, 100, 40, 80,
      {{{0, 1}, {20, 3}}}
   },
   base_dude{
      "somethng",
      255, 255, 255, 255,
      {{{0, 4}, {0, 5}, {0, 6}}}
   },
   base_dude{
      "art slf",
      80, 120, 80, 120,
      {{{0, 7}, {0, 8}, {0, 9}, {25, 10}}}
   }
};

struct dude {
   std::uint16_t hp;
   std::uint16_t max_hp;
   std::uint16_t attack;
   std::uint16_t defense;
   std::uint16_t speed;
   std::uint16_t level;
   int base_dude_index;
   std::array<std::uint8_t, 6> ammo_used = {};
};

struct map_event {
   std::int8_t x;
   std::int8_t y;
   display_sprite sprite;
   std::span<const dude> line_up;
   bool is_battle = true;
   std::uint8_t sprite_no = 0;
   bool active = true;
};

constexpr dude starter_dude {
   60,
   60,
   15,
   15,
   15,
   3,
   0
};

constexpr dude weak_snake {
   20,
   20,
   12,
   12,
   40,
   1,
   0
};

constexpr dude something_dude {
   20000,
   20000,
   500,
   500,
   40000,
   10000,
   2
};

constexpr dude less_angry_snake_dude {
   30,
   30,
   25,
   5,
   20,
   10,
   1
};

constexpr dude angry_snake_dude {
   60,
   60,
   50,
   10,
   40,
   15,
   1
};

constexpr dude art_dude {
   500,
   500,
   30,
   30,
   20,
   20,
   3
};

constexpr std::array weak_snake_team{weak_snake};
constexpr std::array big_weak_snake_team{
   weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake,
   weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake,
   weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake, weak_snake
};
constexpr std::array snake_team{starter_dude, weak_snake};
constexpr std::array angry_snake_one{less_angry_snake_dude};
constexpr std::array angry_snake_two{weak_snake, starter_dude, angry_snake_dude};
constexpr std::array something_team{something_dude};
constexpr std::array art_team{weak_snake, starter_dude, art_dude};

constexpr std::array map_events {
   map_event{3, 3, display_sprite::snake, {weak_snake_team}},
   map_event{4, 4, display_sprite::snake, {weak_snake_team}},
   map_event{3, 5, display_sprite::snake, {weak_snake_team}},
   map_event{7, 3, display_sprite::snake, {big_weak_snake_team}},
   map_event{5, 3, display_sprite::snake, {weak_snake_team}},
   map_event{6, 4, display_sprite::snake, {snake_team}},
   map_event{12, 7, display_sprite::art_itself, {art_team}},
   map_event{11, 8, display_sprite::angry_snake, {angry_snake_two}},
   map_event{8, 6, display_sprite::angry_snake, {angry_snake_one}},
   map_event{12, 3, display_sprite::something, {something_team}},
   map_event{12, 6, display_sprite::none, {}, false},
};

void pause_until_a()
{
   keypad_pressed.update();
   while(!keypad_pressed.a()) { keypad_pressed.update(); }
}

constexpr std::array<char, 6> to_str(std::uint16_t val) {
   std::array<char, 6> to_ret;
   to_ret.back() = 0;
   int loc = 0;
   bool seen_non_zero = false;
   for (int i = 10000; i > 0; i /= 10) {
      const int char_val = val / i;
      if (char_val > 0) {
         seen_non_zero = true;
         to_ret[loc] = '0' + char_val;
      }
      else {
         // placeholder value for space
         to_ret[loc] = seen_non_zero || i == 1 ? '0' : '_';
      }
      val -= char_val * i;
      ++loc;
   }
   return to_ret;
}

int calc_damage(const dude& attacker, const dude& defender, const move& the_move) {
   static std::minstd_rand0 prng;
   int rand_val = 0;
   while (rand_val < 75 || rand_val > 125) {
      rand_val = prng() % 128;
   }
   const auto result = attacker.attack * the_move.power * rand_val / defender.defense / 100;
   return std::clamp(result, 0, 0xFFFF);
}

void get_experience(dude& slayer, dude& slain) {
   const auto& base_slayer = base_dudes[slayer.base_dude_index];
   const std::uint8_t mults[] {base_slayer.attack_mult, base_slayer.defense_mult, base_slayer.speed_mult, base_slayer.hp_mult, base_slayer.hp_mult};
   const std::uint16_t stats[] {slain.attack, slain.defense, slain.speed, slain.max_hp, slain.max_hp};
   std::uint16_t* add_to[] {&slayer.attack, &slayer.defense, &slayer.speed, &slayer.hp, &slayer.max_hp};
   int gains = 0;
   const int level_diff = slain.level - slayer.level;
   for (int i = 0; i < 5; ++i) {
      const auto amount = std::clamp(stats[i] * mults[i] / 512 + (level_diff / 16), 0, 0xFF);
      gains += amount;
      *(add_to[i]) += amount;
   }
   slayer.level += gains / 6;
}

namespace qs_data {

bool exists = false;
int num_dudes;
std::array<dude, map_events.size()> your_dudes;
std::remove_const_t<decltype(map_events)> events;
int x;
int y;

} // namespace qs_data

void field(bool load_data)
{
   std::array<dude, map_events.size()> your_dudes{starter_dude};
   int num_dudes = 1;
   int active_index = 0;
   using namespace gba::lcd_opt;
   using namespace gba::dma_opt;
   // Set background 0 and 1 to use 16 colors,
   // background 0 is at 60kb offset for map data, background 1 is at 62kb offset
   // both use same tile data at 0kb offset
   // Also lower the priority
   const auto reset_field = [&]() {
      gba::lcd.set_options(gba::lcd_options{}
         .set(bg_mode{0})
         .set(forced_blank::off)
         .set(display_bg0::on)
         .set(display_obj::on)
         .set(display_window_0::off)
         .set(display_window_1::off)
         .set(display_window_obj::off)
         .set(obj_char_mapping::one_dimensional)
      );
      *(volatile std::uint16_t*)(0x4000008) = 0b0001'1110'0000'0011;
      *(volatile std::uint16_t*)(0x400000A) = 0b0001'1111'0000'0010;
   };
   reset_field();
   for (int i = 0; i < 128; ++i) {
      set_obj_display(i, style::disable);
   }
   volatile std::uint16_t* bg_palettes = (std::uint16_t*)0x5000000;
   volatile std::uint16_t* obj_palettes = (std::uint16_t*)0x5000200;
   volatile std::uint32_t* tiles_data = (std::uint32_t*)0x6000000;
   volatile std::uint32_t* obj_tiles = (std::uint32_t*)0x6010000;
   // This is so horrible and I hate it but whatever
   const auto char_offset = 0;
   const auto special_output = std::copy(std::begin(chars), std::end(chars), (std::uint8_t*)tiles_data);
   const auto special_offset = char_offset + std::ssize(chars) / 32;
   const auto num_output = std::copy(std::begin(special_font), std::end(special_font), (std::uint8_t*)special_output);
   const auto num_offset = special_offset + std::ssize(special_font) / 32;
   const auto tile_output = std::copy(std::begin(numbers), std::end(numbers), (std::uint8_t*)num_output);
   const auto tile_offset = num_offset + std::ssize(numbers) / 32;
   std::copy(std::begin(tiles), std::end(tiles), (std::uint8_t*)tile_output);
   std::copy(std::begin(snake_pal), std::end(snake_pal), bg_palettes);
   std::copy(std::begin(chars_pal), std::end(chars_pal), bg_palettes + 0x10);
   std::copy(std::begin(tiles_pal), std::end(tiles_pal), bg_palettes + 0x20);
   // obj palettes
   std::copy(std::begin(snake_pal), std::end(snake_pal), obj_palettes);
   std::copy(std::begin(angry_snake_pal), std::end(angry_snake_pal), obj_palettes + 0x10);
   std::copy(std::begin(something_pal), std::end(something_pal), obj_palettes + 0x20);
   std::copy(std::begin(art_itself_pal), std::end(art_itself_pal), obj_palettes + 0x30);
   std::copy(std::begin(you_pal), std::end(you_pal), obj_palettes + 0x40);
   // obj tiles
   const auto obj_tile_size = 2048;
   // const auto snake_offset = 0;
   // const auto angry_snake_offset = 64;
   // const auto something_offset = 64 * 2;
   // const auto art_itself_offset = 64 * 3;
   const auto you_offset = 64 * 4;
   std::copy(std::begin(snake), std::end(snake), (std::uint8_t*)obj_tiles);
   std::copy(std::begin(angry_snake), std::end(angry_snake), ((std::uint8_t*)obj_tiles) + obj_tile_size);
   std::copy(std::begin(something), std::end(something), ((std::uint8_t*)obj_tiles) + obj_tile_size * 2);
   std::copy(std::begin(art_itself), std::end(art_itself), ((std::uint8_t*)obj_tiles) + obj_tile_size * 3);
   std::copy(std::begin(you), std::end(you), ((std::uint8_t*)obj_tiles) + obj_tile_size * 4);
   // set_obj_display(0, style::quarter_size);
   // set_obj_tile_info(0, 0, snake_offset);
   // set_obj_size(0, obj_size::s64_64, false);
   // set_obj_x_y(0, 24, 24);
   set_obj_display(1, style::enable);
   set_obj_tile_info(1, 4, you_offset);
   set_obj_size(1, obj_size::s16_16, false);
   set_obj_x_y(1, 16, 16);
   for (int y = 0; y < 10; ++y) {
      for (int x = 0; x < 15; ++x) {
         const auto i = y * 15 + x;
         auto base_loc = ((std::uint16_t*)((std::uint8_t*)tiles_data + 0x800 * 30 + x * 4 + y * 128));
         base_loc[0] = (2 << 12) + field_data[i] * 2 + tile_offset;
         base_loc[1] = (2 << 12) + field_data[i] * 2 + tile_offset + 1;
         base_loc[32] = (2 << 12) + field_data[i] * 2 + tile_offset + 4;
         base_loc[33] = (2 << 12) + field_data[i] * 2 + tile_offset + 5;
      }
   }
   const auto bg1_loc = [&](int x, int y) { return ((std::uint16_t*)((std::uint8_t*)tiles_data + 0x800 * 31 + x * 2 + y * 0x40)); };
   const auto init_battle_screen = [&](){
      for (int y = 0; y < 20; ++y) {
         for (int x = 0; x < 30; ++x) {
            *bg1_loc(x, y) = (1 << 12) + special_offset + 12;
         }
         *bg1_loc(20, y) = (1 << 12) + special_offset + 8;
      }
      for (int y = 14; y < 20; ++y) {
         *bg1_loc(14, y) = (1 << 12) + special_offset + 8;
      }
      for (int x = 0; x < 20; ++x) {
         *bg1_loc(x, 13) = (1 << 12) + special_offset + 9;
      }
      *bg1_loc(14, 13) = (1 << 12) + special_offset + 10;
      *bg1_loc(20, 13) = (1 << 12) + special_offset + 11;
      *bg1_loc(20, 11) = (1 << 12) + special_offset + 13;
      *bg1_loc(20, 18) = (1 << 12) + special_offset + 13;
      for (int x = 21; x < 30; ++x) {
         *bg1_loc(x, 11) = (1 << 12) + special_offset + 9;
         *bg1_loc(x, 18) = (1 << 12) + special_offset + 9;
      }
   };
   init_battle_screen();
   int sprite_no = 2;
   int char_x = 1;
   int char_y = 1;
   auto events = map_events;
   if (load_data) {
      your_dudes = qs_data::your_dudes;
      char_x = qs_data::x;
      char_y = qs_data::y;
      events = qs_data::events;
      num_dudes = qs_data::num_dudes;
   }
   for (auto& ev : events) {
      ev.sprite_no = sprite_no;
      const int index = static_cast<int>(ev.sprite);
      if (ev.active) {
         set_obj_display(sprite_no, style::quarter_size);
         set_obj_tile_info(sprite_no, index, 64 * index);
         set_obj_size(sprite_no, obj_size::s64_64, false);
         set_obj_x_y(sprite_no, 8 + (ev.x - 2) * 16, 8 + (ev.y - 2) * 16);
      }
      ++sprite_no;
   }
   const auto battle = [&](std::span<dude> enemies) -> bool {
      const auto log_x = 21;
      const auto stat_y = 12;
      const auto disp_text = [&](int x, int y, const void* text_raw) {
         const char* text = (const char*)text_raw;
         const auto base_x = x;
         for (int i = 0; text[i]; ++i) {
            const auto c = text[i];
            if (c >= 'a' && c <= 'z') {
               *bg1_loc(x, y) = (1 << 12) + c + char_offset - 'a';
            }
            else if (c >= '0' && c <= '9') {
               *bg1_loc(x, y) = (1 << 12) + c + num_offset - '0';
            }
            else if (c == '\n') {
               x = base_x - 1;
               ++y;
            }
            else {
               constexpr std::array comp{'/', '.', '>', 'X', 'L', '*', '\'', ','};
               bool okay = false;
               for (unsigned z = 0; z < comp.size(); ++z) {
                  if (c == comp[z]) {
                     *bg1_loc(x, y) = (1 << 12) + special_offset + z;
                     okay = true;
                     break;
                  }
               }
               if (!okay) {
                  *bg1_loc(x, y) = (1 << 12) + special_offset + 12;
               }
            }
            ++x;
         }
      };
      const auto you_sprite = events.back().sprite_no + 1;
      const auto enemy_sprite = you_sprite + 1;
      const auto disp_dude = [&](const dude& d, bool is_enemy) {
         const auto sprite_index = is_enemy ? enemy_sprite : you_sprite;
         set_obj_display(sprite_index, style::enable);
         set_obj_size(sprite_index, obj_size::s64_64, !is_enemy);
         set_obj_tile_info(sprite_index, d.base_dude_index, 64 * d.base_dude_index);
         set_obj_x_y(sprite_index, is_enemy ? 88 : 0, 24);
      };
      const auto disp_dude_stats = [&](const dude& d, bool is_enemy) {
         const auto base_x = is_enemy ? 11 : 0;
         const auto base_y = is_enemy ? stat_y + 5 : stat_y + 2;
         disp_text(base_x, 0, base_dudes[d.base_dude_index].name.data());
         disp_text(log_x, base_y, to_str(d.attack).data() + 1);
         disp_text(log_x + 4, base_y, to_str(d.defense).data());
         disp_text(base_x, 2, "L");
         disp_text(base_x + 1, 2, to_str(d.level).data());
         disp_text(base_x + 3, 11, to_str(d.hp).data());
         disp_text(base_x + 3, 12, "/");
         disp_text(base_x + 4, 12, to_str(d.max_hp).data());
      };
      init_battle_screen();
      keypad_pressed.update();
      gba::lcd.set_options(gba::lcd_options{}
         .set(display_bg1::on)
         .set(display_obj::on)
         .set(obj_char_mapping::one_dimensional)
      );
      // Notice to "help" with changing
      if (num_dudes > 1) {
         disp_text(log_x, 0,
            "with a\n"
            "left\n"
            "  maybe,,\n"
            "  right\n"
            " one can\n"
            "do a\n"
            "   change"
         );
      }
      disp_text(log_x - 6, stat_y + 2,
         "pow\n"
         "\n"
         "mult\n"
         " 1.0X\n"
         "ammo\n"
         "  /"
      );
      disp_text(log_x, stat_y,
         "left\n"
         "atk  def\n"
         "\n"
         "right\n"
         "atk  def\n"
         "\n"
         "\n"
         "faster"
      );
      int active_enemy = 0;
      const auto disp_all_stats = [&](const dude& yours, const dude& theirs) {
         disp_dude(yours, false);
         disp_dude(theirs, true);
         disp_dude_stats(yours, false);
         disp_dude_stats(theirs, true);
         disp_text(0, 3, "num");
         disp_text(4, 3, to_str(active_index + 1).data() + 3);
         disp_text(11, 3, "num");
         disp_text(15, 3, to_str(active_enemy + 1).data() + 3);
         char faster_to_write[2] {'\0', '\0'};
         faster_to_write[0] = yours.speed > theirs.speed ? 'l' : 'r';
         disp_text(log_x + 8, 19, faster_to_write);
      };
      const auto calc_num_moves = [&](const dude& d) {
         const auto moves = base_dudes[d.base_dude_index].moves;
         int num_moves = 0;
         for (const auto& m : moves) {
            if (m.second == 0 || m.first > d.level) { break; }
            ++num_moves;
         }
         return num_moves;
      };
      const auto move_y = stat_y + 2;
      while (true) {
         const auto clear_log = [&]() {
            // laziest/easiest way to clear the log
            disp_text(log_x, 0,
               "         \n"
               "         \n"
               "         \n"
               "         \n"
               "         \n"
               "         \n"
               "         \n"
               "         \n"
               "         \n"
               "         \n"
               "         \n"
            );
         };
         int move_index = 0;
         disp_all_stats(your_dudes[active_index], enemies[active_enemy]);
         auto& me = your_dudes[active_index];
         const auto disp_move_status = [&](const move& m) {
            disp_text(log_x - 6, move_y + 1, to_str(m.power).data());
            const auto ammo_left = m.max_ammo - me.ammo_used[move_index];
            disp_text(log_x - 6, move_y + 5, to_str(ammo_left).data() + 3);
            disp_text(log_x - 3, move_y + 5, to_str(m.max_ammo).data() + 3);
         };
         if (enemies[active_enemy].hp == 0) {
            get_experience(me, enemies[active_enemy]);
            for (int i = 0; i < num_dudes; ++i) {
               get_experience(your_dudes[i], enemies[active_enemy]);
            }
            disp_all_stats(your_dudes[active_index], enemies[active_enemy]);
            if (active_enemy != std::ssize(enemies) - 1) {
               active_enemy += 1;
               disp_text(0, move_y,
                  "next dude\n"
                  "  is     \n"
                  "         \n"
                  "         \n"
                  "         \n"
               );
               const auto index = enemies[active_enemy].base_dude_index;
               disp_text(0, move_y + 2, base_dudes[index].name.data());
               set_obj_display(enemy_sprite, style::disable);
               pause_until_a();
               disp_text(0, move_y,
                  "         \n"
                  "         \n"
                  "         \n"
                  "         \n"
                  "         \n"
               );
               continue;
            }
            break;
         }
         if (me.hp == 0) {
            disp_text(0, move_y,
               "the dude \n"
               "         \n"
               "has a    \n"
               "    fall \n"
               "         \n"
            );
            get_experience(enemies[active_enemy], me);
            for (int i = active_enemy; i < std::ssize(enemies); ++i) {
               get_experience(enemies[i], me);
            }
            disp_all_stats(your_dudes[active_index], enemies[active_enemy]);
            set_obj_display(you_sprite, style::disable);
            disp_text(0, move_y + 1, base_dudes[me.base_dude_index].name.data());
            pause_until_a();
            std::swap(me, your_dudes[num_dudes - 1]);
            active_index = 0;
            num_dudes -= 1;
            disp_text(0, move_y,
               "         \n"
               "         \n"
               "         \n"
               "         \n"
               "         \n"
            );
            if (num_dudes == 0) {
               disp_text(0, move_y,
                  "you losed\n"
                  "         \n"
                  "         \n"
                  "         \n"
                  "         \n"
               );
               pause_until_a();
               return false;
            }
            continue;
         }
         disp_text(0, move_y,
            "         \n"
            "         \n"
            "         \n"
            "         \n"
            "         \n"
         );
         const auto num_moves = calc_num_moves(me);
         for (int i = 0; i < num_moves; ++i) {
            const auto index = base_dudes[me.base_dude_index].moves[i].second;
            if (i == 0) {
               disp_move_status(moves[index]);
            }
            disp_text(1, move_y + i, moves[index].name.data());
         }
         // Move selection
         disp_text(0, move_y, ">");
         const auto keep_going = [&]() {
            keypad_pressed.update();
            while (!keypad_pressed.a()) {
               keypad_pressed.update();
               const auto old_index = move_index;
               if (keypad_pressed.right()) {
                  active_index += 1;
                  active_index %= num_dudes;
                  return false;
               }
               if (keypad_pressed.left()) {
                  if (active_index == 0) {
                     active_index = num_dudes;
                  }
                  active_index -= 1;
                  return false;
               }
               if (keypad_pressed.up()) { --move_index; }
               if (keypad_pressed.down()) { ++move_index; }
               if (move_index < 0 || move_index >= num_moves) {
                  move_index = old_index;
               }
               else if (move_index != old_index) {
                  const auto base_move_index = base_dudes[me.base_dude_index].moves[move_index].second;
                  disp_text(0, move_y + old_index, " ");
                  disp_text(0, move_y + move_index, ">");
                  disp_move_status(moves[base_move_index]);
               }
            }
            disp_text(0, move_y + move_index, " ");
            return true;
         }();
         if (!keep_going) { continue; }
         clear_log();
         int output_y = 0;
         const auto do_attack = [&](dude& attacker, dude& defender, int move_used, bool is_enemy) {
            const auto& attack_used = moves[base_dudes[attacker.base_dude_index].moves[move_used].second];
            char to_disp[3] = {'*', ' ', '\0'};
            to_disp[1] = is_enemy ? 'r' : 'l';
            disp_text(log_x, output_y, to_disp);
            disp_text(log_x + 1, output_y + 1, base_dudes[attacker.base_dude_index].name.data());
            const int damage = [&]() {
               if (attacker.ammo_used[move_used] >= attack_used.max_ammo) {
                  disp_text(log_x, output_y + 2, "*he don't");
                  return 1;
               }
               attacker.ammo_used[move_used] += 1;
               disp_text(log_x, output_y + 2, "*");
               disp_text(log_x + 1, output_y + 2, attack_used.name.data());
               return calc_damage(attacker, defender, attack_used);
            }();
            disp_text(log_x, output_y + 3, "*");
            disp_text(log_x + 1, output_y + 3, to_str(damage).data());
            disp_text(log_x + 1, output_y + 4, "damage");
            if (damage > defender.hp) {
               disp_text(log_x + 7, output_y + 3, "ow");
               defender.hp = 0;
            }
            else {
               defender.hp -= damage;
            }
            output_y += 6;
         };
         const auto process_attacks = [&](dude& first, dude& second, int first_attack, int second_attack) {
            do_attack(first, second, first_attack, &first != &me);
            if (second.hp > 0) {
               do_attack(second, first, second_attack, &second != &me);
            }
         };
         static std::minstd_rand0 prng;
         const auto num_enemy_moves = calc_num_moves(enemies[active_enemy]);
         // random move but try to find something with ammo
         auto enemy_attack = prng() % num_enemy_moves;
         // for (int i = 0; i < num_enemy_moves; ++i) {
         //    const auto new_move = (enemy_attack + i) % num_enemy_moves;
         //    const auto move_index = base_dudes[enemies[active_enemy].base_dude_index].moves[new_move].second;
         //    if (enemies[active_enemy].ammo_used[new_move] <= moves[move_index].max_ammo) {
         //       enemy_attack = new_move;
         //    }
         // }
         if (me.speed > enemies[active_enemy].speed) {
            process_attacks(me, enemies[active_enemy], move_index, enemy_attack);
         }
         else {
            process_attacks(enemies[active_enemy], me, enemy_attack, move_index);
         }
      }
      set_obj_display(enemy_sprite, style::disable);
      disp_text(0, move_y,
         "you winned\n"
         "         \n"
         "it is    \n"
         "         \n"
         "you got  \n"
         "         \n"
      );
      disp_text(0, move_y + 3, base_dudes[enemies.back().base_dude_index].name.data());
      your_dudes[num_dudes] = enemies.back();
      your_dudes[num_dudes].hp = your_dudes[num_dudes].max_hp;
      for (auto& c : your_dudes[num_dudes].ammo_used) {
         c = 0;
      }
      num_dudes += 1;
      pause_until_a();
      set_obj_display(you_sprite, style::disable);
      set_obj_display(enemy_sprite, style::disable);
      return true;
   };

   while (true) {
      keypad_pressed.update();
      const auto last_x = char_x;
      const auto last_y = char_y;
      if (keypad_pressed.down())  { char_y += 1; }
      if (keypad_pressed.up())    { char_y -= 1; }
      if (keypad_pressed.right()) { char_x += 1; }
      if (keypad_pressed.left())  { char_x -= 1; }
      if (field_data[char_x + char_y * 15]) {
         char_x = last_x;
         char_y = last_y;
      }
      set_obj_x_y(1, char_x * 16, char_y * 16);
      for (auto& ev : events) {
         if (ev.active) {
            if (ev.x == char_x && ev.y == char_y) {
               if (!ev.is_battle) {
                  title(won_screen, false);
               }
               qs_data::exists = true;
               qs_data::your_dudes = your_dudes;
               qs_data::x = last_x;
               qs_data::y = last_y;
               qs_data::events = events;
               qs_data::num_dudes = num_dudes;
               ev.active = false;
               set_obj_display(1, style::disable);
               for (const auto& ev2 : events) {
                  set_obj_display(ev2.sprite_no, style::disable);
               }
               set_obj_display(ev.sprite_no, style::enable);
               set_obj_x_y(ev.sprite_no, 120 - 32, 80 - 32);
               pause_until_a();
               set_obj_display(ev.sprite_no, style::disable);
               std::array<dude, 32> temp_dudes;
               std::copy(ev.line_up.begin(), ev.line_up.end(), temp_dudes.begin());
               if(!battle({temp_dudes.begin(), ev.line_up.size()})) { return; }
               reset_field();
               set_obj_display(1, style::enable);
               for (const auto& ev2 : events) {
                  set_obj_display(ev2.sprite_no, ev2.active ? style::quarter_size : style::disable);
                  set_obj_x_y(ev2.sprite_no, 8 + (ev2.x - 2) * 16, 8 + (ev2.y - 2) * 16);
               }
               set_obj_display(ev.sprite_no, style::disable);
            }
         }
      }
   }
}

bool title(const std::uint16_t* graphics, bool show_options)
{
   for (int i = 0; i < 128; ++i) {
      set_obj_display(i, style::disable);
   }
   volatile std::uint32_t* vram = (std::uint32_t*)0x6000000;
   using namespace gba::lcd_opt;
   using namespace gba::dma_opt;
   gba::lcd.set_options(gba::lcd_options{}
      .set(bg_mode{3})
      .set(forced_blank::off)
      .set(display_bg2::on)
      .set(display_obj::on)
      .set(display_window_0::off)
      .set(display_window_1::off)
      .set(display_window_obj::off)
      .set(obj_char_mapping::one_dimensional)
   );
   gba::dma3.set_source(graphics);
   gba::dma3.set_destination((void*)vram);
   gba::dma3.set_word_count(std::size(title_screen) / 2);
   gba::dma3.set_options(gba::dma_options{}
      .set(dest_addr_cntrl::increment)
      .set(source_addr_cntrl::increment)
      .set(repeat::off)
      .set(transfer_type::bits_32)
      .set(start_timing::vblank)
      .set(irq::disable)
      .set(enable::on)
   );
   // make background lower priority than sprites
   volatile std::uint16_t* bg2_control = (std::uint16_t*)0x400000C;
   auto temp_val = *bg2_control;
   temp_val |= 3;
   *bg2_control = temp_val;
   if (!show_options) {
      while(true) {}
   }
   // copy tile and palette data
   volatile std::uint32_t* obj_tiles = (std::uint32_t*)0x6014000;
   volatile std::uint16_t* obj_palettes = (std::uint16_t*)0x5000200;
   const auto char_output = std::copy(std::begin(snake), std::end(snake), (std::uint8_t*)obj_tiles);
   const auto special_output = std::copy(std::begin(chars), std::end(chars), (std::uint8_t*)char_output);
   std::copy(std::begin(special_font), std::end(special_font), (std::uint8_t*)special_output);
   std::copy(std::begin(snake_pal), std::end(snake_pal), obj_palettes);
   std::copy(std::begin(chars_pal), std::end(chars_pal), obj_palettes + 0x10);
   // Set transformation zero to quarter size
   *(std::uint16_t*)(0x7000006) = 0b0000'0100'0000'0000;
   *(std::uint16_t*)(0x700001E) = 0b0000'0100'0000'0000;
   // Display text options (horribly hackish)
   constexpr std::array<const char*, 3> strings{"new", "continue", "quick load"};
   int x = 15;
   int y = 2;
   int n = 2;
   for (unsigned s = 0; s < strings.size(); ++s) {
      for (int i = 0; strings[s][i] != '\0'; ++i) {
         if (strings[s][i] < 'a' || strings[s][i] > 'z') {
            ++x;
            continue;
         }
         set_obj_display(n, style::enable);
         set_obj_tile_info(n, 1, 512 + 64 + (strings[s][i] - 'a'));
         set_obj_size(n, obj_size::s8_8, false);
         set_obj_x_y(n, x * 8, y * 8);
         ++n;
         ++x;
      }
      ++y;
      x = 15;
   }
   // Arrow (also horribly hackish)
   set_obj_display(1, style::enable);
   set_obj_tile_info(1, 1, 512 + 64 + 26 + 2);
   set_obj_size(1, obj_size::s8_8, false);
   set_obj_x_y(1, 14 * 8, 2 * 8);
   int arrow_y = qs_data::exists ? 4 : 2;
   while (true) {
      keypad_pressed.update();
      if (keypad_pressed.down()) {
         arrow_y += 1;
      }
      else if (keypad_pressed.up()) {
         arrow_y -= 1;
      }
      if (keypad_pressed.a()) {
         if (arrow_y == 2) { return false; }
         if (arrow_y == 4 && qs_data::exists) { return true; }
      }
      arrow_y = std::clamp(arrow_y, 2, 4);
      set_obj_x_y(1, 14 * 8, arrow_y * 8);
   }
}

int main()
{
   while (true) {
      field(title(title_screen, true));
   }
}
