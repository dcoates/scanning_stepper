## Lookup Tables
Lookups tables are used to specify a delay between steps that will yield the type of motion desired.

For the scissors lifter, the mapping the motion itself is non-linear: a linear horizontal movement results in
a non-linear vertical movement. This was determined to approximate a quadratic. Python code takes the
linear->nonlinear measured mapping, and inverts it (through the quadratic formula) to arrive at the necessary
horizontal motion for a desired lift amount.
