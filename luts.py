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

table_val=0
duration = 0
dur_multiply=1.0

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
    start_steps = round( np.interp(start_time, desired_times, xr_steps) )
    start_index = start_steps + TABLE_OFFSET1
    start_location = start_steps + STEPPER1_START

    end_steps = round( np.interp(end_time, desired_times, xr_steps) )
    end_location = end_steps + STEPPER1_START 
    end_index = end_steps + TABLE_OFFSET1

    duration_estimate = np.sum( intervals[start_index:end_index:direction])
    dur_multiply = desired_duration/duration_estimate 

    print( sweep_beg_frac, sweep_end_frac, desired_duration, end='\n')
    #print( "Sweep", sweep_duration_steps, start_index, direction)
    print( "Start", start_location, start_steps, start_time, start_index)
    print( "End  ", end_location, end_steps, end_time, end_index)
    print( "Dur. ", dur_multiply, duration_estimate)
    sweep_duration_steps=0

    return start_location,end_location,dur_multiply,start_index
