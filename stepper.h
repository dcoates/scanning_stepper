#define GLOBAL_STATE_WAITING 0
#define GLOBAL_STATE_TO_STARTPOS 1
#define GLOBAL_STATE_SWEEPING 2
#define GLOBAL_STATE_TO_ZEROPOS 3

#define MIN_PULSE_DUR_USEC 5 // TESTING:1SEC 3 //minimum 2.5 usec. 3 usec to be safe

#define DIP_SETTING1 xxx
#define DIP_SETTING2
#define DIP_SETTING3

#define DEBOUNCE_DELAY_MS 50


class StepperState
{
public:
	StepperState(int num_motor, int pin_go, int pin_dir);
  inline void do_update();  // state machine to run every loop

  void prepare_move(signed long pos_end, unsigned long move_duration);
  //void prepare_move(signed int pos_end, unsigned long int step_interval_us);
  void start_move();
  
  void stop_pulse(); // Immediately set pulse low
  void stop_move(unsigned int lower_pulse);  // Stop and cancel future movements.

  void debug_output(unsigned long msg);
  
  virtual void writePulseLow(boolean)=0;
  virtual void writePulseHigh(boolean)=0;

  unsigned int sweeping;
  
private:
	// All durations in usec
	float step_interval_us; // target value
  unsigned int interval_next;
  float error;
  
	int mypin_pulse;
	int mypin_dir;
  signed int mydir; // -1 or +1

  signed long pos_current;
  signed long mypos_end;
  int num_motor;
  

	long int pulse_off_time; // TODO: remove
	unsigned long pulse_on_time;
  unsigned long high_time;
//  signed int start_pos_steps;
//  signed int end_pos_steps;
  //unsigned long steps_per_rev;  // defined by motor/driver DIPs. Not using.
};

// Create derived classes just to override read and write digital Fns
// inline them to make them faster
// See the "writeFast" library: improves read/write digital from 5us to <1us
// Currently not using this until we know we need the performance.
class Stepper1 : public StepperState
{
public:
  Stepper1(int num_motor, int pin_go, int pin_dir);
	inline void writePulseLow(boolean) __attribute__((always_inline));
  inline void writePulseHigh(boolean) __attribute__((always_inline));
};

// Create derived classes just to override read and write digital Fns
// inline them to make them faster
// See the "writeFast" library: improves read/write digital from 5us to <1us
class Stepper2 : public StepperState
{
public:
  Stepper2(int num_motor, int pin_go, int pin_dir);
  inline void writePulseLow(boolean) __attribute__((always_inline));
  inline void writePulseHigh(boolean) __attribute__((always_inline));
};


	//long int step_size;
	//long int step_rate; 

	//long int camera_rate;
	//
/* {
  state_ready,          // waiting for a command. Assumed to be in middle position, to run next sequences
  state_move_to_start,  // moving to the start position. Not optimized for speed
  state_sweeping,       // Sweeping from start position to end position
  state_move_to_zero    // moving back from end position to zero position
};
*/
