import cv2

title = cv2.imread('../resources/title.png')

colors = {}

# Apparently only 15-31 are really visible
# So use 14 as 0 and go from there
# Mess around with this eventually for better colors
def conv(val):
   low_val = 0
   return int(low_val + (31 - low_val) * round(val / 255))

def make_color(r, g, b):
   # Either the documentation is wrong (doubtful), or there's some weird endian issue
   # This seems to work, though
   # Swapping the bytes around doesn't seem to fix it so ehhhh
   return (conv(r) << 10) + (conv(g) << 5) + (conv(b) << 0)

values = []

for row in title:
   for r, g, b in row:
      values.append(make_color(r, g, b))

snake = cv2.imread('../resources/dudes/snake.png')

snake_vals = []
snake_pal = []
pal_map = {}

for base_y in range(len(snake) // 8):
   for base_x in range(len(snake[0]) // 8):
      for y in range(8):
         add_snake = False
         snake_temp = 0
         for x in range(8):
            r, g, b = snake[base_y * 8 + y][base_x * 8 + x]
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

print(f'#include<cstdint>\nconstexpr std::uint16_t title_screen[]{str(values).replace("[", "{").replace("]", "};")}')
print(f'constexpr std::uint8_t snake[]{str(snake_vals).replace("[", "{").replace("]", "};")}')
print(f'constexpr std::uint16_t snake_pal[]{str(snake_pal).replace("[", "{").replace("]", "};")}')

# for row in title:
#    for r, g, b in row:
#       colors[(r, g, b)] = 1

# print(colors)
