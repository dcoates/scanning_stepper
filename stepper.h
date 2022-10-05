#define MIN_PULSE_DUR_USEC 5 // TESTING:1SEC 3 //minimum 2.5 usec. 3 usec to be safe


class StepperState
{
public:
  StepperState(int num_motor, int pin_go, int pin_dir);
  inline void do_update();  // state machine to run every loop

  void prepare_move(signed long pos_end, unsigned long move_duration);
  void start_move();
  void stop_pulse(); // Immediately set pulse low
  void stop_move(unsigned int lower_pulse);  // Stop and cancel future movements.
  virtual unsigned int get_next_interval()=0;

  void debug_output(unsigned long msg);
  
  unsigned int sweeping;	// Public for global access
  unsigned int steps_completed; // global for debugging
  float step_interval_us; // target value, when constant (not LUT)
  word table_counter;		// Public for debugging, LUT

  signed int pos_current;

  // These only refer to tables, so should be in derived class but are here
  // for convenience
  uint8_t *table_ptr;
  unsigned int table_scaler;
  unsigned int table_expander_exponent;
  unsigned int table_interval_min;


  void set_table_info(byte ntable);

private:
  int num_motor;

  // All durations in usec
  unsigned int interval_next;
  float error;

  unsigned long pulse_on_time;
  unsigned long high_time;
  
  // Hardware correspondenc:
  int mypin_pulse;
  int mypin_dir;

  signed long mypos_end;
  signed int mydir; // -1 or +1
};

// Create derived classes just to override read and write digital Fns
// inline them to make them faster
// See the "writeFast" library: improves read/write digital from 5us to <1us
// Currently not using this until we know we need the performance.
class StepperLUT : public StepperState
{
public:
  StepperLUT(int num_motor, int pin_go, int pin_dir);
  unsigned int get_next_interval();
};

// Create derived classes just to override read and write digital Fns
// inline them to make them faster
// See the "writeFast" library: improves read/write digital from 5us to <1us
class StepperConstant : public StepperState
{
public:
  StepperConstant(int num_motor, int pin_go, int pin_dir);
  unsigned int get_next_interval();
};
