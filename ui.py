from tkinter import *
from tkinter import ttk
import tkinter
import serial
from serial.tools.list_ports import comports
from functools import partial
import xml.etree.ElementTree as ET

import os
import os.path

import camera_window

### Config/settings
SETTINGS={}

def read_config(fname='./config.xml'):
    settings={}
    tree=ET.parse(fname)

    for child in tree.getroot(): # Assume root is "settings"
        print( child.tag, child.text )
        settings[child.tag]=child.text
    return settings

def write_config(settings, fname='./config.xml'):
    tree = ET.Element('settings')
    for key,val in settings.items():
        child=ET.Element(key)
        child.text=str(val)
        tree.append(child)
    ET.indent(tree, '    ')
    ET.ElementTree(tree).write(fname,encoding='utf-8',xml_declaration=True)

## Serial stuff:
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

    lin=ser.readline()
    if len(lin)>0:
        print('Ser:', lin, end='', flush=True)

        if b'ready' in lin:
            theApp.l_status.configure(text='READY', foreground='green' )
        elif b'pos1' in lin:
            theApp.l_p1.configure(text=lin[0:])
        elif b'pos2' in lin:
            theApp.l_p2.configure(text=lin[0:])
        elif b'pos3' in lin:
            theApp.l_p3.configure(text=lin[0:])
        elif b'pos4' in lin:
            theApp.l_p4.configure(text=lin[0:])

    theApp.after(10, lambda: check_serial()) # Poll often (10ms)

def getpos():
    ser.write(b'p');

# TODO: All the single-letter commands could call a single function (doing ser.write), with a dictionary of
# human-readable mapping from command to letter code.
def mov0():
    ser.write(b'Z');
def mov1():
    ser.write(b'S');
def mov2():
    start_sweep('V')
    ser.write(b'F');

def movH0():
    ser.write(b'z');
def movH1():
    ser.write(b's');
def movH2():
    start_sweep('H')
    ser.write(b'f');

def movD0():
    ser.write(b'O'); # Zero
def movD1():
    ser.write(b'I'); # Initial
def movD2():
    start_sweep('D')
    ser.write(b'L'); # Last

def start_sweep(str_which,path_results_base='results'):
    path_sweepfiles=theApp.str_filename.get()
    if not(os.path.exists(path_results_base)):
        os.mkdir(path_results_base)
    if not(os.path.exists(os.path.join(path_results_base,path_sweepfiles))):
        os.mkdir(os.path.join(path_results_base,path_sweepfiles))
    nSweep=0
    while True:
        fullpath=os.path.join(path_results_base,path_sweepfiles,'sweep_%02d'%nSweep )
        if os.path.exists(fullpath):
            nSweep += 1
        else:
            os.mkdir(fullpath)
            break

    filename0=os.path.join(fullpath,'sweep_cam%d_%s'%(0,str_which) )
    filename1=os.path.join(fullpath,'sweep_cam%d_%s'%(1,str_which) )
    theApp.cam0.start_sweep(filename0)
    theApp.cam1.start_sweep(filename1)

def reset_trig():
    theApp.cam0.stop_sweep()
    theApp.cam1.stop_sweep()
    #theApp.cam0.set_camera_trigger_source()
    #theApp.cam1.set_camera_trigger_source()

# These won't work since the widget's aren't exposed:
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
    ser=serial.Serial(portname,baudrate=SETTINGS['baudrate'],timeout=0)
    theApp.after(10, lambda: check_serial())

def ser_command(arg,evnt):
    ser.write(arg)

def movex(arg,evnt):
    s=('%s%c'%(E_amt.get(),chr(ord('A')+arg) )).encode()
    ser.write(s)

# Create the widget UI
class App(Frame):
    def __init__(self,parent,SETTINGS):
        super().__init__()

        self.settings=SETTINGS

        #root = self # Tk()
        #root.title('Scanning WFS - Simple Controller UI')
        #root.geometry('900x512')
        f = ttk.Frame(parent, width=512); f.grid()

        str_port=StringVar();
        list_ports = ttk.Combobox(f,textvariable=str_port); add_coms( list_ports); list_ports.grid(row=1, column=6,padx=5,pady=5)
        #l_port = ttk.Label(f, text="Port"); l_port.grid(row=1, column=5, padx=5, pady=5)
        list_ports.bind("<<ComboboxSelected>>", lambda x: b_connect.configure(state="enable"))
        self.l_status = ttk.Label(f, text="NOT Connected", foreground='red'); self.l_status.grid(row=0, column=6, padx=5, pady=5)
        self.b_connect = ttk.Button(f, text="Connect!", command=partial(start,str_port.get() ),state='disable')
        self.b_connect.grid(row=1,column=7, padx=5, pady=5)

        b_pos = ttk.Button(f, text="Get pos", command=getpos).grid(row=1, column=2, padx=5, pady=5)
        self.l_p1 = ttk.Label(f, text="Pos1:"); self.l_p1.grid(row=2, column=2, padx=5, pady=5)
        self.l_p2 = ttk.Label(f, text="Pos2:"); self.l_p2.grid(row=3, column=2, padx=5, pady=5)
        self.l_p3 = ttk.Label(f, text="Pos3:"); self.l_p3.grid(row=4, column=2, padx=5, pady=5)
        self.l_p4 = ttk.Label(f, text="Pos4:"); self.l_p4.grid(row=5, column=2, padx=5, pady=5)

        b_finesR=[ttk.Button(f, text='%d+'%(n+1)) for n in range(4)]
        b_coarsesR=[ttk.Button(f, text='%d++'%(n+1)) for n in range(4)]
        b_finesL=[ttk.Button(f, text='-%d'%(n+1)) for n in range(4)]
        b_coarsesL=[ttk.Button(f, text='--%d'%(n+1)) for n in range(4)]

        b_cals=[ttk.Button(f, text='CAL%d'%(n+1)) for n in range(4)]

        E_amt = Entry(f,textvariable=-100)
        E_amt.grid(row=6,column=6,padx=5,pady=5)

        b_moves=[ttk.Button(f, text='Move %d'%(n+1)) for n in range(4)]

        # Each one goes: --,-,+,++ . They are in the top letter row of an asdf keyboard. Caps for big.
        codes=[[b'Q',b'q',b'w',b'W'], [b'E',b'e',b'r',b'R'],[b'T',b't',b'y',b'Y'], [b'U',b'u',b'i',b'I'], [b'a',b'b',b'c',b'd'] ]

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

        b_cal1 = ttk.Button(f, text="Cal1", command=cal1); b_cal1.grid(row=0, column=0, padx=5, pady=5)
        b_cal2 = ttk.Button(f, text="Cal unlimit", command=cal2,state='enable'); b_cal2.grid(row=0, column=1, padx=5, pady=5)
        b_cal0 = ttk.Button(f, text="Zero Pos.", command=zero_pos,state='enable'); b_cal0.grid(row=0, column=3, padx=5, pady=5)

        # Those are not used anymore. Eventually remove. For now disable.
        b_cal1.configure(state="disable")
        b_cal2.configure(state="disable")
        b_cal0.configure(state="disable")

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

        b_lDiag = ttk.Label(f, text="Diagonal:"); b_lDiag.grid(row=11, column=2, padx=5, pady=10)
        b_Ddo1 = ttk.Button(f, text="Sweep Start", command=movD1); b_Ddo1.grid(row=12, column=0, padx=5, pady=5)
        b_Ddo0 = ttk.Button(f, text="Sweep Zero",  command=movD0); b_Ddo0.grid(row=12, column=2, padx=5, pady=5)
        b_Ddo2 = ttk.Button(f, text="Sweep End",   command=movD2); b_Ddo2.grid(row=12, column=4, padx=5, pady=5)

        self.l_name = ttk.Label(f, text="Subject ID:", anchor=tkinter.E); self.l_name.grid(row=11, column=5, padx=0, pady=5)
        self.str_filename=StringVar(); #"Enter Subject ID");
        self.str_filename.set("TEST")
        e_sweep_filename = ttk.Entry(f, textvariable=self.str_filename); e_sweep_filename.grid(row=11, column=6, padx=5, pady=5)
        b_reset_trigger = ttk.Button(f, text="Reset Cam. Trig", command=reset_trig); b_reset_trigger.grid(row=12, column=6, padx=5, pady=5)

ser=None

SETTINGS=read_config() # Reads into global SETTINGS dict

root = Tk()
theApp=App(root,SETTINGS);

if int(SETTINGS['camera_preview'])>0:
    theApp.cam0=camera_window.open_window(theApp,0)
    theApp.cam1=camera_window.open_window(theApp,1)


if not(SETTINGS['autoconnect'] is None) and not (SETTINGS['autoconnect']=="None"):
    theApp.after(100, partial(start,SETTINGS['autoconnect']) )

theApp.mainloop()

if not(ser==None):
    ser.close()

theApp.cam0.stop()
theApp.cam1.stop()

write_config(SETTINGS)

