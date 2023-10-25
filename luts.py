import numpy as np

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
TABLE_START1  = 250

table_val=0
duration = 0
dur_multiply=1.0

def get_pos(sweep_beg_frac, sweep_end_frac, desired_duration,is_end=False):
    global table_val, duration,dur_multiply
    # nrel position from -1 to 1
    # Need to rescale to make -1 = START and +1 = END (empirical from Chloe)
    start_time = (sweep_beg_frac + 1)/2.0 * desired_times[-1]
    if sweep_end_frac > 0:
        # 0.96 was empirical, to make +1 equal to STEPPER1_END
        end_time = (sweep_end_frac + 1)/2.0 * desired_times[-1] * 0.9574  # * ( (STEPPER1_END-STEPPER1_START)/7000)
        sweep_duration_steps = round( np.interp(end_time, desired_times, xr_steps) )
        print( sweep_duration_steps)
    table_val = round( np.interp(start_time, desired_times, xr_steps) )
    print( table_val )
    stepnum = table_val + STEPPER1_START
    table_val += TABLE_START1
    duration = np.sum( intervals[table_val:table_val+sweep_duration_steps])
    dur_multiply = desired_duration/duration
    print( desired_duration, duration, dur_multiply )
    return stepnum,table_val
