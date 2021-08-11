# XYtable
Software for electronically controlled XY table with G1 interpreter and graphical menu for Arduino Mega with TFT screen.
It works based on https://github.com/prenticedavid/MCUFRIEND_kbv and on Arduino enviroment.

Video tutorial:
https://youtu.be/hIaf1pSfH0I

Features:
-Graphical Menu with seperate adjustable stepper motor scalling(pulses per sentimeter).
-Jog with graphical menu options for speed selection and axis selection(XY, X, Y, none). Jog sets up automatically neutral position at startup. Jog can be 2 axis. It switches off during execution of G1 Moves.
-Position with display accuracy of 1 micro(meter). Depending what units you consider as the base unit.


Because of the way Refresh function(G1_Move) is written, G1 moves are accurate at low feeds, but at very fast feeds they are not accurate. Function still ensures the ending of the move is correct. I do not consider this a practical issue as mentioned fast moves are way above normal milling speeds. However it may make a difference with fast moves(using G1 as a G0).
If you want to calculate the inaccuracies spitout the Halfpulse_Time_Micros for each Stepper and Halfpulses_To_Do in G1_Move constructor. Inacuraccies occur due to quantized time to single microseconds (int32_t Halfpulse_Time_Micros resault of division is rounded down. This rounded down part sums Halfpulses_To_Do times during move).
