#define MIN_PULSE_DUR_USEC 5 // TESTING:1SEC 3 //minimum 2.5 usec. 3 usec to be safe

#define MODE_TO_ZERO 0
#define MODE_TO_START 1
#define MODE_TO_END 2
#define MODE_REVERSING 3 // Hack for sinusoidal that needs to switch directions

#define MODE_CALIBRATING 4 // Go until the Limit slams, then..
#define MODE_CALIBRATING_BACK 5 // Reverse a smidgen to be off limit switch
#define STEPS_TO_REVERSE_OFF_LIMIT 100

class StepperState
{
public:
  StepperState(int num_motor, int pin_go, int pin_dir, int pin_limit, signed long start_pos);
  inline void do_update();  // state machine to run every loop
  void reset_state();

  void prepare_move(signed long pos_end, unsigned long move_duration, int mode);
  void prepare_move_relative(signed long pos_rel, unsigned long move_duration, int mode);
  void start_move();
  void relative_sweep(signed long amount, int mode);

  void stop_pulse(); // Immediately set pulse low
  void stop_move(unsigned int lower_pulse);  // Stop and cancel future movements.
  virtual unsigned int get_next_interval()=0;

  void read_limit();
  
  // These use tone, like the legacy code:
  void smooth_start(int dir, int speed);
  void smooth_stop();

  void debug_output(unsigned long msg);
  
  unsigned int sweeping;	// Public for global access
  unsigned int steps_completed; // global for debugging
  float step_interval_us; // target value, when constant (not LUT)
  float dur_mult; // multiplier for each delay in table (to expand short sections to longer times)
  word table_counter;		// Public for debugging, LUT

  int mode;

  // Positions:
  signed long pos_current;
  signed long pos_start; // Pos at the sweep start (or, just off the limit switch)
  // Also used in the coronal motor to know where to go back to after zero. Changes in sweep

  //Limit switches:
  unsigned char limit_hit=0; 
  unsigned char lims_present=0; // yes or no?
  unsigned char lims_state=1;
  unsigned long lims_last_stable_time=-1; // For debounce
  unsigned long last_limit_read=-1; // so that turn on/off doesn't kill it after a hiatus
  
  // These only refer to tables, so should be in derived class but are here
  // for convenience
  uint8_t *table_ptr;
  unsigned int table_scaler;
  unsigned int table_expander_exponent;
  unsigned int table_interval_min;

  void set_table_info(byte ntable);

  int num_motor;

protected:
  signed int mydir; // -1 or +1

private:

  // All durations in usec
  unsigned int interval_next;
  float error;

  unsigned long pulse_on_time;
  unsigned long high_time;
  
  // Hardware correspondenc:
  int mypin_pulse;
  int mypin_dir;
  int pin_limit;

  signed long mypos_end;
};

class StepperLUT8 : public StepperState // 8 bit
{
public:
  StepperLUT8(int num_motor, int pin_go, int pin_dir, int pin_limit, signed long pos_start);
  unsigned int get_next_interval();
};
class StepperLUT16 : public StepperState // 16 bit
{
public:
  StepperLUT16(int num_motor, int pin_go, int pin_dir, int pin_limit, signed long pos_start);
  unsigned int get_next_interval();
};

// Create derived classes just to override read and write digital Fns
// inline them to make them faster
// See the "writeFast" library: improves read/write digital from 5us to <1us
class StepperConstant : public StepperState
{
public:
  StepperConstant(int num_motor, int pin_go, int pin_dir, int pin_limit, signed long pos_start);
  unsigned int get_next_interval();
};
