all:
	arduino-cli compile --fqbn arduino:avr:uno stepper.ino

upload: all
	arduino-cli upload --fqbn arduino:avr:uno -p /dev/ttyACM0 stepper.ino
