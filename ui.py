from tkinter import *
from tkinter import ttk
import serial
from serial.tools.list_ports import comports
from functools import partial

def add_coms(lb):
    lports = comports()
    list_ports=[]
    for nport,port1 in enumerate(lports):
        print( lb['values'] )
        #lb.insert(nport+1,port1.device) # For a listbox
        list_ports += [port1.device]
    lb['values']=list_ports
    
def check_serial():
    global ser
    #if ser.in_waiting>0:
        #c=ser.read();
        #print (c,end='',flush=True)
    #else:
        #print ('.',end='',flush=True)

    lin=ser.readline()
    if len(lin)>0:
        print(lin,end='',flush=True)

        if b'ready' in lin:
            l_status.configure(text='READY', foreground='green' )
        elif b'pos1' in lin:
            l_p1.configure(text=lin[0:])
        elif b'pos2' in lin:
            l_p2.configure(text=lin[0:])
        elif b'pos3' in lin:
            l_p3.configure(text=lin[0:])
        elif b'pos4' in lin:
            l_p4.configure(text=lin[0:])

    root.after(10, lambda: check_serial())

def getpos():
    ser.write(b'p');
def mov0():
    ser.write(b'0');
def mov1():
    ser.write(b'1');
def mov2():
    ser.write(b'2');

def cal1():
    ser.write(b'A')
    b_cal2.configure(state="enable")
    b_cal1.configure(state="disable")
def cal2():
    ser.write(b'C')
    b_cal2.configure(state="enable")
    b_cal1.configure(state="enable")

def start(portname):
    global ser
    #port=list_ports.curselection();
    #portname=list_ports.get(port)
    #portname=

    ser=serial.Serial(portname.get(),baudrate=115200,timeout=0)
    root.after(10, lambda: check_serial())

#def start():
    #b.configure(text='Stop', command=stop)
    #l['text'] = 'Working...'
    #global interrupt; interrupt = False
    #root.after(1, step)
    
#def stop():
    #global interrupt; interrupt = True
    
#def step(count=0):
    #p['value'] = count
    #if interrupt:
        #result(None)
        #return
    #root.after(100)  # next step in our operation; don't take too long!
    #if count == 20:  # done!
        #result(42)
        #return
    #root.after(1, lambda: step(count+1))
    #
#def result(answer):
    #p['value'] = 0
    #b.configure(text='Start!', command=start)
    #l['text'] = "Answer: " + str(answer) if answer else "No Answer"

going=False
def doit(arg):
    global going
    if going:
        print(arg,end=' ',flush=True)
        root.after(20, partial(doit,arg+1))
def fine_start(arg,evnt):
    global going
    #print( arg, " !!!  ", what )
    going=True
    root.after(20, partial(doit,arg+1))
def fine_stop(arg,evnt):
    global going
    going=False
    
def coarse_start(arg,evnt):
    global going
    #print( arg, " !!!  ", what )
    going=True
    root.after(20, partial(doit,arg+1))
def coarse_stop(arg,evnt):
    global going
    going=False
    
root = Tk()
root.title('Scanning WFS - Simple Controller UI')
root.geometry('768x512')
f = ttk.Frame(root, width=512); f.grid()

str_port=StringVar();
list_ports = ttk.Combobox(f,textvariable=str_port); add_coms( list_ports); list_ports.grid(row=1, column=5,padx=5,pady=5)
#l_port = ttk.Label(f, text="Port"); l_port.grid(row=1, column=5, padx=5, pady=5)
list_ports.bind("<<ComboboxSelected>>", lambda x: b_connect.configure(state="enable"))
l_status = ttk.Label(f, text="NOT Connected", foreground='red'); l_status.grid(row=0, column=5, padx=5, pady=5)
b_connect = ttk.Button(f, text="Connect!", command=partial(start,str_port),state='disable'); b_connect.grid(row=2,column=5, padx=5, pady=5)

b_pos = ttk.Button(f, text="Get pos", command=getpos); b_pos.grid(row=1, column=2, padx=5, pady=5)
l_p1 = ttk.Label(f, text="Pos1:"); l_p1.grid(row=2, column=2, padx=5, pady=5)
l_p2 = ttk.Label(f, text="Pos2:"); l_p2.grid(row=3, column=2, padx=5, pady=5)
l_p3 = ttk.Label(f, text="Pos3:"); l_p3.grid(row=4, column=2, padx=5, pady=5)
l_p4 = ttk.Label(f, text="Pos4:"); l_p4.grid(row=5, column=2, padx=5, pady=5)

b_finesR=[ttk.Button(f, text='%d>'%n) for n in range(4)]
b_coarsesR=[ttk.Button(f, text='%d>>'%n) for n in range(4)]
b_finesL=[ttk.Button(f, text='<%d'%n) for n in range(4)]
b_coarsesL=[ttk.Button(f, text='<<%d'%n) for n in range(4)]
for nbutton,b1 in enumerate(b_finesR):
    b1.grid(row=nbutton+2,column=3,padx=5,pady=5)
    b1.bind('<ButtonPress-1>',partial(fine_start,1))
    b1.bind('<ButtonRelease-1>',partial(fine_stop,1))

for nbutton,b1 in enumerate(b_coarsesR):
    b1.grid(row=nbutton+2,column=4,padx=5,pady=5)
    b1.bind('<ButtonPress-1>',partial(coarse_start,1))
    b1.bind('<ButtonRelease-1>',partial(coarse_stop,1))

for nbutton,b1 in enumerate(b_finesL):
    b1.grid(row=nbutton+2,column=1,padx=5,pady=5)
    b1.bind('<ButtonPress-1>',partial(fine_start,1))
    b1.bind('<ButtonRelease-1>',partial(fine_stop,1))

for nbutton,b1 in enumerate(b_coarsesL):
    b1.grid(row=nbutton+2,column=0,padx=5,pady=5)
    b1.bind('<ButtonPress-1>',partial(coarse_start,1))
    b1.bind('<ButtonRelease-1>',partial(coarse_stop,1))

b_cal1 = ttk.Button(f, text="Cal1", command=cal1); b_cal1.grid(row=0, column=0, padx=5, pady=5)
b_cal2 = ttk.Button(f, text="Cal2", command=cal2,state='disable'); b_cal2.grid(row=0, column=1, padx=5, pady=5)

# Row/column sizes: w is parent widget
#w.columnconfigure(N, option=value, ...)
#w.rowconfigure(N, option=value, ...)
# minsize 	The column or row's minimum size in pixels. If there is nothing in the given column or row, it will not appear, even if you use this option.
# pad 	A number of pixels that will be added to the given column or row, over and above the largest cell in the column or row.
# weight : stretchable, weighted

#b_do0 = ttk.Button(f, text="Move 0", command=mov0); b_do0.grid(row=3, column=2, padx=5, pady=5)
#b_do1 = ttk.Button(f, text="Move 1", command=mov1); b_do1.grid(row=4, column=2, padx=5, pady=5)
#b_do2 = ttk.Button(f, text="Move 2", command=mov2); b_do2.grid(row=5, column=2, padx=5, pady=5)

#b_test = ttk.Button(f, text="Test"); b_test.grid(row=5, column=3, padx=5, pady=5)
#b_test.bind('<ButtonPress-1>',partial(fstart,1))
#b_test.bind('<ButtonRelease-1>',partial(fstop,1))
#p = ttk.Progressbar(f, orient="horizontal", mode="determinate", maximum=20); 
#p.grid(column=2, row=2, padx=5, pady=5)
root.mainloop()

ser.close()
