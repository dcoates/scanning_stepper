#define GLOBAL_STATE_WAITING 0
#define GLOBAL_STATE_TO_STARTPOS 1
#define GLOBAL_STATE_SWEEPING 2
#define GLOBAL_STATE_TO_ZEROPOS 3

#define MIN_PULSE_DUR_USEC 3 // TESTING:1SEC 3 //minimum 2.5 usec. 3 usec to be safe

#define DIP_SETTING1 xxx
#define DIP_SETTING2
#define DIP_SETTING3

#define DEBOUNCE_DELAY_MS 50


class StepperState
{
public:
	StepperState(int pin_go, int pin_dir);

  void prepare_move(signed int pos_end, unsigned long int move_duration);
  //void prepare_move(signed int pos_end, unsigned long int step_interval_us);
  void start_move();
  void do_update();

  void debug_output(unsigned long msg);

	// All durations in usec
	signed int start_pos_steps;
	signed int end_pos_steps;
	unsigned long int step_interval_us;
	int mypin_pulse;
	int mypin_dir;
  signed int mydir; // -1 or +1
  //unsigned long steps_per_rev;  // defined by motor/driver DIPs. Not using.
  
private:
	long int pulse_off_time;
	long int pulse_on_time;
	signed long int pos_current;
  signed int mypos_end;
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
