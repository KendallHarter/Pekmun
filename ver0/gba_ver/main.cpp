#include <array>
#include <algorithm>
#include <cstdint>
#include <random>
#include <iterator>
#include <cassert>

#include "gba.hpp"
#include "build/heck.hpp"

// [[gnu::section(".ewram")]] std::array<std::uint32_t, 20000> buffer;

int main()
{
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
   gba::dma3.set_source(title_screen);
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
   volatile std::uint16_t* bg2_control = (std::uint16_t*)0x400000C;
   *bg2_control |= 3;
   volatile std::uint16_t* obj_attr  = (std::uint16_t*)0x7000000;
   volatile std::uint32_t* obj_tiles = (std::uint32_t*)0x6014000;
   volatile std::uint16_t* obj_palettes = (std::uint16_t*)0x5000200;
   *obj_attr = 0b0000'0001'0000'0001;
   *(volatile std::uint16_t*)(0x7000002) = 0b1100'0000'0000'1000;
   *(volatile std::uint16_t*)(0x7000004) = 0b0000'0010'0000'0000;
   std::minstd_rand0 prng;
   // for (int i = 0; i < 2000; ++i) {
   //    obj_tiles[i] = prng();
   // }
   // for (int i = 0; i < 256; ++i) {
   //    obj_palettes[i] = prng();
   // }
   std::copy(std::begin(snake), std::end(snake), (std::uint8_t*)obj_tiles);
   std::copy(std::begin(snake_pal), std::end(snake_pal), obj_palettes);
   // Scaling testing
   *(std::uint16_t*)(0x7000006) = 0b0000'0100'0000'0000;
   *(std::uint16_t*)(0x700001E) = 0b0000'0100'0000'0000;
   while (true) {
      volatile std::uint16_t* keys = (std::uint16_t*)0x4000130;
      if ((*keys & 1) == 0) {
         *obj_attr = 0b0000'0001'0000'0001;
      }
      else {
         *obj_attr = 0b0000'0000'0000'0001;
      }
      // std::generate(buffer.begin(), buffer.end(), [&](){ return prng(); });
   }
}
