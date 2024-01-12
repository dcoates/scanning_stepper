import numpy as np

# Thes settings are partially empirical, from Chloe's measurements of horizontal (linear)
# motion on the scissors, vs. vertical.
#
# Then it is fit with a quadratic which maps desired angular movement to times between
# steps needed to achieve that, to go from min to max in about 3sec.
#
# But now to be more flexible need to interpolate into that table in various ways.

num_steps=7000 # We know (empirically) 7000 steps to go from minimum to maximum

xr_steps=np.arange(num_steps)

desired_y=(49.4145 / num_steps * (xr_steps+1) )

a=2.983135 #3.8687
b=4.28199 #4.8654
c=0.4721
desired_times=(-b+np.sqrt(b**2-4*a*(-desired_y)))/(2*a)
intervals=np.diff(desired_times)

STEPPER1_START= (-32.5+10) * 100
STEPPER1_END  = ( 32.5+10) * 100
TABLE_OFFSET1  = 250 # Empirical from Chloe (stepping back off limit switch?)

STEPPER2_START=-900
STEPPER3_START=95
STEPPER4_START=-30
STEPPER2_END=0
STEPPER3_END=-95
STEPPER4_END=30

STEPPER4_PER_UNIT=50

table_val=0
duration = 0
dur_multiply=1.0

MAX_DEGREES=20.0

MAX_TIME=3.27 # Empirical, from Chloe's titration to yield a symmetric scan
def get_pos(sweep_beg_frac, sweep_end_frac, desired_duration,is_end=False):
    global table_val, duration,dur_multiply
    # nrel position from -1 to 1
    # Need to rescale to make -1 = START and +1 = END (which are empirical from Chloe)
    # Use the -1 to 1 to represent a time, then find stepper position at the appropriate times.
    start_time = (sweep_beg_frac + 1)/2.0 * desired_times[-1]
    end_time   = ( (sweep_beg_frac+1)/2.0 + sweep_end_frac ) * desired_times[-1] 

    if end_time > MAX_TIME:
        end_time = MAX_TIME
    if start_time > MAX_TIME:
        start_time = MAX_TIME

    direction = int( np.sign(sweep_end_frac) )

    # start_steps: positive (0-6500) steps from the begin (backoff-limit) position
    # Index is the index in the lookup table (basically an offset of +250 from 0 (for first relevant entry) )
    # Location is the number historically used by Arduino to represent position (with 0 in the middle/alignment position)
    start_steps = round( np.interp(start_time, desired_times, xr_steps+1) )
    start_index = start_steps + TABLE_OFFSET1*0
    start_location = start_steps + STEPPER1_START

    end_steps = round( np.interp(end_time, desired_times, xr_steps+1) )
    end_location = end_steps + STEPPER1_START 
    end_index = end_steps + TABLE_OFFSET1*0

    duration_estimate = np.sum( intervals[start_index:end_index:direction])
    dur_diff = desired_duration-duration_estimate 
    extra_per_step = dur_diff/(abs(start_index-end_index)-1) * 1e6

    print( sweep_beg_frac, sweep_end_frac, desired_duration, extra_per_step, end='\n')
    #print( "Sweep", sweep_duration_steps, start_index, direction)
    print( "Start", start_location, start_steps, start_time, start_index)
    print( "End  ", end_location, end_steps, end_time, end_index)
    print( "Dur. ", extra_per_step, duration_estimate, dur_diff)
    #sweep_duration_steps=0

    return start_location,end_location,extra_per_step,start_index

def get_pos_new(fraction):
    # New way to get a position for an arbitary "angle" is bsaed on fitting a
    # quadratic to positions that Chloe found to work for a 3sec -20deg to 20deg scan.
    # Fraction: -1=-20deg, 0=0deg, +1=20deg, etc.

    params=[423.83905613, 889.64685276,-2249.69896004]

    # Get any arbitrary position as function of fraction of this scan (timewise)
    original_zero = 3.0/2.0 # Original time=3.0sec
    time = original_zero + fraction*3.0/2.0
    pos=params[0]*time**2+params[1]*time+params[2]
    return pos

#----------------------------------------
# INOUT Table (coronal)
#----------------------------------------

# Time (seconds)
t_max=1.5            # time (seconds) to get from min to max position
angle_start_deg=20   # -angle to start (will go to zero and back)
coronal_steps=900         # Number of stepper motor steps to traverse #800

t=np.linspace(0,t_max,1000) 
desired_pos2=np.cos( t/t_max*np.radians(angle_start_deg))
steps2=np.linspace(1,np.cos(np.radians(angle_start_deg)),coronal_steps-1)
t_desired2 = np.arccos(steps2) * t_max / np.radians(angle_start_deg)
t_delays2 = np.diff(-t_desired2[::-1])

def coronal_pos(degrees,duration):
    frac=degrees/MAX_DEGREES
    t0=t_max*abs(frac)
    pos=int( desired_pos2.shape[0] * (frac-1e-15) )
    steps_needed = int(  np.round(coronal_steps*(desired_pos2[0]-desired_pos2[pos] ) /
                   (desired_pos2[0]-desired_pos2[-1])) )

    # This duration is for total time (double because this table contains only
    # half: going from in to out)
    difference=duration-np.sum(t_delays2[coronal_steps-steps_needed:])*2
    # Divide difference by two to get back the half steps the table uses
    extra_per_step = difference/2.0/(steps_needed)*1e6
    print( pos, extra_per_step, difference, steps_needed, coronal_steps-steps_needed)

    return steps_needed,extra_per_step

def rot_pos(degrees):
    frac=degrees/MAX_DEGREES
    steps=int(round(STEPPER3_START*frac ))
    return steps
