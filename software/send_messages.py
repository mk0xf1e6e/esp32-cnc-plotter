from funcs import (send_message)
def send_messages():
  while True:
    user_input=input('(type exit for go back to main menu)>>> ')
    if user_input=='exit': break
    send_message(user_input)
