# XYtable
Software for electronically controlled XY table with G1 move support and graphical menu for Arduino Mega with TFT screen.
It works based on https://github.com/prenticedavid/MCUFRIEND_kbv and on Arduino enviroment.

Video of coding:
https://youtu.be/hIaf1pSfH0I

Features:
-Graphical Menu with seperate adjustable stepper motor scalling(pulses per sentimeter).
-Jog with graphical menu options for speed selection and axis selection(XY, X, Y, none). Jog sets up automatically neutral position at startup. Jog can be 2 axis. It switches off during execution of G1 Moves.
-Position with display accuracy of 1 micro(meter). Depending what units you consider as the base unit.


Because of the way Refresh function(G1_Move) is written, G1 moves are accurate at low feeds, but at very fast feeds they are not accurate. Function still ensures the ending of the move is correct. I do not consider this a practical issue as mentioned fast moves are way above normal milling speeds. However it may make a difference with fast moves(using G1 as a G0).
If you want to calculate the inaccuracies spitout the Halfpulse_Time_Micros for each Stepper and Halfpulses_To_Do in G1_Move constructor. Inacuraccies occur due to quantized time to single microseconds (int32_t Halfpulse_Time_Micros resault of division is rounded down. This rounded down part sums Halfpulses_To_Do times during move).
Software is free to use. If you like to support me I highly appreciate donations: btc: 1H8XwyQogdTFekH9w5CPonKFq9upmiqZ1P

Overall structure looks this way that there is a Stepper_Driver class maintaining each separate stepper motor. It knows its position, pin connections to stepper driver etc.
Class G1_Move looks up into multiple Stepper_Driver classes and performs moves ensuring Stepper_Driver's are ready.
There is also a Moves_Buffer[] that holds future movements to which you can add new move using Add_Position_To_Buffer() function
Moves_Buffer works as a cirkular buffer if it is full Add_Position_To_Buffer() will return false and wont add any more moves to the buffer( including the one just tried to add )
Moves are executed automatically in main until they are all finished.
GUI controlls all the setup options including movement scalling and jog control.
Jog control is not available while performing a G1 move.
Jog neutral position is being set during start-up.
Stepper_Driver can also set constant stepper speed movement using Set_Speed(). Just keep in mind that in main function jog position is used to call Set_Speed() function just when there is no G1 move performed, so if you want to use Set_Speed() manuall, you should disable this. if (Jog_Setup == ... Y_Axis.Set_Speed(Speed...

Licence: MIT free use, distribute and profit with this copyright notice
By GenOme, https://github.com/63n0m3/XYtable/
Btc: 1H8XwyQogdTFekH9w5CPonKFq9upmiqZ1P
