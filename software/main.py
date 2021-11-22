from funcs import (close_port,clear)
from send_messages import (send_messages)
from the_controller import (setup_the_controller,wait_for_the_controller,set_enable_the_controller)
current_page='main'
page_options={
  'main':[
    'send messages',
    'the controller',
    'read and send g-code file',
    'upload file to sd card'
  ],
  'main > send_messages':send_messages,
  'main > the_controller':wait_for_the_controller,
}
skip_get_option=False
def exit_program():
  clear()
  close_port()
  exit(0)
def print_error(message: str):
  clear()
  print(message)
  input()
def get_option() -> int:
  options=list(page_options[f'{current_page}'])
  user_input=input('>>> ')
  if user_input.isdigit():
    user_input=int(user_input)
    if user_input==len(options)+1: exit_program()
    if 0<user_input<=len(options):
      return user_input
    print_error(f'Please enter a number between 1 and {len(options)} or type the option.')
  else:
    user_input.lower()
    if user_input=='exit': exit_program()
    if user_input in options:
      return options.index(user_input)+1
    print_error(f'<{user_input}> is not an option.')
  return -1
def render_current_page():
  global current_page,skip_get_option
  clear()
  print(f'{current_page}\n')
  page=page_options[f'{current_page}']
  if type(page)==list:
    for i,option in enumerate(page):
      print(f'{i+1}. {str(option).title()}')
    print(f'{len(page)+1}. Exit')
  else:
    page()
    current_page='main'
    skip_get_option=True
def process_option(option: int):
  global current_page
  if option==-1: return
  if current_page=='main':
    if option==1:
      current_page='main > send_messages'
    elif option==2:
      set_enable_the_controller(True)
      current_page='main > the_controller'
def main():
  global skip_get_option
  while True:
    render_current_page()
    if not skip_get_option:
      process_option(get_option())
    else:
      skip_get_option=False
if __name__=='__main__':
  setup_the_controller()
  main()
