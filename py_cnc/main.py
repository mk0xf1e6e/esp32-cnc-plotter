import serial
from time import sleep
def send():
  with serial.Serial("COM6",115200,timeout=50) as port:
    f=open("test.gcode","r")
    lines=f.readlines()
    for l in lines:
      l=l.replace('\n','')
      port.write(bytes(f'{l}\n','utf-8'))
      while port.readline().decode("utf-8").replace('\r\n','') != 'ok':
        sleep(0.1)
      print(f'{l} ... ok')
    f.close()
def main():
  send()
if __name__=='__main__':
  main()
