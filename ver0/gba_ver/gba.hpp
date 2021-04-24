#ifndef GBA_HPP
#define GBA_HPP

#include <concepts>
#include <cstdint>
#include <span>

#ifdef NDEBUG
#define GBA_ASSERT(cond) (void)(cond)
#else
// TODO: Improve this; currently just hangs if condition is true
#define GBA_ASSERT(cond) do { while(cond){} } while(0)
#endif

namespace gba {

constexpr bool is_internal_memory(std::uintptr_t loc) noexcept
{
   return loc < 0x0800'0000;
}

constexpr bool is_external_memory(std::uintptr_t loc) noexcept
{
   return loc >= 0x0800'0000 && loc < 0x1000'0000;
}

constexpr std::uintptr_t ewram_start{0x2000000};
constexpr std::uintptr_t ewram_end{ewram_start + 0x40000};

constexpr std::uintptr_t iwram_start{0x4000000};
constexpr std::uintptr_t iwram_end{iwram_start + 0x8000};

constexpr std::uintptr_t game_pack_start{0x8000000};
constexpr std::uintptr_t game_pack_end{game_pack_start + 0x6000000};

namespace lcd_opt {

enum class bg_mode {
   mode_0,
   mode_1,
   mode_2,
   mode_3,
   mode_4,
   mode_5
};

enum class frame_select {
   frame_0,
   frame_1
};

enum class oam_access_during_hblank {
   disabled,
   enabled
};

enum class obj_char_mapping {
   two_dimensional,
   one_dimensional
};

enum class forced_blank {
   off,
   on
};

enum class display_bg0 {
   off,
   on
};

enum class display_bg1 {
   off,
   on
};

enum class display_bg2 {
   off,
   on
};

enum class display_bg3 {
   off,
   on
};

enum class display_obj {
   off,
   on
};

enum class display_window_0 {
   off,
   on
};

enum class display_window_1 {
   off,
   on
};

enum class display_window_obj {
   off,
   on
};

} // namespace gba::lcd_opt

namespace dma_opt {

enum class dest_addr_cntrl {
   increment,
   decrement,
   fixed,
   inc_reload
};

enum class source_addr_cntrl {
   increment,
   decrement,
   fixed
};

enum class repeat {
   off,
   on
};

enum class transfer_type {
   bits_16,
   bits_32
};

// This can't be used to access SRAM so we'll never actually use it
// enum class to_game_pak {
//    no = 0,
//    yes = 1
// };

enum class start_timing {
   immediate,
   vblank,
   hblank,
   special
};

enum class irq {
   disable,
   enable
};

enum class enable {
   off,
   on
};

} // namespace gba::dma_opt

namespace detail {
   struct dma_builder {};
} // namespace gba::detail

#define MAKE_SET(type, shift) \
   constexpr self& set(MAKE_SET_NAMESPACE::type val) noexcept { set_field(shift, static_cast<int>(val) & 1); return *this; }

#define MAKE_SET2(type, shift) \
   constexpr self& set(MAKE_SET_NAMESPACE::type val) noexcept \
   { \
      set_field(shift, static_cast<int>(val) & 1); \
      set_field(shift + 1, static_cast<int>(val) >> 1); \
      return *this; \
   }

#define MAKE_SET3(type, shift)\
   constexpr self& set(MAKE_SET_NAMESPACE::type val) noexcept \
   { \
      set_field(shift, static_cast<int>(val) & 1); \
      set_field(shift + 1, (static_cast<int>(val) >> 1) & 1); \
      set_field(shift + 2, (static_cast<int>(val) >> 2) & 1); \
      return *this; \
   }

#define MAKE_SET_NAMESPACE lcd_opt

struct lcd_options {
   using self = lcd_options;

   MAKE_SET3(bg_mode, 0)
   MAKE_SET(frame_select, 4)
   MAKE_SET(oam_access_during_hblank, 5)
   MAKE_SET(obj_char_mapping, 6)
   MAKE_SET(forced_blank, 7)
   MAKE_SET(display_bg0, 8)
   MAKE_SET(display_bg1, 9)
   MAKE_SET(display_bg2, 10)
   MAKE_SET(display_bg3, 11)
   MAKE_SET(display_obj, 12)
   MAKE_SET(display_window_0, 13)
   MAKE_SET(display_window_1, 14)
   MAKE_SET(display_window_obj, 15)

   constexpr void set_field(int shift_amount, int value) noexcept
   {
      const std::uint16_t base_value = value << shift_amount;
      and_mask &= base_value;
      or_mask |= base_value;
   }

   std::uint16_t and_mask{0xFFFF};
   std::uint16_t or_mask{0x0000};
};

#undef MAKE_SET_NAMESPACE

#define MAKE_SET_NAMESPACE dma_opt

struct dma_options {
   using self = dma_options;

   MAKE_SET2(dest_addr_cntrl, 5)
   MAKE_SET2(source_addr_cntrl, 7)
   MAKE_SET(repeat, 9)
   MAKE_SET(transfer_type, 10)
   MAKE_SET2(start_timing, 12)
   MAKE_SET(irq, 14)
   MAKE_SET(enable, 15)

   constexpr void set_field(int shift_amount, int value) noexcept
   {
      const std::uint16_t base_value = value << shift_amount;
      and_mask &= base_value;
      or_mask |= base_value;
   }

   // Two fields since may want to leave options untouched
   // The left-out bit is the to game pak option; which has no use to me
   std::uint16_t and_mask{0b1111'0111'1111'1111};
   std::uint16_t or_mask{0x0000};
};

#undef MAKE_SET_NAMESPACE

#undef MAKE_SET
#undef MAKE_SET2

struct dma {
   constexpr dma(int num, std::uintptr_t base_addr, detail::dma_builder) noexcept
      : num{num}
      , source_addr_raw{ base_addr}
      , dest_addr_raw{ base_addr + 4}
      , word_count_raw{ base_addr + 8}
      , control_raw{ base_addr + 10}
   {}

   void set_options(dma_options opt) const noexcept
   {
      auto to_set = *control();
      to_set &= opt.and_mask;
      to_set |= opt.or_mask;
      // Do some error checking
      if (num == 0) {
         const auto start_timing = (to_set & 0b0011'0000'0000'0000) >> 12;
         GBA_ASSERT(start_timing != 3);
      }
      *control() = to_set;
   }

   void set_source(const void* ptr) const noexcept
   {
      const auto value = reinterpret_cast<std::uint32_t>(ptr);
      if (num == 0) {
         GBA_ASSERT(is_internal_memory(value));
      }
      *source_addr() = value;
   }

   void set_destination(void* ptr) const noexcept
   {
      const auto value = reinterpret_cast<std::uint32_t>(ptr);
      if (num != 3) {
         GBA_ASSERT(is_internal_memory(value));
      }
      *dest_addr() = value;
   }

   void set_word_count(int num_words) const noexcept
   {
      if (num != 3) {
         GBA_ASSERT(num_words <= 0x4000);
      }
      else {
         GBA_ASSERT(num_words <= 0x10000);
      }
      *word_count() = num_words;
   }

private:
   volatile std::uint32_t* source_addr() const noexcept { return reinterpret_cast<std::uint32_t*>(source_addr_raw); }
   volatile std::uint32_t* dest_addr() const noexcept { return reinterpret_cast<std::uint32_t*>(dest_addr_raw); }
   volatile std::uint16_t* word_count() const noexcept { return reinterpret_cast<std::uint16_t*>(word_count_raw); }
   volatile std::uint16_t* control() const noexcept { return reinterpret_cast<std::uint16_t*>(control_raw); }

   int num;
   std::uintptr_t source_addr_raw;
   std::uintptr_t dest_addr_raw;
   std::uintptr_t word_count_raw;
   std::uintptr_t control_raw;
};

constexpr dma dma0{0, 0x40000B0, detail::dma_builder{}};
constexpr dma dma1{1, 0x40000BC, detail::dma_builder{}};
constexpr dma dma2{2, 0x40000C8, detail::dma_builder{}};
constexpr dma dma3{3, 0x40000D4, detail::dma_builder{}};

namespace detail {

struct lcd {
   void set_options(lcd_options opt) const noexcept
   {
      volatile auto* control = reinterpret_cast<std::uint16_t*>(0x4000000);
      auto to_set = *control;
      to_set &= opt.and_mask;
      to_set |= opt.or_mask;
      *control = to_set;
   }
};

} // namespace gba::detail

constexpr detail::lcd lcd;

} // namespace gba

#endif
