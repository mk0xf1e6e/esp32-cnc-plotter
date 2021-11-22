import keyboard
from funcs import (send_message)
the_controller_is_en=False
x_step_size=0.1
y_step_size=0.1
z_step_size=0.1
def set_enable_the_controller(en: bool):
  global the_controller_is_en
  send_message('g91' if en else 'g90',print_result=False)
  if not en: send_message('m18')
  the_controller_is_en=en
def backspace():
  keyboard.press('backspace')
  keyboard.release('backspace')
def on_up_pressed(_):
  if not the_controller_is_en or not keyboard.is_pressed('up'): return
  backspace()
  send_message(f'g00 x{x_step_size}')
def on_down_pressed(_):
  if not the_controller_is_en or not keyboard.is_pressed('down'): return
  backspace()
  send_message(f'g00 x-{x_step_size}')
def on_left_pressed(_):
  if not the_controller_is_en or not keyboard.is_pressed('left'): return
  backspace()
  send_message(f'g00 y{y_step_size}')
def on_right_pressed(_):
  if not the_controller_is_en or not keyboard.is_pressed('right'): return
  backspace()
  send_message(f'g00 y-{y_step_size}')
def on_plus_pressed(_):
  if not the_controller_is_en or not keyboard.is_pressed('+'): return
  backspace()
  send_message(f'g00 z{z_step_size}')
def on_minus_pressed(_):
  if not the_controller_is_en or not keyboard.is_pressed('-'): return
  backspace()
  send_message(f'g00 z-{z_step_size}')
def on_e_pressed(_):
  if not the_controller_is_en or not keyboard.is_pressed('e'): return
  global x_step_size
  backspace()
  x_step_size*=10 if (x_step_size<10) else 1/100
  print(f'x step size is {x_step_size}')
def on_d_pressed(_):
  if not the_controller_is_en or not keyboard.is_pressed('d'): return
  global y_step_size
  backspace()
  y_step_size*=10 if (y_step_size<10) else 1/100
  print(f'y step size is {y_step_size}')
def on_r_pressed(_):
  if not the_controller_is_en or not keyboard.is_pressed('r'): return
  global z_step_size
  backspace()
  z_step_size*=10 if (z_step_size<10) else 1/100
  print(f'z step size is {z_step_size}')
def on_q_pressed(_):
  if not the_controller_is_en or not keyboard.is_pressed('q'): return
  backspace()
  set_enable_the_controller(False)
def clear_keyboard(_):
  if not the_controller_is_en: return
  backspace()
def setup_the_controller():
  keyboard.on_press_key('up',on_up_pressed)
  keyboard.on_press_key('down',on_down_pressed)
  keyboard.on_press_key('left',on_left_pressed)
  keyboard.on_press_key('right',on_right_pressed)
  keyboard.on_press_key('+',on_plus_pressed)
  keyboard.on_press_key('-',on_minus_pressed)
  keyboard.on_press_key('e',on_e_pressed)
  keyboard.on_press_key('d',on_d_pressed)
  keyboard.on_press_key('r',on_r_pressed)
  keyboard.on_press_key('q',on_q_pressed)
  keyboard.on_release(clear_keyboard)
def wait_for_the_controller():
  while the_controller_is_en: pass
