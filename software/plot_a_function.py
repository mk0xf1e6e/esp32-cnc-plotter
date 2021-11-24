from numpy import (arange)
from funcs import (send_message)
def transform(point,scale,offset):
  point=[point[0]/scale,point[1]/scale]
  point=[point[0]+offset[0],point[1]+offset[1]]
  point=[round(point[0],2),round(point[1],2)]
  return point
def plot(points,scale,offset):
  send_message('G90')
  send_message('MS8')
  send_message('G01 F300')
  send_message('G00 Z5')
  for i,p in enumerate(points):
    p=transform(p,scale=scale,offset=offset)
    send_message(f'G01 X{p[1]} Y{p[0]}')
    if i==0: send_message('G00 Z0')
  send_message('G00 Z5')
  send_message('MS1')
  send_message('G00 X0 Y0')
  send_message('G28')
def plot_a_function():
  while True:
    user_input=input('(Enter a function <2*x> or exit)>>> ')
    if user_input=='exit': break
    func=user_input
    user_input=input('(Enter the range or exit)>>> ')
    if user_input=='exit': break
    try:
      lx,ux=map(float,user_input.split('_' if '_' in user_input else ','))
      scale=(ux-lx)/30
      step=scale*0.1
      x_arr=list(map(lambda i:round(i,2),arange(lx,ux+step,step)))
      _locals=locals()
      exec(f"y_arr=list(map(lambda x:round({func},2),x_arr))",globals(),_locals)
      y_arr=_locals['y_arr']
      ly=min(y_arr)
      uy=ly+(ux-lx)
      offset=[-lx/scale,-ly/scale]
      plot(list(zip(x_arr,y_arr)),scale,offset)
    except ValueError:
      continue
    user_input=input('(Do Want To Draw Coordinates?(y/or any thing else) or exit)>>> ')
    if user_input=='exit': break
    if user_input.lower()=='y':
      if ly<=0 and uy>=0:
        x_arr=list(map(lambda x:round(x,2),arange(lx,ux+step,step)))
        y_arr=list(map(lambda x:x*0,x_arr))
        plot(list(zip(x_arr,y_arr)),scale,offset)
      if lx<=0 and ux>=0:
        y_arr=list(map(lambda x:round(x,2),arange(ly,uy+step,step)))
        x_arr=list(map(lambda x:x*0,y_arr))
        plot(list(zip(x_arr,y_arr)),scale,offset)
