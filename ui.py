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
    ser.write(b'Z');
def mov1():
    ser.write(b'S');
def mov2():
    ser.write(b'F');

def movH0():
    ser.write(b'z');
def movH1():
    ser.write(b's');
def movH2():
    ser.write(b'f');
    
def cal1():
    ser.write(b'A')
    b_cal2.configure(state="enable")
    #b_cal1.configure(state="disable")
def cal2():
    ser.write(b'C')
    b_cal2.configure(state="enable")
    b_cal1.configure(state="enable")
def zero_pos():
    ser.write(b'0')
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

def ser_command(arg,evnt):
    #print(arg)
    ser.write(arg)

def movex(arg,evnt):
    s=('%s%c'%(E_amt.get(),chr(ord('A')+arg) )).encode()
    ser.write(s)
    #print(s)

root = Tk()
root.title('Scanning WFS - Simple Controller UI')
root.geometry('900x512')
f = ttk.Frame(root, width=512); f.grid()

str_port=StringVar();
list_ports = ttk.Combobox(f,textvariable=str_port); add_coms( list_ports); list_ports.grid(row=1, column=6,padx=5,pady=5)
#l_port = ttk.Label(f, text="Port"); l_port.grid(row=1, column=5, padx=5, pady=5)
list_ports.bind("<<ComboboxSelected>>", lambda x: b_connect.configure(state="enable"))
l_status = ttk.Label(f, text="NOT Connected", foreground='red'); l_status.grid(row=0, column=6, padx=5, pady=5)
b_connect = ttk.Button(f, text="Connect!", command=partial(start,str_port),state='disable'); b_connect.grid(row=1,column=7, padx=5, pady=5)

b_pos = ttk.Button(f, text="Get pos", command=getpos); b_pos.grid(row=1, column=2, padx=5, pady=5)
l_p1 = ttk.Label(f, text="Pos1:"); l_p1.grid(row=2, column=2, padx=5, pady=5)
l_p2 = ttk.Label(f, text="Pos2:"); l_p2.grid(row=3, column=2, padx=5, pady=5)
l_p3 = ttk.Label(f, text="Pos3:"); l_p3.grid(row=4, column=2, padx=5, pady=5)
l_p4 = ttk.Label(f, text="Pos4:"); l_p4.grid(row=5, column=2, padx=5, pady=5)

b_finesR=[ttk.Button(f, text='%d+'%(n+1)) for n in range(4)]
b_coarsesR=[ttk.Button(f, text='%d++'%(n+1)) for n in range(4)]
b_finesL=[ttk.Button(f, text='-%d'%(n+1)) for n in range(4)]
b_coarsesL=[ttk.Button(f, text='--%d'%(n+1)) for n in range(4)]

b_cals=[ttk.Button(f, text='CAL%d'%(n+1)) for n in range(4)]

E_amt = Entry(f,textvariable=-100)
E_amt.grid(row=6,column=6,padx=5,pady=5)


b_moves=[ttk.Button(f, text='Move %d'%(n+1)) for n in range(4)]

# Each one goes: --,-,+,++ . They are in the top letter row of an asdf keyboard. Caps for big.
codes=[[b'Q',b'q',b'w',b'W'], [b'E',b'e',b'r',b'R'],[b'T',b't',b'y',b'Y'], [b'U',b'u',b'i',b'I'], [b'1',b'2',b'3',b'4'] ]

for nbutton,b1 in enumerate(b_coarsesL):
    b1.grid(row=nbutton+2,column=0,padx=5,pady=5)
    b1.bind('<ButtonPress-1>',partial(ser_command,codes[nbutton][0]))
    b1.bind('<ButtonRelease-1>',partial(ser_command,b'x'))
 
for nbutton,b1 in enumerate(b_finesL):
    b1.grid(row=nbutton+2,column=1,padx=5,pady=5)
    b1.bind('<ButtonPress-1>',partial(ser_command,codes[nbutton][1]))
    b1.bind('<ButtonRelease-1>',partial(ser_command,b'x'))
    
for nbutton,b1 in enumerate(b_finesR):
    b1.grid(row=nbutton+2,column=3,padx=5,pady=5)
    b1.bind('<ButtonPress-1>',partial(ser_command,codes[nbutton][2] )  )
    b1.bind('<ButtonRelease-1>',partial(ser_command,b'x') )

for nbutton,b1 in enumerate(b_coarsesR):
    b1.grid(row=nbutton+2,column=4,padx=5,pady=5)
    b1.bind('<ButtonPress-1>',partial(ser_command,codes[nbutton][3]))
    b1.bind('<ButtonRelease-1>',partial(ser_command,b'x'))

for nbutton,b1 in enumerate(b_cals):
    b1.grid(row=nbutton+2,column=5,padx=5,pady=5)
    b1.bind('<ButtonPress-1>',partial(ser_command,codes[4][nbutton]))
 
for nbutton,b1 in enumerate(b_moves):
    b1.grid(row=nbutton+2,column=6,padx=5,pady=5)
    b1.bind('<ButtonPress-1>',partial(movex,nbutton))
 
 # 2 letter commands:
 #   b1.bind('<ButtonRelease-1>',partial(coarse_stop,'%dM'%(nbutton+1)))

b_cal1 = ttk.Button(f, text="Cal1", command=cal1); b_cal1.grid(row=0, column=0, padx=5, pady=5)
b_cal2 = ttk.Button(f, text="Cal unlimit", command=cal2,state='enable'); b_cal2.grid(row=0, column=1, padx=5, pady=5)
b_cal0 = ttk.Button(f, text="Zero Pos.", command=zero_pos,state='enable'); b_cal0.grid(row=0, column=3, padx=5, pady=5)

b_calX = ttk.Button(f, text="Cal ALL", command=cal1); b_calX.grid(row=1, column=5, padx=5, pady=5)

# Row/column sizes: w is parent widget
#w.columnconfigure(N, option=value, ...)
#w.rowconfigure(N, option=value, ...)
# minsize 	The column or row's minimum size in pixels. If there is nothing in the given column or row, it will not appear, even if you use this option.
# pad 	A number of pixels that will be added to the given column or row, over and above the largest cell in the column or row.
# weight : stretchable, weighted

# b_do1 = ttk.Button(f, text="-", command=mov1, state="disable"); b_do1.grid(row=7, column=2, padx=5, pady=5)

b_lVert = ttk.Label(f, text="Vertical:"); b_lVert.grid(row=7, column=2, padx=5, pady=10)

b_do1 = ttk.Button(f, text="Sweep Start", command=mov1); b_do1.grid(row=8, column=0, padx=5, pady=5)
b_do0 = ttk.Button(f, text="Sweep Zero",  command=mov0); b_do0.grid(row=8, column=2, padx=5, pady=5)
b_do2 = ttk.Button(f, text="Sweep End",   command=mov2); b_do2.grid(row=8, column=4, padx=5, pady=5)

b_lHoriz = ttk.Label(f, text="Horizontal:"); b_lHoriz.grid(row=9, column=2, padx=5, pady=10)

b_Hdo1 = ttk.Button(f, text="Sweep Start", command=movH1); b_Hdo1.grid(row=10, column=0, padx=5, pady=5)
b_Hdo0 = ttk.Button(f, text="Sweep Zero",  command=movH0); b_Hdo0.grid(row=10, column=2, padx=5, pady=5)
b_Hdo2 = ttk.Button(f, text="Sweep End",   command=movH2); b_Hdo2.grid(row=10, column=4, padx=5, pady=5)

#b_test = ttk.Button(f, text="Test"); b_test.grid(row=5, column=3, padx=5, pady=5)
#b_test.bind('<ButtonPress-1>',partial(fstart,1))
#b_test.bind('<ButtonRelease-1>',partial(fstop,1))
#p = ttk.Progressbar(f, orient="horizontal", mode="determinate", maximum=20); 
#p.grid(column=2, row=2, padx=5, pady=5)

ser=None
root.mainloop()

if not(ser==None):
    ser.close()
