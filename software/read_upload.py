from os.path import (isfile)
from funcs import (send_message)
def read_upload():
  while True:
    file_address=input('(File Address or exit)>>> ')
    if file_address=='exit':
      break
    if isfile(file_address):
      f=open(file_address,'r')
      g_codes=f.readlines()
      for gc in g_codes:
        send_message(gc.replace('\n',''))
    else:
      print('The File Address Is Not Valid.')
