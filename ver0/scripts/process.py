import cv2

title = cv2.imread('../resources/title.png')
won = cv2.imread('../resources/win.png')

colors = {}

# Apparently only 15-31 are really visible
# So use 14 as 0 and go from there
# Mess around with this eventually for better colors
def conv(val):
   low_val = 0
   return int(low_val + round((31 - low_val) * val / 255))

def make_color(r, g, b):
   # Either the documentation is wrong (doubtful), or there's some weird endian issue
   # This seems to work, though
   # Swapping the bytes around doesn't seem to fix it so ehhhh
   return (conv(r) << 10) + (conv(g) << 5) + (conv(b) << 0)

values = []
for row in title:
   for r, g, b in row:
      values.append(make_color(r, g, b))

values2 = []
for row in won:
   for r, g, b in row:
      values2.append(make_color(r, g, b))

def process_image(image, all_visible):
   snake_vals = []
   snake_pal = []
   if all_visible:
      snake_pal.append(0)
   pal_map = {}

   assert len(image) % 8 == 0
   for base_y in range(len(image) // 8):
      assert len(image[0]) % 8 == 0
      for base_x in range(len(image[0]) // 8):
         for y in range(8):
            add_snake = False
            snake_temp = 0
            for x in range(8):
               r, g, b = image[base_y * 8 + y][base_x * 8 + x]
               if (r, g, b) not in pal_map:
                  pal_num = len(snake_pal)
                  pal_map[(r, g, b)] = pal_num
                  snake_pal.append(make_color(r, g, b))
               if add_snake:
                  snake_vals.append((pal_map[(r, g, b)] << 4) + snake_temp)
                  add_snake = False
               else:
                  add_snake = True
                  snake_temp = pal_map[(r, g, b)]

   return snake_vals, snake_pal

def to_cpp(a_list):
   return str(a_list).replace("[", "{").replace("]", "};")

def output_image(name, file_loc, all_visible):
   vals, pal = process_image(cv2.imread(f'../resources/{file_loc}.png'), all_visible)
   print(f'constexpr std::uint8_t {name}[]{to_cpp(vals)}')
   print(f'constexpr std::uint16_t {name}_pal[]{to_cpp(pal)}')

print(f'#include<cstdint>\nconstexpr std::uint16_t title_screen[]{to_cpp(values)}')
print(f'#include<cstdint>\nconstexpr std::uint16_t won_screen[]{to_cpp(values2)}')
output_image('snake', 'dudes/snake', False)
output_image('angry_snake', 'dudes/snake_angry', False)
output_image('something', 'dudes/something', False)
output_image('art_itself', 'dudes/art_itself', False)
output_image('chars', 'font/chars', True)
output_image('numbers', 'font/numbers', True)
output_image('special_font', 'font/special', True)
output_image('tiles', 'tiles', True)
output_image('you', 'you', False)

# for row in title:
#    for r, g, b in row:
#       colors[(r, g, b)] = 1

# print(colors)
