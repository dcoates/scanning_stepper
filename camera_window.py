import tkinter as tk
from tkinter import ttk
from PIL import Image, ImageTk
import numpy as np
#import ttk
from ximea import xiapi,xidefs
import os.path


# multiwin originally from: https://www.pythontutorial.net/tkinter/tkinter-toplevel/
DEFAULT_GEOMETRY="256x256+10+10"

ASPECT_RATIO=1280.0/1024

dim=512
def rgb2Hex(rgb_tuple):
        return '#%02x%02x%02x' % tuple(rgb_tuple)

def open_window(parent,num):
    window = CameraWindow(parent,num)
    return window
        #window.grab_set()

def get_unique_filename(base,ext,code='%s_%03d.%s',start=0):
    num=start
    fullname=code%(base,num,ext) 
    while os.path.exists(fullname):
        num += 1
        fullname=code%(base,num,ext) 
    return fullname

class CameraWindow(tk.Toplevel):
    def __init__(self, parent,num):
        super().__init__(parent)

        self.settings=parent.settings
        self.num=num
        self.name="camera%d"%num # For config settings (..etc..)

        self.update_camera=True
        self.sweeping=False
        self.sweep_image_save_filename="" # Needs to be set before starting sweep

        geom=self.settings.get('%s_geometry'%self.name,DEFAULT_GEOMETRY )
        self.geometry(geom)
        self.title('Camera '+str(num))

        self.nTimeouts=0
                
        #colors = np.random.randint(0,255,(dim,dim),dtype='uint8' )
        #im = Image.fromarray(colors)
        #pi = tk.PhotoImage(im)
        #pi2=ImageTk.PhotoImage(image=im)
        #self.i = tk.PhotoImage(width=dim, height=dim )
        #self.c = tk.Canvas(tk, width=parent.width, height=parent.height-100); c.pack()

        # Start the periodic updates
        self.after(self.settings['camera_update_rate_ms'], self.updater)
        
        self.bind("<Configure>", self.on_window_resize)

        self.val_exposure=tk.DoubleVar()
        self.val_gain=tk.DoubleVar()
        try:
            self.cam=xiapi.Camera(num)
            self.cam.open_device()
            self.cam.set_imgdataformat('XI_MONO8')
            self.expo_min=self.cam.get_exposure_minimum()
            self.expo_max=self.cam.get_exposure_maximum()
            self.gain_min=self.cam.get_gain_minimum()
            self.gain_max=self.cam.get_gain_maximum()

            self.label1 = tk.Label(self)
            self.label1.pack(expand=True)
            
            if self.settings['%s_exposure_log'%self.name]:
                self.slider1 = tk.Scale(self,  showvalue=False, 
                    from_=np.ceil(np.log10(self.expo_min)), to=np.floor( np.log10(self.expo_max) ), orient=tk.HORIZONTAL, resolution=0.05,
                    command=self.slider1_changed, variable=self.val_exposure)
            else:
                self.slider1 = tk.Scale(self,  showvalue=False, 
                    from_=(self.expo_min), to=(self.expo_max), orient=tk.HORIZONTAL, #resolution=0.05,
                    command=self.slider1_changed, variable=self.val_exposure)

            # TODO: Read these defaults from the config file
            expo=float( self.settings.get('%s_exposure'%self.name,self.expo_min) )
            self.slider1.pack(fill=tk.X)
            if self.settings['%s_exposure_log'%self.name]:
                self.slider1.set( np.log10(expo) )
            else:
                self.slider1.set( (expo) )
                
            self.label2 = tk.Label(self)
            self.label2.pack(expand=True)
            self.slider2 = tk.Scale(self, from_=self.gain_min, to=self.gain_max,
                    orient=tk.HORIZONTAL, showvalue=False,
                    command=self.slider2_changed, variable=self.val_gain)

            gain=float( self.settings.get('%s_gain'%self.name,self.gain_min) )
            self.slider2.pack(fill=tk.X)
            self.slider2.set(gain)

            self.b=tk.Button(self,
                text='Snap image',
                command=self.snap).pack( fill=tk.X)
            
            #self.cam.set_exposure(expo)
            #self.cam.set_gain(gain)
            self.cam.set_trigger_source('XI_TRG_OFF')
            self.cam.start_acquisition()
            self.img = xiapi.Image()

        except:
            self.cam = None
            #raise

        # TODO: We shouldn't need both of these, but with only one nothing appears. ??
        self.c = tk.Canvas(self, width=512, height=512); self.c.pack(expand=True)
        #self.c.create_image(1,1,image=pi2)
        self.l = tk.Label(self, width=20, height=20); self.l.pack(expand=True)
        #self.l.image=pi2

        #strings: XI_TRG_OFF, XI_TRG_EDGE_RISING
        # DRC: needed? self.cam.set_gpo_selector("XI_GPO_PORT1") TODO
    def set_camera_trigger_source(self,source='XI_TRG_OFF'):

        #self.update_camera= (source == 'XI_TRG_OFF')

        if not (self.cam is None):
            self.cam.stop_acquisition()
            self.cam.set_trigger_source(source)
            self.cam.start_acquisition()

    def start_sweep(self,sweep_image_filname):
        self.sweep_image_save_filename=sweep_image_filname
        self.sweeping=True
        self.set_camera_trigger_source('XI_TRG_EDGE_RISING')
        #set_camera_trigger_source('XI_GPO_PORT1')

    def stop_sweep(self):
        self.sweeping=False
        self.set_camera_trigger_source('XI_TRG_OFF')

    def snap(self):
        fname=get_unique_filename('image_%s'%self.name,'bmp',code='%s_%03d.%s',start=0)
        self.im.save(fname)
        return

    def slider1_changed(self,event):
        if self.settings['%s_exposure_log'%self.name]:    
            val=10**self.slider1.get()
        else:
            val=self.slider1.get()
        if not( self.cam is None):
            self.cam.set_exposure(val)
        self.settings['%s_exposure'%self.name]=val
        self.label1.config(text='Exposure: %0.0f'%val )

    def slider2_changed(self,event):
        val=self.slider2.get()
        if not( self.cam is None):
            self.cam.set_gain(val)
            print('set gain')
        self.settings['%s_gain'%self.name]=val
        self.label2.config(text='Gain: %0.0f'%val )

    def on_window_resize(self,event):
        w,h = event.width, event.height
        w = self.winfo_width() 
        w=w*0.75
        self.c.config(width=w,height=w)
        self.settings['%s_geometry'%self.name]=self.geometry()

    def updater(self):
        valid_image=False
        # If there is a camera, use it.
        # (Can be either free-running or GPIO triggered.)
        # Hopefully the GPIO triggered slows this loop down
        if not (self.cam is None):
            try:
                self.cam.get_image(self.img, timeout=10)
                valid_image=True
                
                self.data = self.img.get_image_data_numpy()
                self.im = Image.fromarray(self.data)
    
                self.nTimeouts=0
            except xiapi.Xi_error as err:
                if err.status == 10:
                    print('Timeout' + str(self.nTimeouts) )
                    self.nTimeouts += 1
                else:
                    print(err.status)
                    
                colors = np.random.randint(0,255,(dim,dim),dtype='uint8' )
                self.im = Image.fromarray(colors)
                pi = tk.PhotoImage(self.im)
                pi2=ImageTk.PhotoImage(image=self.im)            

            w = self.winfo_width() 
            if w<100: w=100 # Very first time w is tiny, causing error.. Workaround
            im_resized = self.im.resize((int(w*0.75),int(w*0.75/ASPECT_RATIO)), Image.ANTIALIAS)

            pi2=ImageTk.PhotoImage(im_resized) #,mode='L')
        else:
            colors = np.random.randint(0,255,(dim,dim),dtype='uint8' )
            self.im = Image.fromarray(colors)
            pi = tk.PhotoImage(self.im)
            pi2=ImageTk.PhotoImage(image=self.im)

        # IF there is a real camera image, and we are sweeping, save the file
        if self.sweeping and valid_image:
            filename=get_unique_filename(self.sweep_image_save_filename,'bmp',code='%s_%03d.%s',start=0)
            self.im.save(filename)

        # For now, need both these lines. NOT GOOD. TODO
        self.c.create_image(0,0, image=pi2, anchor=tk.NW)
        self.l.image = pi2

        #print (self.geometry() );
        if self. sweeping:
            self.after( int(self.settings['camera_sweep_snap_rate_ms']), self.updater)
        else:
            self.after(self.settings['camera_update_rate_ms'], self.updater)

    def stop(self):
        if not( self.cam is None): 
            self.cam.stop_acquisition()
            self.cam.close_device()

class App(tk.Tk):
    def __init__(self):
        super().__init__()

        self.geometry('300x200')
        self.title('Main Window')

        # place a button on the root window
        ttk.Button(self,
                text='Open a window',
                command=self.open_window).pack(expand=True)

        self.open_window(0)
        self.open_window(1)

    def open_window(self,num):
        window = Window(self,num)
        #window.grab_set()

if __name__ == "__main__":
    app = App()
    app.mainloop()
