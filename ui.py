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
import luts
import numpy as np
import time

HORIZ_SENTINEL=9999

VSWEEP_START=-20
SWEEP_DUR=3

### Config/settings
SETTINGS={}

new_sweep_count=0;
sweep_steps=26

def read_config(fname='./config.xml'):
    settings={}
    tree=ET.parse(fname)

    for child in tree.getroot(): # Assume root is "settings"
        #print( child.tag, child.text )
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
        #print( lb['values'] )
        #lb.insert(nport+1,port1.device) # For a listbox
        list_ports += [port1.device]
    lb['values']=list_ports

def check_serial():
    global ser

    lin=ser.readline()
    if len(lin)>0:
        lin=lin.decode() #lin=lin.replace('\r\n','\n')
        print(lin, end='', flush=True)

        if 'ready' in lin:
            theApp.l_status.configure(text='READY', foreground='green' )
        elif 'pos1' in lin:
            theApp.l_p1.configure(text=lin[0:])
        elif 'pos2' in lin:
            theApp.l_p2.configure(text=lin[0:])
        elif 'pos3' in lin:
            theApp.l_p3.configure(text=lin[0:])
        elif 'pos4' in lin:
            theApp.l_p4.configure(text=lin[0:])

    theApp.after(10, lambda: check_serial()) # Poll often (10ms)

def getpos():
    ser.write(b'p');
def getdbg():
    ser.write(b'?');

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
    ser.write(b'K'); # Initial
def movD2():
    start_sweep('D')
    ser.write(b'L'); # Last

def movD0_2():
    ser.write(b'o'); # Zero
def movD1_2():
    ser.write(b'k'); # Initial
def movD2_2():
    start_sweep('D2')
    ser.write(b'l'); # Last

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
    global new_sweep_count

    send_PVT(arg,evnt) # First send new table

    pos1 = float( E_pos1.get() )
    pos2 = float( E_pos2.get() )
    pos3 = -pos1
    pos4 = float( E_horiz_pos.get() )
    s=('%d,%d,%d,%d,%c'%(pos1,pos2,pos3,pos4,chr(ord('A')+arg) )).encode()
    ser.write(s)

def make_positions(npos=27,use_UI=True):

    if use_UI:
        #sweep_params= E_sweep.get().split()
        #sweep_begin = int( sweep_params[0] )
        #sweep_dur = int( sweep_params[1] )

        sweep_begin = float(E_pos1.get() )
        sweep_dur = float(E_pos2.get() )
        sweep_end = -sweep_begin

        horiz_sweep_begin = float( E_horiz_pos.get() )
    else:
        sweep_begin = -10
        sweep_dur=3
        sweep_end=10

        horiz_sweep_begin = -20

    horiz_sweep_end = -horiz_sweep_begin
    if horiz_sweep_begin==HORIZ_SENTINEL:
        horiz_sweep_end = HORIZ_SENTINEL
    else:
        horiz_sweep_begin *= luts.STEPPER4_PER_UNIT
        horiz_sweep_end *= luts.STEPPER4_PER_UNIT

    begin_frac = sweep_begin/luts.MAX_DEGREES
    end_frac = sweep_end/luts.MAX_DEGREES

    #stepnum,step_end,dur_extra,table_val=luts.get_pos(begin_frac, end_frac, sweep_dur)
    coronal_pos,coronal_tfrac=luts.coronal_pos( abs(sweep_begin), sweep_dur  )
    rot_pos=-luts.rot_pos( sweep_begin  )

    #print( begin_frac, coronal_pos, rot_pos )

    times=np.linspace(0,3,npos)
    fracs=np.linspace(begin_frac,end_frac,npos)
    angles=np.linspace(begin_frac,end_frac,npos)
    #pos1=np.linspace(stepnum, step_end, npos)
    #pos1=[luts.get_pos(angl1,end_frac,sweep_dur)[0] for angl1 in angles]
    pos1=[luts.get_pos_new(frac1) for frac1 in fracs]
    pos2=[luts.coronal_pos(abs(-angl1*luts.MAX_DEGREES),sweep_dur)[0] for angl1 in angles]
    pos3=np.linspace(rot_pos, -rot_pos, npos)
    pos4=np.linspace(horiz_sweep_begin,horiz_sweep_end,npos)
    return angles,pos1,pos2,pos3,pos4


def add_pos(combo):
    angles,pos1,pos2,pos3,pos4=make_positions()
    combo_entries=[]
    for nwhich,nval in enumerate(angles):
        combo_entries += ['%2d:(%+3d) %+5d %+4d %+3d'%(nwhich,int(nval*20),pos1[nwhich],-pos2[nwhich],pos3[nwhich]) ]

    list_pos["values"]=combo_entries

def regen_pos(arg,evnt):
    add_pos(arg)

def choose_pos(event):
    vals=list_pos.get().split() 
    # Reference from the end with negs. since the beginning might have extra spaces, etc.
    E_pos1.delete(0,"end");
    E_pos1.insert(0,vals[-3])
    E_pos2.delete(0,"end");
    E_pos2.insert(0,vals[-2])
    E_pos3.delete(0,"end");
    E_pos3.insert(0,vals[-1] )

def send_PVT(arg,evnt):
    global new_sweep_count

    angles,pos1,pos2,pos3,pos4=make_positions()
    s=('*').encode()
    ser.write(s)
    time.sleep(0.02)
    new_sweep_count=0

    for nidx in np.arange(1,sweep_steps+1):
        s=('%d,%d,%d,%dv'%(pos1[nidx],-pos2[nidx],pos3[nidx],pos4[nidx])).encode()
        print( s )
        ser.write(s)
        time.sleep(0.02)
    print()

    # Now move
    s=('%d,%d,%d,%d,%c'%(pos1[0],-pos2[0],pos3[0],pos4[0],chr(ord('A')+arg) )).encode()
    ser.write(s)

    # Now move
    #s=('%d,%d,%d,%d,%c'%(stepnum,-coronal_pos,rot_pos,horiz_sweep_begin,chr(ord('A')+arg) )).encode()
    #ser.write(s)

    #b_sweeps[0].configure(text="Sweep X "%(new_sweep_count+1,sweep_steps) )

def sweepx(arg,evnt):
    global new_sweep_count

    s='V'.encode()
    ser.write(s)
    #s=('%d,%d,%d,%0.4f,%d,%d,%0.4f,%d,%d,%d,%c'%(stepnum,step_end,sweep_dur*1e6,extra_per_step,table_val,
    #-coronal_pos,coronal_extra,rot_pos,horiz_sweep_begin,horiz_sweep_end,chr(ord('B')+arg) )).encode()
    #ser.write(s)

    #b_sweeps[0].configure(text="Sweep Step #%d/%d"%(new_sweep_count+1,sweep_steps))
    #new_sweep_count += 1

# Create the widget UI
class App(Frame):
    def __init__(self,parent,SETTINGS):
        global E_pos1, E_pos2 , E_pos3, E_sweep
        global E_horiz_pos
        global b_sweeps
        global list_pos

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
        b_pos = ttk.Button(f, text="DEBUG", command=getdbg).grid(row=1, column=3, padx=5, pady=5)
        self.l_p1 = ttk.Label(f, text="Pos1:"); self.l_p1.grid(row=2, column=2, padx=5, pady=5)
        self.l_p2 = ttk.Label(f, text="Pos2:"); self.l_p2.grid(row=3, column=2, padx=5, pady=5)
        self.l_p3 = ttk.Label(f, text="Pos3:"); self.l_p3.grid(row=4, column=2, padx=5, pady=5)
        self.l_p4 = ttk.Label(f, text="Pos4:"); self.l_p4.grid(row=5, column=2, padx=5, pady=5)

        b_finesR=[ttk.Button(f, text='%d+'%(n+1)) for n in range(4)]
        b_coarsesR=[ttk.Button(f, text='%d++'%(n+1)) for n in range(4)]
        b_finesL=[ttk.Button(f, text='-%d'%(n+1)) for n in range(4)]
        b_coarsesL=[ttk.Button(f, text='--%d'%(n+1)) for n in range(4)]

        b_cals=[ttk.Button(f, text='CAL%d'%(n+1)) for n in range(4)]

        #########################
        ## New segmented movement
        #########################
        E_pos1 = Entry(f)
        E_pos1.grid(row=6,column=5,padx=5,pady=5)
        E_pos2 = Entry(f)
        E_pos2.grid(row=7,column=5,padx=5,pady=5)
        #E_pos3 = Entry(f)
        #E_pos3.grid(row=8,column=5,padx=5,pady=5)

        E_pos1.insert(0,str(VSWEEP_START));  #default
        E_pos2.insert(0,str(SWEEP_DUR));  #default
        #E_pos3.insert(0,str(-VSWEEP_START));  #default

        l_start = ttk.Label(f, text="VSweep:", justify="right"); l_start.grid(row=6, column=4, padx=5, pady=5)
        l_dur = ttk.Label(f, text="Dur:", justify="right"); l_dur.grid(row=7, column=4, padx=5, pady=5)
        l_dur = ttk.Label(f, text="HSweep:", justify="right"); l_dur.grid(row=5, column=6, padx=5, pady=5)
        l_end = ttk.Label(f, text="End:", justify="right"); l_end.grid(row=8, column=4, padx=5, pady=5)
        #l_dur = ttk.Label(f, text="VSweep dur:", justify="right"); l_dur.grid(row=7, column=6, padx=5, pady=5)

        E_horiz_pos = Entry(f); E_horiz_pos.grid(row=6,column=6,padx=5,pady=5)
        E_horiz_pos.insert(0,HORIZ_SENTINEL);  #default
        #E_sweep = Entry(f); E_sweep.grid(row=8,column=6,padx=5,pady=5)
        #E_sweep.insert(0,'%d %d'%(VSWEEP_START,SWEEP_DUR)); 

        b_moves=[ttk.Button(f, text='Move %d'%(n+1)) for n in range(1)]
        b_sweeps=[ttk.Button(f, text='Sweep End %d'%(n+1)) for n in range(1)]
        b_prep=[ttk.Button(f, text='Sweep Start %d'%(n+1)) for n in range(1)]
        b_regen=[ttk.Button(f, text='<-Regen') for n in range(1)]
        #for nbutton,b1 in enumerate(b_moves):
            #b1.grid(row=nbutton+6,column=7,padx=5,pady=5)
            #b1.bind('<ButtonPress-1>',partial(movex,nbutton))
        for nbutton,b1 in enumerate(b_prep):
            b1.grid(row=nbutton+7,column=7,padx=5,pady=5)
            b1.bind('<ButtonPress-1>',partial(send_PVT,nbutton))
        for nbutton,b1 in enumerate(b_sweeps):
            b1.grid(row=nbutton+8,column=7,padx=5,pady=5)
            b1.bind('<ButtonPress-1>',partial(sweepx,nbutton))

        sty_fixed = ttk.Style()
        sty_fixed.configure('StyFixed.TComboBox', font=('TkFixedFont', 16))
        str_pos=StringVar();
        root.option_add("*TCombobox*Listbox*Font", 'TkFixedFont')
        list_pos = ttk.Combobox(f,textvariable=str_pos, width=30)
        add_pos( list_pos); list_pos.grid(row=9, column=6,padx=5,pady=5)
        list_pos.bind("<<ComboboxSelected>>", choose_pos )

        #for nbutton,b1 in enumerate(b_regen):
            #b1.grid(row=nbutton+9,column=7,padx=5,pady=5)
            #b1.bind('<ButtonPress-1>',partial(regen_pos,list_pos))

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

        b_lDiag2 = ttk.Label(f, text="Diagonal2:"); b_lDiag2.grid(row=13, column=2, padx=5, pady=10)
        b_D2do1 = ttk.Button(f, text="Sweep Start", command=movD1_2); b_D2do1.grid(row=14, column=0, padx=5, pady=5)
        b_D2do0 = ttk.Button(f, text="Sweep Zero",  command=movD0_2); b_D2do0.grid(row=14, column=2, padx=5, pady=5)
        b_D2do2 = ttk.Button(f, text="Sweep End",   command=movD2_2); b_D2do2.grid(row=14, column=4, padx=5, pady=5)
        #b_do1.configure(state='disable');
        b_do2.configure(state='disable');
        #b_Hdo1.configure(state='disable');
        b_Hdo2.configure(state='disable');
        #b_Ddo1.configure(state='disable');
        b_Ddo2.configure(state='disable');

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

