from flask import (Flask,request,redirect,render_template,jsonify)
from uuid import uuid4
from flask_cors import (CORS)
from serial import (Serial)
from serial.tools.list_ports_windows import (comports)
from numpy import arange
import cv2 as cv
import time
app=Flask(__name__,static_folder='static',static_url_path='/')
cors=CORS(app)
serial_port: Serial=None
camera: cv.VideoCapture=cv.VideoCapture(0)
data_file: str=""
# region plotter functions
def list_ports() -> str:
  try:
    ports=comports()
    response=''
    for i,p in enumerate(ports):
      response+=f'{p.name},\t' if i+1!=len(ports) else p.name
    return response
  except Exception as e:
    return str(e)
def connect(p: str = 'none',br: int = -1) -> str:
  if p=='none' or br==-1:
    return 'we except something like this: connect -p "COM6" -br 115200'
  p=p.upper()
  global serial_port
  if serial_port: serial_port.close()
  try:
    serial_port=Serial(port=p,baudrate=br,timeout=10)
  except Exception as e:
    return str(e)
  return 'Connected!!'
def open_port() -> str:
  try:
    serial_port.open()
  except Exception as e:
    return str(e)
  return 'Connected!!'
def is_connected() -> str:
  if serial_port is None: return 'We are not connected!!'
  else: return f'port ({serial_port.name}, {serial_port._baudrate}): '\
               f'{"connected" if serial_port.isOpen() else "not connected"}'
def disconnect() -> str:
  if serial_port is None: return 'we are not connected!!'
  else: serial_port.close(); return f'disconnected'
def check_connection(func):
  def inner1(*args,**kwargs):
    if serial_port is None or not serial_port.isOpen(): return 'Please connect first.'
    serial_port.reset_input_buffer()
    try:
      return func(*args,**kwargs)
    except Exception as e:
      return str(e)
  return inner1
@check_connection
def send_message(msg: str,read_result: bool = True,append_new_line: bool = True) -> str:
  if append_new_line: msg=msg if msg[-1]=='\n' else msg+'\n'
  serial_port.write(msg.encode('utf-8'))
  if read_result:
    msg=serial_port.readline()
    msg=msg[:-2].decode('utf-8')
    return msg
  return "Done"
@check_connection
def send_gcode(g: str = 'none') -> str:
  if g=='none': return 'we except something like: send_gcode -g "G00_X15_Y15"'
  return send_message(g)
@check_connection
def mov(x: int,y: int) -> str:
  msg=send_message('G90')
  if msg!='ok': return f"can't do <G90> message: {msg}"
  msg=send_message('G00 Z5')
  if msg!='ok': return f"can't do <G00 Z5> message: {msg}"
  return send_message(f'G00 X{x} Y{y}')
@check_connection
def disable_steppers() -> str: return send_message(f'M18')
@check_connection
def home() -> str: return send_message('G28')
@check_connection
def plot(func: str = 'none',lx: int = -1,ux: int = -1) -> str:
  if func=='none' or (lx==ux): return 'we except something like: plot "x**2" -1 1'
  scale=(ux-lx)/30
  step=scale*0.1
  x_arr=list(map(lambda x:round(x,5),arange(lx,ux+step,step)))
  _locals=locals()
  exec(f"y_arr=list(map(lambda x:round({func},5),x_arr))",globals(),_locals)
  y_arr=_locals['y_arr']
  ly=min(y_arr)
  uy=ly+(ux-lx)
  offset=[-lx/scale,-ly/scale]
  points=list(zip(x_arr,y_arr))
  points=list(filter(lambda x:ux>=x[0]>=lx and uy>=x[1]>=ly,points))
  points=list(map(lambda x:[x[0]/scale,x[1]/scale],points))
  points=list(map(lambda x:[x[0]+offset[0],x[1]+offset[1]],points))
  points=list(map(lambda x:[round(x[0],2),round(x[1],2)],points))
  send_message('G90')
  send_message('G00 Z5')
  send_message('G01 F300')
  send_message('MS8')
  for i,p in enumerate(points):
    send_message(f'G01 X{p[1]} Y{p[0]}')
    if i==0: send_message('G00 Z0')
  send_message('G00 Z5')
  if ly<=0 and uy>=0:
    x_arr=list(map(lambda x:round(x,2),arange(lx,ux+step,step)))
    y_arr=list(map(lambda x:x*0,x_arr))
    points=list(zip(x_arr,y_arr))
    points=list(filter(lambda x:ux>=x[0]>=lx and uy>=x[1]>=ly,points))
    points=list(map(lambda x:[x[0]/scale,x[1]/scale],points))
    points=list(map(lambda x:[x[0]+offset[0],x[1]+offset[1]],points))
    points=list(map(lambda x:[round(x[0],2),round(x[1],2)],points))
    points=[points[0],points[-1]]
    for i,p in enumerate(points):
      send_message(f'G01 X{p[1]} Y{p[0]}')
      if i==0: send_message('G00 Z0')
  send_message('G00 Z5')
  if lx<=0 and ux>=0:
    y_arr=list(map(lambda x:round(x,2),arange(ly,uy+step,step)))
    x_arr=list(map(lambda x:x*0,y_arr))
    points=list(zip(x_arr,y_arr))
    points=list(filter(lambda x:ux>=x[0]>=lx and uy>=x[1]>=ly,points))
    points=list(map(lambda x:[x[0]/scale,x[1]/scale],points))
    points=list(map(lambda x:[x[0]+offset[0],x[1]+offset[1]],points))
    points=list(map(lambda x:[round(x[0],2),round(x[1],2)],points))
    points=[points[0],points[-1]]
    for i,p in enumerate(points):
      send_message(f'G01 X{p[1]} Y{p[0]}')
      if i==0: send_message('G00 Z0')
  send_message('G00 Z5')
  send_message('G00 X0 Y0')
  send_message('G28')
  return "The graph is ready :)"
@check_connection
def gtb() -> str: return send_message("gtb")
@check_connection
def gub() -> str: return send_message("gub")
@check_connection
def ls(_dir: str = '') -> str: return send_message(f"ls {_dir}")
@check_connection
def cd(_dir: str = '..') -> str: return send_message(f"cd {_dir}")
@check_connection
def pwd() -> str: return send_message("pwd")
@check_connection
def mkdir(_dir: str = '') -> str: return send_message(f"mkdir {_dir}")
@check_connection
def rm(_dir: str = '') -> str: return send_message(f"rm {_dir}")
@check_connection
def touch(_dir: str = '') -> str: return send_message(f"touch {_dir}")
@check_connection
def write(_dir: str = '') -> str:
  send_message(f"write {_dir}",read_result=False)
  print(data_file)
  send_message(f"{data_file}")
  return
@check_connection
def read(_dir: str = '') -> str:
  send_message(f"rf {_dir}",read_result=False)
  time.sleep(3)
  data=''.join(list(map(lambda x:x.decode('utf-8'),serial_port.readlines())))
  return data
# endregion
# region imaging
def get_img(q: int = 100,rf: float = 1) -> str:
  _,frame=camera.read()
  dist=f'imgs/{uuid4()}.jpg'
  frame=cv.resize(frame,(int(frame.shape[0]*rf),int(frame.shape[1]*rf)),interpolation=cv.INTER_AREA)
  frame=cv.rotate(frame,cv.ROTATE_90_CLOCKWISE)
  cv.imwrite(f'static/{dist}',frame,[cv.IMWRITE_JPEG_QUALITY,q])
  return dist
# endregion
@app.errorhandler(404)
def e404(_): return redirect('http://www.milad-karami.ir/404')
@app.route('/')
def index(): return render_template('index.html')
@app.route('/404')
def p404(): return render_template('404.html')
@app.route('/do_command',methods=["POST"])
def do_command():
  global data_file
  command=str(request.get_json()['command']).lower()
  data_file=request.get_json()['file']
  params=command.split(' ')
  command=params[0]
  params.pop(0)
  typeof='span'
  try:
    _locals=locals()
    args=''
    for p in params:
      if '~' in p: args+=f'{p[1:]}='
      else:
        if p.isalpha(): p=f'"{p}"'
        args+=f'{p.replace("_"," ")},'
    if args: args=args[:-1]
    exec(f"response_message={command}({args})",globals(),_locals)
    response_message=_locals['response_message']
    typeof='img' if command=='get_img' else typeof
  except: response_message=f'Error: <{command}> is not recognized.'
  return jsonify({'response_message':response_message,'type':typeof})
if __name__=="__main__":
  app.run()
