import pygame
import sys
import os
import random
import copy

pygame.init()
SCREEN_WIDTH = 240
SCREEN_HEIGHT = 160

os.chdir('../resources')
screen = pygame.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT), pygame.SCALED)
font = pygame.image.load('font/chars.png')
numbers = pygame.image.load('font/numbers.png')
special_font = pygame.image.load('font/special.png')
tiles = pygame.image.load('tiles.png')
special_chars = ['/', '.', '>', '#', '$', '-', "'", ',']
images = {}

class move:
   def __init__(self, name, power, ammo):
      self.name = name
      self.power = power
      self.ammo = ammo
      self.max_ammo = ammo

class dude:
   def __init__(self, name, file_name, hp, level, attack, defense, m_attack, m_defense, speed, moves):
      global images
      images[name] = pygame.image.load(f'dudes/{file_name}.png')
      self.name = name
      self.hp = hp
      self.level = level
      self.max_hp = hp
      self.attack = attack
      self.defense = defense
      self.m_attack = m_attack
      self.m_defense = m_defense
      self.speed = speed
      self.moves = moves

def calc_damage(attacker, defender, attack):
   return int((attacker.attack / defender.defense) * (attack.power) * random.uniform(.75, 1.25))

moves = {
   'hiss': move('hiss', 10, 90),
   'hiss2': move('hiss2', 15, 80),
   'hiss3': move('hiss3', 255, 1),
   'extreme hiss': move('extreme hiss', 200, 5)
}

dudes = {
   'snake': dude('snake', 'snake', 9999, 200, 1234, 1234, 1234, 1234, 1234, [moves['hiss'], moves['hiss2'], moves['hiss3']]),
   'angry snake': dude('angry snake', 'snake_angry', 567, 2, 1234, 10, 10, 10, 9999, [moves['extreme hiss']])
}

def disp_text(x, y, text):
   x *= 8
   y *= 8
   text = text.lower()
   loc = x
   for c in text:
      if 'a' <= c <= 'z':
         offset = (ord(c) - ord('a')) * 8
         screen.blit(font, (x, y), ((offset, 0), (8, 8)))
      elif '0' <= c <= '9':
         offset = (ord(c) - ord('0')) * 8
         screen.blit(numbers, (x, y), ((offset, 0), (8, 8)))
      elif c in special_chars:
         offset = special_chars.index(c) * 8
         screen.blit(special_font, (x, y), ((offset, 0), (8, 8)))
      x += 8

def title():
   title = pygame.image.load('title.png')
   while True:
      for event in pygame.event.get():
         if event.type == pygame.QUIT:
            sys.exit()
         elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_z:
               return
      screen.fill((0, 0, 0))
      screen.blit(title, title.get_rect())
      disp_text(15, 2, '>New')
      disp_text(16, 3, 'Continue')
      disp_text(16, 4, 'Quick Load')
      pygame.display.flip()

def battle(you, enemy):
   arrow_loc = 0
   log = []
   turn = 1
   fight_over = False

   def disp_stats(d, x):
      # Name
      if len(d.name.split(' ')) == 1:
         disp_text(x, 0, d.name)
      else:
         line1, line2 = d.name.split(' ')
         disp_text(x, 0, line1)
         disp_text(x + 1, 1, line2)
      # Level
      disp_text(x + 4, 2, f'${d.level:>3}')
      # HP
      disp_text(x, 11, f'{d.hp:>5}')
      disp_text(x, 12, f'/{d.max_hp:>5}')

   def do_attack():
      nonlocal turn
      nonlocal log
      nonlocal fight_over
      try:
         if you.moves[arrow_loc].ammo == 0:
            raise Exception
         if fight_over:
            raise Exception
         dudes = reversed(sorted([(you, enemy, you.moves[arrow_loc]), (enemy, you, random.choice(enemy.moves))], key=lambda x: x[0].speed))
         log = []
         log.append(f'Turn {turn:>5}')
         for attacker, defender, atk in dudes:
            if attacker.hp == 0:
               continue

            damage = calc_damage(attacker, defender, atk)
            def add_name(name):
               if len(name.split(' ')) == 1:
                  log.append(name)
               else:
                  line1, line2 = name.split(' ')
                  log.append(line1)
                  log.append(f' {line2}')

            add_name(f'-{attacker.name}')
            add_name(f'-{atk.name}')
            log.append(f'-{damage:>5}')
            log.append(' damage')
            log.append('')
            atk.ammo -= 1
            defender.hp -= damage
            if defender.hp < 0:
               defender.hp = 0

         if enemy.hp == 0:
            log.append('')
            log.append('you winned')
            log.append('xxx point')
            fight_over = True
         elif you.hp == 0:
            log.append('')
            log.append('you losed')
            fight_over = True

         turn += 1
      except Exception as e:
         print(e)

   while True:
      for event in pygame.event.get():
         if event.type == pygame.QUIT:
            sys.exit()
         elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_DOWN:
               arrow_loc += 1
            elif event.key == pygame.K_UP:
               arrow_loc -= 1
            elif event.key == pygame.K_z:
               if not fight_over:
                  do_attack()
               else:
                  return
      screen.fill((255, 255, 255))
      # Borders
      black = (0, 0, 0)
      pygame.draw.rect(screen, black, ((0, 104 + 2), (160 - 2, 4)))
      pygame.draw.rect(screen, black, ((152 + 2, 0), (4, 1000)))
      pygame.draw.rect(screen, black, ((104 + 2 , 104 + 2), (4, 1000)))
      # Other stuffs
      if you.hp > 0:
         disp_stats(you, 0)
         screen.blit(images[you.name], (0, 24))
      if enemy.hp > 0:
         disp_stats(enemy, 11)
         screen.blit(images[enemy.name], (88, 24))
      if not fight_over:
         # Moves
         for i, m in enumerate(you.moves):
            disp_text(1, 14 + i, m.name)
         disp_text(0, 14 + arrow_loc, '>')
         # Move info
         try:
            m = you.moves[arrow_loc]
            disp_text(14, 14, "Pow")
            disp_text(16, 15, f'{m.power:>3}')
            disp_text(14, 16, "Mult")
            disp_text(15, 17, "1.0#")
            disp_text(14, 18, "Ammo")
            disp_text(14, 19, f'{m.ammo:>2}/{m.max_ammo:>2}')
         except:
            pass
      else:
         if you.hp > 0:
            disp_text(0, 14, '>End fight')
         else:
            disp_text(0, 14, ">You'm died")
      # Battle log
      for i, line in enumerate(log):
         disp_text(20, i, line)
      pygame.display.flip()


def explore():
   the_map = [
      [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1], #  1
      [1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1], #  2
      [1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1], #  3
      [1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1], #  4
      [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1], #  5
      [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1], #  6
      [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1], #  7
      [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1], #  8
      [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1], #  9
      [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1], # 10
   ]
   you_dude = copy.deepcopy(dudes['snake'])
   you = pygame.image.load('you.png')
   x = 80
   y = 80
   snake_x = 96
   snake_y = 96
   small_snake = pygame.transform.scale(images['angry snake'], (16, 16))
   while True:
      for event in pygame.event.get():
         if event.type == pygame.QUIT:
            sys.exit()
      keys = pygame.key.get_pressed()
      new_x = x
      new_y = y
      if keys[pygame.K_DOWN]:
         new_y += 16
      elif keys[pygame.K_UP]:
         new_y -= 16
      if keys[pygame.K_RIGHT]:
         new_x += 16
      elif keys[pygame.K_LEFT]:
         new_x -= 16
      check_y = int(new_y / 16)
      check_x = int(new_x / 16)
      if the_map[check_y][check_x] == 0:
         x = new_x
         y = new_y
      screen.fill((255, 255, 255))
      for (tile_y, row) in enumerate(the_map):
         for (tile_x, tile) in enumerate(row):
            screen.blit(tiles, (tile_x * 16, tile_y * 16), ((tile * 16, 0), (16, 16)))
      screen.blit(you, (x, y))
      screen.blit(small_snake, (snake_x, snake_y))
      if x == snake_x and y == snake_y:
         battle(you_dude, copy.deepcopy(dudes['angry snake']))
         if you_dude.hp == 0:
            return
         while (x == snake_x and y == snake_y) or (the_map[snake_y // 16][snake_x // 16] == 1):
            snake_x = random.randint(0, SCREEN_WIDTH // 16 - 1) * 16
            snake_y = random.randint(0, SCREEN_HEIGHT // 16 - 1) * 16
      pygame.display.flip()
      pygame.time.wait(20)


def main():
   while True:
      title()
      explore()

if __name__ == '__main__':
   main()
