import keyboard
from serial import Serial
port=Serial('COM6',115200,timeout=30)
def clear():
  keyboard.press('ctrl+l')
  keyboard.release('ctrl+l')
def send_message(message: str,print_result: bool = True) -> str:
  msg=bytes(f'{message}\n','utf-8')
  if print_result: print(message,end=' ... ',flush=True)
  port.write(msg)
  response=port.readline().decode('utf-8').replace('\r\n','')
  if print_result: print(response)
  return response
def close_port():
  port.close()
