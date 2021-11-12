//This software was coded together with in deepth software explanation via a video tutorial:
//  https://youtu.be/hIaf1pSfH0I
//by GenOme, https://github.com/63n0m3/XYtable/
//btc: 1H8XwyQogdTFekH9w5CPonKFq9upmiqZ1P

#include <EEPROM.h>
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
MCUFRIEND_kbv tft;
#include <TouchScreen.h>
#include <FreeDefaultFonts.h>
#define MINPRESSURE 200
#define MAXPRESSURE 1000

#define EEPROM_Z_ENABLED 0
#define EEPROM_JOG_TYPE 1
#define EEPROM_X_PULSES 2
#define EEPROM_Y_PULSES 6
#define EEPROM_Z_PULSES 10
#define EEPROM_X_START_POS 14
#define EEPROM_y_START_POS 18
#define EEPROM_z_START_POS 22
#define EEPROM_JOG_X_MID_POS 26
#define EEPROM_JOG_Y_MID_POS 28
#define EEPROM_JOG_SENS 30 /// NEXT AT 31!!!

const int XP=8,XM=A2,YP=A3,YM=9; //240x320 ID=0x9595
const int TS_LEFT=876,TS_RT=139,TS_TOP=390,TS_BOT=769;

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

void Write_To_Eeprom(uint16_t addr, uint16_t var){
  byte bl = var & 0xff;
  byte bh = (var>>8) & 0xff;
  EEPROM.write(addr, bl);
  EEPROM.write(addr+1, bh);  
}
uint16_t Read_Eeprom(uint16_t addr){
  byte bl = EEPROM.read(addr);
  byte bh = EEPROM.read(addr + 1);
  uint16_t ret = (bh<<8) + bl;
  return ret;
}
void Write_To_Eeprom(uint16_t addr, int32_t var){
  byte bl = var & 0xff;
  byte lm = (var>>8) & 0xff;
  byte hm = (var>>16) & 0xff;
  byte bh = (var>>24) & 0xff;
  EEPROM.write(addr, bl);
  EEPROM.write(addr+1, lm); 
  EEPROM.write(addr+2, hm); 
  EEPROM.write(addr+3, bh);  
}
int32_t Read_Eeprom4b(uint16_t addr){
  byte bl = EEPROM.read(addr);
  byte lm = EEPROM.read(addr + 1);
  byte hm = EEPROM.read(addr + 2);
  byte bh = EEPROM.read(addr + 3);
  int32_t ret = (bh<<24) + (hm<<16) + (lm<<8) + bl;
  return ret;
}
int Cursor_X, Cursor_Y;
bool Cursor_Pressed;
bool Touch_getXY(void)
{
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);      //restore shared pins
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);   //because TFT control pins
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    if (pressed) {
        Cursor_X = map(p.x, 913, 128, 0, tft.width());
        Cursor_Y = map(p.y, 97, 909, 0, tft.height());
    }
    return pressed;
}

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF


int32_t Pulses_per_centimeter_X;
int32_t Pulses_per_centimeter_Y;
#define STEPPER_X_DIRECTION 19
#define STEPPER_X_PULSE 18
#define STEPPER_Y_DIRECTION 21
#define STEPPER_Y_PULSE 20
#define JOG_X_ANALOG_PIN A14
#define JOG_Y_ANALOG_PIN A15


//#define MAX_LAST_PULSE_NO 0xffffffff

class Stepper_Driver{
public:
  uint8_t Pulse_Pin;
  uint8_t Direction_Pin;
  int32_t Pulses_Per_Centimeter;
  uint32_t Halfpulse_Time_micros;
  int8_t Current_Direction; ///-1 negative, 0 none,  1 positive
  uint32_t Last_Pulse_Time;
  bool Last_Pulse_State;
  int32_t Current_Position_Pulses;
  bool Currently_Used;
public:
  Stepper_Driver(uint8_t pulse_pin, uint8_t direction_pin, int32_t pulses_per_centimeter){
    Pulse_Pin = pulse_pin;
    Direction_Pin = direction_pin;
    Pulses_Per_Centimeter = pulses_per_centimeter;
    pinMode(direction_pin, OUTPUT);
    pinMode(pulse_pin, OUTPUT);
    Current_Direction = 0;
    Current_Position_Pulses = 0;
    Last_Pulse_Time = 0;
    Currently_Used = false;
  }
  
  void Set_Speed(int16_t speed_micrometers, uint32_t time_at_check){
    Last_Pulse_Time = time_at_check;
    if(Currently_Used == true) return;
    if (Current_Direction>0 && speed_micrometers<=0){
      Last_Pulse_State = LOW;
      Current_Direction = -1;
      digitalWrite (Pulse_Pin, Last_Pulse_State);
      digitalWrite (Direction_Pin, LOW);
    }
    else if (Current_Direction<0 && speed_micrometers>=0){
      Last_Pulse_State = LOW;
      Current_Direction = 1;
      digitalWrite (Pulse_Pin, Last_Pulse_State);
      digitalWrite (Direction_Pin, HIGH);
    }
    else if (Current_Direction == 0){
      if (speed_micrometers>0){
        Current_Direction = 1;
        digitalWrite (Direction_Pin, HIGH);
      }
      else if (speed_micrometers<0){
        Current_Direction = -1;
        digitalWrite (Direction_Pin, LOW);
      }
    }
    if (speed_micrometers == 0) Current_Direction = 0;
    else Halfpulse_Time_micros = abs(100000/speed_micrometers); ///This is not really calculated!

  }
  void Flip_Pulse_Pin(){
    Last_Pulse_State = !Last_Pulse_State;
    digitalWrite (Pulse_Pin, Last_Pulse_State);
    if (Last_Pulse_State == HIGH){
      if (Current_Direction == 1)
        Current_Position_Pulses++;
      else if (Current_Direction == -1)
       Current_Position_Pulses--;
    }
  }
  void Reset_Position_To_0(){
    Current_Position_Pulses = 0;
  }
  void Set_Position_To_Micrometers(int32_t set_pos_to){
    Current_Position_Pulses = (int32_t)(((int64_t)set_pos_to * Pulses_Per_Centimeter)/10000);
  }
  int32_t Get_Current_Position_Micrometers(){
    return (int32_t)((int64_t)Current_Position_Pulses*10000)/Pulses_Per_Centimeter;
  }
  void Change_Pulses_Per_Centimeter_To(int32_t pulses){
      int32_t Old_Pulses_Per_Cm = Pulses_Per_Centimeter;
      Pulses_Per_Centimeter = pulses;
      int32_t Current_Position_Pulses = (int32_t)((int64_t)(pulses*Current_Position_Pulses)/Pulses_Per_Centimeter);
  }
  
  void Refresh(uint32_t time_at_check){
    if(Currently_Used == true) return;
    if (Current_Direction == 0) return;
    if (time_at_check>Last_Pulse_Time+Halfpulse_Time_micros){
        if (Last_Pulse_State == HIGH) Last_Pulse_State = LOW;
        else Last_Pulse_State = HIGH;
        digitalWrite (Pulse_Pin, Last_Pulse_State);
        if (Last_Pulse_State == HIGH){
          if (Current_Direction == 1)
            Current_Position_Pulses++;
          else if (Current_Direction == -1)
            Current_Position_Pulses--;
        }        
        Last_Pulse_Time = Last_Pulse_Time + Halfpulse_Time_micros;
      }
      
    }
};

class G1_Move{
private:
  Stepper_Driver* Stepper1;
  Stepper_Driver* Stepper2;
  int32_t Halfpulses_To_Do_Stepper1;
  int32_t Halfpulses_To_Do_Stepper2;
  int32_t Halfpulse_Time_Stepper1;
  int32_t Halfpulse_Time_Stepper2;
  uint32_t Last_Time_Stepper1_Update; //Microseconds
  uint32_t Last_Time_Stepper2_Update;
  int8_t Direction_Stepper1_Before;
  int8_t Direction_Stepper2_Before;
  
public:
  G1_Move(Stepper_Driver* stepper1, Stepper_Driver* stepper2, int32_t final_pos_s1_micrometers, int32_t final_pos_s2_micrometers, int32_t micrometers_per_second, uint32_t Current_Time_Microseconds){
    Stepper1 = stepper1;
    Stepper2 = stepper2;
    Halfpulses_To_Do_Stepper1 = 2*(((final_pos_s1_micrometers * Stepper1->Pulses_Per_Centimeter)/10000) - Stepper1->Current_Position_Pulses);
    Halfpulses_To_Do_Stepper2 = 2*(((final_pos_s2_micrometers * Stepper2->Pulses_Per_Centimeter)/10000) - Stepper2->Current_Position_Pulses);
    
    Direction_Stepper1_Before = Stepper1->Current_Direction;
    Direction_Stepper2_Before = Stepper2->Current_Direction;
    Stepper1->Currently_Used = true;
    Stepper2->Currently_Used = true;
    if (Halfpulses_To_Do_Stepper1 > 0){
      digitalWrite (Stepper1->Direction_Pin, HIGH);
      Stepper1->Current_Direction = 1;
    }
    else {
      digitalWrite (Stepper1->Direction_Pin, LOW);
      Stepper1->Current_Direction = -1;
      Halfpulses_To_Do_Stepper1 = abs(Halfpulses_To_Do_Stepper1);
    }
    if (Halfpulses_To_Do_Stepper2 > 0){
      digitalWrite (Stepper2->Direction_Pin, HIGH);
      Stepper2->Current_Direction = 1;
    }
    else {
      digitalWrite (Stepper2->Direction_Pin, LOW);
      Stepper2->Current_Direction = -1;
      Halfpulses_To_Do_Stepper2 = abs(Halfpulses_To_Do_Stepper2);
    }
    
    int32_t Distance_Stepper1 = final_pos_s1_micrometers - Stepper1->Get_Current_Position_Micrometers();
    int32_t Distance_Stepper2 = final_pos_s2_micrometers - Stepper2->Get_Current_Position_Micrometers();
    int32_t Pitagoras_Distance = (int32_t)sqrt(((int64_t)Distance_Stepper1*(int64_t)Distance_Stepper1) + ((int64_t)Distance_Stepper2*(int64_t)Distance_Stepper2));
    int64_t Full_Move_Time_Microsec = (int64_t)Pitagoras_Distance*1000000/(int64_t)micrometers_per_second;
    Halfpulse_Time_Stepper1 = (int32_t)(Full_Move_Time_Microsec/Halfpulses_To_Do_Stepper1);
    Halfpulse_Time_Stepper2 = (int32_t)(Full_Move_Time_Microsec/Halfpulses_To_Do_Stepper2);
    Last_Time_Stepper1_Update = Current_Time_Microseconds;
    Last_Time_Stepper2_Update = Current_Time_Microseconds;
    
  }
  bool Refresh(uint32_t Current_Time_Microseconds){
    if (Halfpulses_To_Do_Stepper1 == 0 && Halfpulses_To_Do_Stepper2 == 0){
      Stepper1->Current_Direction = Direction_Stepper1_Before;
        Stepper2->Current_Direction = Direction_Stepper2_Before;
        if (Direction_Stepper1_Before == 1) digitalWrite (Stepper1->Direction_Pin, HIGH);
        else digitalWrite (Stepper1->Direction_Pin, LOW);
        if (Direction_Stepper2_Before == 1) digitalWrite (Stepper2->Direction_Pin, HIGH);
        else digitalWrite (Stepper2->Direction_Pin, LOW);
        Stepper1->Currently_Used = false;
        Stepper2->Currently_Used = false;
        return true;
    }
    if (Halfpulses_To_Do_Stepper1>0)
      if (Current_Time_Microseconds >= Last_Time_Stepper1_Update + Halfpulse_Time_Stepper1){
        Stepper1->Flip_Pulse_Pin();
        Halfpulses_To_Do_Stepper1--;
        Last_Time_Stepper1_Update = Last_Time_Stepper1_Update + Halfpulse_Time_Stepper1;
        }
    if (Halfpulses_To_Do_Stepper2>0)
      if (Current_Time_Microseconds >= Last_Time_Stepper2_Update + Halfpulse_Time_Stepper2){
        Stepper2->Flip_Pulse_Pin();
        Halfpulses_To_Do_Stepper2--;
        Last_Time_Stepper2_Update = Last_Time_Stepper2_Update + Halfpulse_Time_Stepper2;
        }
    return false;

        
    }
};

bool Moves_Has_Finished;
#define MAX_BUFFER_POSITION 30
int32_t Moves_Buffer [MAX_BUFFER_POSITION][4];
uint8_t Moves_Buffer_Add_Counter;
uint8_t Moves_Buffer_Exe_Counter;
int32_t G1_Menu_X;
int32_t G1_Menu_Y;
int32_t G1_Menu_F;
void Set_G1_Menu_X(int32_t num){
  G1_Menu_X = num;
}
void Set_G1_Menu_Y(int32_t num){
  G1_Menu_Y = num;
}
void Set_G1_Menu_F(int32_t num){
  G1_Menu_F = num;
}

bool Add_Position_To_Buffer(int32_t X_Position, int32_t Y_Position,  int32_t Z_Position,  int32_t Feedrate){
    if (Moves_Buffer_Add_Counter + 1 == Moves_Buffer_Exe_Counter) return false;
    if ((Moves_Buffer_Add_Counter == (MAX_BUFFER_POSITION-1)) && Moves_Buffer_Exe_Counter == 0) return false;
    Moves_Buffer[Moves_Buffer_Add_Counter][0] = X_Position;
    Moves_Buffer[Moves_Buffer_Add_Counter][1] = Y_Position;
    Moves_Buffer[Moves_Buffer_Add_Counter][2] = Z_Position;
    Moves_Buffer[Moves_Buffer_Add_Counter][3] = Feedrate;
    Moves_Buffer_Add_Counter++;
    if (Moves_Buffer_Add_Counter >= MAX_BUFFER_POSITION) Moves_Buffer_Add_Counter = 0;
    return true;
}

Stepper_Driver X_Axis(STEPPER_X_PULSE, STEPPER_X_DIRECTION, Pulses_per_centimeter_X);
Stepper_Driver Y_Axis(STEPPER_Y_PULSE, STEPPER_Y_DIRECTION, Pulses_per_centimeter_Y);
uint16_t JogX_Mid;
uint16_t JogY_Mid;
int16_t Jog_Sensitivity_Micrometers;
uint8_t Jog_Sensitivity_Type;

Adafruit_GFX_Button Zero_position, Yes_Button, No_Button, Ok_Button, Units, Control_Jog, Change_X_Pulses, Change_Y_Pulses, Change_Z_Pulses, Return_Button, G1_Move_Butt;
Adafruit_GFX_Button Jog_Sens0032, Jog_Sens001, Jog_Sens032, Jog_Sens1, Jog_Sens3p2, Set_Jog_XY, Set_Jog_X, Set_Jog_Y, Set_Jog_Z;
Adafruit_GFX_Button G1_Edit_X, G1_Edit_Y, G1_Edit_F;
Adafruit_GFX_Button One_Btn, Two_Btn, Three_Btn, Four_Btn, Five_Btn, Six_Btn, Seven_Btn, Eight_Btn, Nine_Btn, Zero_Btn, Del_Btn, Min_Btn;
uint8_t Current_Menu;
bool Z_Axis_Enabled;
void Change_Menu(uint8_t);
void Refresh_Menu();


uint8_t itoa_32_t_dec(int32_t num, char* buf){  ///buf should have 12 bytes including negative sign and '\0' termination
    bool Negative = false;
    if (num<0) {
      Negative = true;
      num = -num;
    }
    int i=0;
    char Reverse[10];
    char zero_char = '0';
    for ( ; num>0; i++){
      Reverse[i] = (char)(num%10) + zero_char;
      num = num/10;
      if (num == 0) break;
    }
    int j=0;
    if (Negative == true) {
      buf[0] = '-';
      j++;
    }
    for( ; i>=0; i--){
      buf[j] = Reverse[i];
      j++;
    }
    buf[j] = '\0';
    return j;
}

char Position_X_ch[17];
char Position_Y_ch[17];
void Convert_Positions_To_Chars(){
  int32_t Position_i = X_Axis.Get_Current_Position_Micrometers();
  uint8_t last = itoa_32_t_dec(Position_i, Position_X_ch);
  int8_t Negative = 0;
  uint8_t expo;
  if (Position_i<0) {
    expo = last-1;
    Negative = 1;
  }
  else expo = last;
  int8_t add_zeros = 4 - expo;
  if (add_zeros < 0) add_zeros = 0;
  
  Position_X_ch[last+add_zeros+1] = '\0';
  Position_X_ch[last+add_zeros] = Position_X_ch[last-1];
  if (add_zeros == 3) {
    Position_X_ch[last+add_zeros-1] = '0';
    Position_X_ch[last+add_zeros-2] = '0';
    Position_X_ch[last+add_zeros-3] = '.';
    Position_X_ch[last+add_zeros-4] = '0';
  }
  else if (add_zeros == 2) {
    Position_X_ch[last+add_zeros-1] = Position_X_ch[last-2];
    Position_X_ch[last+add_zeros-2] = '0';
    Position_X_ch[last+add_zeros-3] = '.';
    Position_X_ch[last+add_zeros-4] = '0';
  }
  else if (add_zeros == 1) {
    Position_X_ch[last+add_zeros-1] = Position_X_ch[last-2];
    Position_X_ch[last+add_zeros-2] = Position_X_ch[last-3];
    Position_X_ch[last+add_zeros-3] = '.';
    Position_X_ch[last+add_zeros-4] = '0';
  }
  else {
    Position_X_ch[last+add_zeros-1] = Position_X_ch[last-2];
    Position_X_ch[last+add_zeros-2] = Position_X_ch[last-3];
    Position_X_ch[last+add_zeros-3] = '.';
  }
  
  Position_i = Y_Axis.Get_Current_Position_Micrometers();
  last = itoa_32_t_dec(Position_i, Position_Y_ch);
  Negative = 0;
  if (Position_i<0) {
    expo = last-1;
    Negative = 1;
  }
  else expo = last;
  add_zeros = 4 - expo;
  if (add_zeros < 0) add_zeros = 0;
  
  Position_Y_ch[last+add_zeros+1] = '\0';
  Position_Y_ch[last+add_zeros] = Position_Y_ch[last-1];
  if (add_zeros == 3) {
    Position_Y_ch[last+add_zeros-1] = '0';
    Position_Y_ch[last+add_zeros-2] = '0';
    Position_Y_ch[last+add_zeros-3] = '.';
    Position_Y_ch[last+add_zeros-4] = '0';
  }
  else if (add_zeros == 2) {
    Position_Y_ch[last+add_zeros-1] = Position_Y_ch[last-2];
    Position_Y_ch[last+add_zeros-2] = '0';
    Position_Y_ch[last+add_zeros-3] = '.';
    Position_Y_ch[last+add_zeros-4] = '0';
  }
  else if (add_zeros == 1) {
    Position_Y_ch[last+add_zeros-1] = Position_Y_ch[last-2];
    Position_Y_ch[last+add_zeros-2] = Position_Y_ch[last-3];
    Position_Y_ch[last+add_zeros-3] = '.';
    Position_Y_ch[last+add_zeros-4] = '0';
  }
  else {
    Position_Y_ch[last+add_zeros-1] = Position_Y_ch[last-2];
    Position_Y_ch[last+add_zeros-2] = Position_Y_ch[last-3];
    Position_Y_ch[last+add_zeros-3] = '.';
  }
}

void Set_Current_Pos_To_0(){
  X_Axis.Reset_Position_To_0();
  Y_Axis.Reset_Position_To_0();
}

int32_t Pulses_Per_100u_Z;
void Set_Pulses_X(int32_t pulses){
  if (Pulses_per_centimeter_X != pulses){
    Pulses_per_centimeter_X = pulses;
    X_Axis.Change_Pulses_Per_Centimeter_To(pulses);
    Write_To_Eeprom(EEPROM_X_PULSES, Pulses_per_centimeter_X);
  }
}
void Set_Pulses_Y(int32_t pulses){
  if (Pulses_per_centimeter_Y != pulses){
    Pulses_per_centimeter_Y = pulses;
    Y_Axis.Change_Pulses_Per_Centimeter_To(pulses);
    Write_To_Eeprom(EEPROM_Y_PULSES, Pulses_per_centimeter_Y);
  }
}
void Set_Pulses_Z(int32_t pulses){
  if (Pulses_Per_100u_Z != pulses){
    Pulses_Per_100u_Z = pulses;
 ///Z_Axis.Change_Pulses_Per_Centimeter_To(pulses);
    Write_To_Eeprom(EEPROM_Z_PULSES, Pulses_Per_100u_Z);
  }
}

int32_t Int32_Input;
bool Negative_Int32_Input;
uint8_t Jog_Setup;  /// 0-XY, 1-X, 2-Y, 3-Z
void Set_Jog_To(uint8_t jog){
  if (Jog_Setup != jog) Jog_Setup = jog;
 // if (EEPROM.read(EEPROM_JOG_TYPE) != Jog_Setup)
    EEPROM.write(EEPROM_JOG_TYPE, Jog_Setup);
}
void Set_Jog_Sensitivity(uint8_t jog_sens_type){
  if (Jog_Sensitivity_Type != jog_sens_type) {
    Jog_Sensitivity_Type = jog_sens_type;
    EEPROM.write(EEPROM_JOG_SENS, jog_sens_type);
    Set_Jog_Sensitivity();
  }
}
void Set_Jog_Sensitivity(){
  switch(Jog_Sensitivity_Type){
      case 1:
        Jog_Sensitivity_Micrometers = 32;
        break;
      case 2:
        Jog_Sensitivity_Micrometers = 100;
        break;
      case 3:
        Jog_Sensitivity_Micrometers = 320;
        break;
      case 4:
        Jog_Sensitivity_Micrometers = 1000;
        break;
      case 5:
        Jog_Sensitivity_Micrometers = 3200;
        break;
      default:
        Jog_Sensitivity_Micrometers = 500;
        break;
    }
}

void setup() {
  
  Moves_Buffer_Exe_Counter = 0;
  Moves_Buffer_Add_Counter = 0;
  G1_Menu_X = 0;
  G1_Menu_Y = 0;
  G1_Menu_F = 0;
  Moves_Has_Finished = true;
  Serial.begin(9600);
  uint16_t ID = tft.readID();
    tft.begin(ID);
    tft.setRotation(0);
    tft.fillScreen(BLACK);
    tft.setFont(NULL);
//    Position_X = Read_Eeprom4b(EEPROM_X_START_POS);
//    Position_Y = Read_Eeprom4b(EEPROM_y_START_POS);
    Z_Axis_Enabled = EEPROM.read(EEPROM_Z_ENABLED);
    Jog_Setup = EEPROM.read(EEPROM_JOG_TYPE);
    Jog_Sensitivity_Type = EEPROM.read(EEPROM_JOG_SENS);
    Set_Jog_Sensitivity();
    Pulses_per_centimeter_X = Read_Eeprom4b(EEPROM_X_PULSES);
    Pulses_per_centimeter_Y = Read_Eeprom4b(EEPROM_Y_PULSES);
    Pulses_Per_100u_Z = Read_Eeprom4b(EEPROM_Z_PULSES);
    X_Axis.Change_Pulses_Per_Centimeter_To(Pulses_per_centimeter_X);  //6400
    Y_Axis.Change_Pulses_Per_Centimeter_To(Pulses_per_centimeter_Y);
    JogX_Mid = analogRead(JOG_X_ANALOG_PIN);
    JogY_Mid = analogRead(JOG_Y_ANALOG_PIN);
    
    Zero_position.initButton(&tft,  120, 110, 220, 32, WHITE, BLACK, MAGENTA, "Set 0 Pos", 2);
    Units.initButton(&tft,  120, 147, 220, 32, WHITE, BLACK, MAGENTA, "Units", 2);
    Control_Jog.initButton(&tft,  120, 184, 220, 32, WHITE, BLACK, MAGENTA, "Jog", 2);
    G1_Move_Butt.initButton(&tft,  120, 221, 220, 32, WHITE, BLACK, MAGENTA, "G1 Move", 2);
    Yes_Button.initButton(&tft, 60, 200, 100, 40, WHITE, RED, GREEN, "Yes", 2);
    No_Button.initButton(&tft,  180, 200, 100, 40, WHITE, BLUE, GREEN, "No", 2);
    Ok_Button.initButton(&tft,  180, 250, 30, 30, WHITE, BLUE, GREEN, "Ok", 2);
    Return_Button.initButton(&tft,  180, 295, 80, 30, WHITE, BLUE, GREEN, "Return", 2);
    Change_X_Pulses.initButton(&tft,  50, 180, 70, 30, WHITE, BLUE, GREEN, "Set X", 2);
    Change_Y_Pulses.initButton(&tft,  50, 230, 70, 30, WHITE, BLUE, GREEN, "Set Y", 2);
    Change_Z_Pulses.initButton(&tft,  50, 280, 70, 30, WHITE, BLUE, GREEN, "Set Z", 2);
    

    One_Btn.initButton(&tft,  29, 60, 40, 40, WHITE, BLACK, MAGENTA, "1", 3);
    Two_Btn.initButton(&tft,  86, 60, 40, 40, WHITE, BLACK, MAGENTA, "2", 3);
    Three_Btn.initButton(&tft,  143, 60, 40, 40, WHITE, BLACK, MAGENTA, "3", 3);
    Four_Btn.initButton(&tft,  29, 110, 40, 40, WHITE, BLACK, MAGENTA, "4", 3);
    Five_Btn.initButton(&tft,  86, 110, 40, 40, WHITE, BLACK, MAGENTA, "5", 3);
    Six_Btn.initButton(&tft,  143, 110, 40, 40, WHITE, BLACK, MAGENTA, "6", 3);
    Seven_Btn.initButton(&tft,  29, 160, 40, 40, WHITE, BLACK, MAGENTA, "7", 3);
    Eight_Btn.initButton(&tft,  86, 160, 40, 40, WHITE, BLACK, MAGENTA, "8", 3);
    Nine_Btn.initButton(&tft,  143, 160, 40, 40, WHITE, BLACK, MAGENTA, "9", 3);
    Zero_Btn.initButton(&tft,  86, 210, 40, 40, WHITE, BLACK, MAGENTA, "0", 3);
    Del_Btn.initButton(&tft,  205, 60, 65, 40, WHITE, BLACK, MAGENTA, "Del", 3);
    Min_Btn.initButton(&tft,  205, 110, 40, 40, WHITE, BLACK, MAGENTA, "-", 3);

    Jog_Sens0032.initButton(&tft,  38, 40, 65, 32, WHITE, BLACK, MAGENTA, "x.032", 2);
    Jog_Sens001.initButton(&tft,  38, 95, 60, 32, WHITE, BLACK, MAGENTA, "x.01", 2);
    Jog_Sens032.initButton(&tft,  38, 150, 60, 32, WHITE, BLACK, MAGENTA, "x.32", 2);
    Jog_Sens1.initButton(&tft,  38, 205, 60, 32, WHITE, BLACK, MAGENTA, "x1", 2);
    Jog_Sens3p2.initButton(&tft,  38, 260, 60, 32, WHITE, BLACK, MAGENTA, "x3.2", 2);
    Set_Jog_XY.initButton(&tft,  180, 55, 120, 32, WHITE, BLACK, MAGENTA, "Jog XY", 2);
    Set_Jog_X.initButton(&tft,  180, 100, 120, 32, WHITE, BLACK, MAGENTA, "Jog X", 2);
    Set_Jog_Y.initButton(&tft,  180, 145, 120, 32, WHITE, BLACK, MAGENTA, "Jog Y", 2);
    Set_Jog_Z.initButton(&tft,  180, 190, 120, 32, WHITE, BLACK, MAGENTA, "Jog Z", 2);

    G1_Edit_X.initButton(&tft,  205, 50, 60, 32, WHITE, BLACK, MAGENTA, "Edit", 2);
    G1_Edit_Y.initButton(&tft,  205, 85, 60, 32, WHITE, BLACK, MAGENTA, "Edit", 2);
    G1_Edit_F.initButton(&tft,  205, 120, 60, 32, WHITE, BLACK, MAGENTA, "Edit", 2);
    Change_Menu(1);
  /*
  Add_Position_To_Buffer (10000, 0, 0, 8000);
  Add_Position_To_Buffer (10000, -10000, 0, 8000);
  Add_Position_To_Buffer (00000, -10000, 0, 8000);
  Add_Position_To_Buffer (-10000, -10000, 0, 8000);
  Add_Position_To_Buffer (-10000, 10000, 0, 8000);
  Add_Position_To_Buffer (10000, 10000, 0, 8000);
  Add_Position_To_Buffer (10000, -10000, 0, 8000);
  Add_Position_To_Buffer (-10000, -10000, 0, 8000);
  Add_Position_To_Buffer (-10000, 10000, 0, 8000);
  Add_Position_To_Buffer (10000, -10000, 0, 8000);
  Add_Position_To_Buffer (-10000, -10000, 0, 8000);
  Add_Position_To_Buffer (10000, 10000, 0, 8000);
  */
}

void Draw_Int32_Input(int32_t no, bool With_Min_Butt){
  tft.fillScreen(BLACK);
  if (With_Min_Butt){  
    if (no<0){
      no = -no;
      Negative_Int32_Input = true;
      Min_Btn.drawButton(false);
      tft.setTextColor(GREEN);
      tft.setTextSize(3);
      tft.setCursor(6, 6);
      tft.print("-");  
    }
    else {
      Negative_Int32_Input = false;
      Min_Btn.drawButton(true);
    }
  }
  else Negative_Int32_Input = false;
  Int32_Input = no;
  tft.setTextColor(GREEN);
  tft.setTextSize(3);
  tft.setCursor(120, 10);
  tft.print(Int32_Input);
  One_Btn.drawButton(true);
  Two_Btn.drawButton(true);
  Three_Btn.drawButton(true);
  Four_Btn.drawButton(true);
  Five_Btn.drawButton(true);
  Six_Btn.drawButton(true);
  Seven_Btn.drawButton(true);
  Eight_Btn.drawButton(true);
  Nine_Btn.drawButton(true);
  Zero_Btn.drawButton(true);
  Del_Btn.drawButton(true);
  Ok_Button.drawButton(true);
  Return_Button.drawButton(true);
}
void Refresh_Int32_Input(void (*Set_Int32_fun)(int32_t), uint8_t Return_Menu, bool With_Min_Butt){
  One_Btn.press(Cursor_Pressed && One_Btn.contains(Cursor_X, Cursor_Y));
  if (One_Btn.justPressed()){
    One_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    if (Int32_Input*10+1 < Int32_Input);
    else Int32_Input = Int32_Input*10+1;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    One_Btn.drawButton(true);
    delay(300);
  }
  Two_Btn.press(Cursor_Pressed && Two_Btn.contains(Cursor_X, Cursor_Y));
  if (Two_Btn.justPressed()){
    Two_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    if (Int32_Input*10+2 < Int32_Input);
    else Int32_Input = Int32_Input*10+2;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    Two_Btn.drawButton(true);
    delay(300);
  }
  Three_Btn.press(Cursor_Pressed && Three_Btn.contains(Cursor_X, Cursor_Y));
  if (Three_Btn.justPressed()){
    Three_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    if (Int32_Input*10+3 < Int32_Input);
    else Int32_Input = Int32_Input*10+3;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    Three_Btn.drawButton(true);
    delay(300);
  }
  Four_Btn.press(Cursor_Pressed && Four_Btn.contains(Cursor_X, Cursor_Y));
  if (Four_Btn.justPressed()){
    Four_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    if (Int32_Input*10+4 < Int32_Input);
    else Int32_Input = Int32_Input*10+4;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    Four_Btn.drawButton(true);
    delay(300);
  }
  Five_Btn.press(Cursor_Pressed && Five_Btn.contains(Cursor_X, Cursor_Y));
  if (Five_Btn.justPressed()){
    Five_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    if (Int32_Input*10+5 < Int32_Input);
    else Int32_Input = Int32_Input*10+5;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    Five_Btn.drawButton(true);
    delay(300);
  }
  Six_Btn.press(Cursor_Pressed && Six_Btn.contains(Cursor_X, Cursor_Y));
  if (Six_Btn.justPressed()){
    Six_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    if (Int32_Input*10+6 < Int32_Input);
    else Int32_Input = Int32_Input*10+6;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    Six_Btn.drawButton(true);
    delay(300);
  }
  Seven_Btn.press(Cursor_Pressed && Seven_Btn.contains(Cursor_X, Cursor_Y));
  if (Seven_Btn.justPressed()){
    Seven_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    if (Int32_Input*10+7 < Int32_Input);
    else Int32_Input = Int32_Input*10+7;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    Seven_Btn.drawButton(true);
    delay(300);
  }
  Eight_Btn.press(Cursor_Pressed && Eight_Btn.contains(Cursor_X, Cursor_Y));
  if (Eight_Btn.justPressed()){
    Eight_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    if (Int32_Input*10+8 < Int32_Input);
    else Int32_Input = Int32_Input*10+8;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    Eight_Btn.drawButton(true);
    delay(300);
  }
  Nine_Btn.press(Cursor_Pressed && Nine_Btn.contains(Cursor_X, Cursor_Y));
  if (Nine_Btn.justPressed()){
    Nine_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    if (Int32_Input*10+9 < Int32_Input);
    else Int32_Input = Int32_Input*10+9;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    Nine_Btn.drawButton(true);
    delay(300);
  }
  Zero_Btn.press(Cursor_Pressed && Zero_Btn.contains(Cursor_X, Cursor_Y));
  if (Zero_Btn.justPressed()){
    Zero_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    if (Int32_Input*10 < Int32_Input);
    else Int32_Input = Int32_Input*10;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    Zero_Btn.drawButton(true);
    delay(300);
  }
  Del_Btn.press(Cursor_Pressed && Del_Btn.contains(Cursor_X, Cursor_Y));
  if (Del_Btn.justPressed()){
    Del_Btn.drawButton(false);
    tft.fillRect(40, 6, tft.width()-40, 26, BLACK);
    Int32_Input = (Int32_Input - Int32_Input%10) / 10;
    tft.setTextSize(3);
    tft.setCursor(120, 10);
    tft.setTextColor(GREEN);
    tft.print(Int32_Input);
    Del_Btn.drawButton(true);
    delay(300);
  }
  if(With_Min_Butt){
    Min_Btn.press(Cursor_Pressed && Min_Btn.contains(Cursor_X, Cursor_Y));
    if (Min_Btn.justPressed()){
      Negative_Int32_Input = !Negative_Int32_Input;
      if (Negative_Int32_Input){
        Min_Btn.drawButton(false);
        tft.setTextSize(3);
        tft.setCursor(6, 6);
        tft.setTextColor(GREEN);
        tft.print("-");  
      }
      else {
        tft.fillRect(6, 6, 26, 26, BLACK);
        Min_Btn.drawButton(true);
      }
      delay(300);
    }
      
  }
  Return_Button.press(Cursor_Pressed && Return_Button.contains(Cursor_X, Cursor_Y));
  if (Return_Button.justPressed()){
    Return_Button.drawButton(false);
    Change_Menu(Return_Menu);
  }
  Ok_Button.press(Cursor_Pressed && Ok_Button.contains(Cursor_X, Cursor_Y));
  if (Ok_Button.justPressed()){
    Ok_Button.drawButton(false);
    if (Negative_Int32_Input) (*Set_Int32_fun)(-Int32_Input);
    else (*Set_Int32_fun)(Int32_Input);
    Change_Menu(Return_Menu);
  }
}

void Draw_X_Pulses_Menu(){
  Draw_Int32_Input(Pulses_per_centimeter_X, false);
}
void Refresh_X_Pulses_Menu(){
  Refresh_Int32_Input(Set_Pulses_X, 3, false);
}
void Draw_Y_Pulses_Menu(){
  Draw_Int32_Input(Pulses_per_centimeter_Y, false);
}
void Refresh_Y_Pulses_Menu(){
  Refresh_Int32_Input(Set_Pulses_Y, 3, false);
}
void Draw_Z_Pulses_Menu(){
//  Draw_Uint16_t_Input(Pulses_Per_100u_Z);
}
void Refresh_Z_Pulses_Menu(){
//  Refresh_Uint_Input(Set_Pulses_Z, 3, false);
}

void Draw_G1_X_Menu(){
  Draw_Int32_Input(G1_Menu_X, true);
}
void Refresh_G1_X_Menu(){
  Refresh_Int32_Input(Set_G1_Menu_X, 8, true);
}
void Draw_G1_Y_Menu(){
  Draw_Int32_Input(G1_Menu_Y, true);
}
void Refresh_G1_Y_Menu(){
  Refresh_Int32_Input(Set_G1_Menu_Y, 8, true);
}
void Draw_G1_F_Menu(){
  Draw_Int32_Input(G1_Menu_F, false);
}
void Refresh_G1_F_Menu(){
  Refresh_Int32_Input(Set_G1_Menu_F, 8, false);
}


void Draw_G1_Menu(){
  tft.fillScreen(BLACK);
  tft.setTextColor(YELLOW);
  tft.setTextSize(3);
  tft.setCursor(20, 15);
  tft.print("G1");
  tft.setCursor(20, 50);
  tft.print("X:");
  tft.setCursor(20, 85);
  tft.print("Y:");
  tft.setCursor(20, 120);
  tft.print("F:");
  tft.setTextColor(GREEN);
  tft.setCursor(60, 50);
  tft.print(G1_Menu_X);
  tft.setCursor(60, 85);
  tft.print(G1_Menu_Y);
  tft.setCursor(60, 120);
  tft.print(G1_Menu_F);
  G1_Edit_X.drawButton(true);
  G1_Edit_Y.drawButton(true);
  G1_Edit_F.drawButton(true);
  Return_Button.drawButton(true);
  Ok_Button.drawButton(true);
}
void Refresh_G1_Menu(){
  G1_Edit_X.press(Cursor_Pressed && G1_Edit_X.contains(Cursor_X, Cursor_Y));
  if (G1_Edit_X.justPressed()){
    G1_Edit_X.drawButton(false);
    Change_Menu(9);
  }
  G1_Edit_Y.press(Cursor_Pressed && G1_Edit_Y.contains(Cursor_X, Cursor_Y));
  if (G1_Edit_Y.justPressed()){
    G1_Edit_Y.drawButton(false);
    Change_Menu(10);
  }
  G1_Edit_F.press(Cursor_Pressed && G1_Edit_F.contains(Cursor_X, Cursor_Y));
  if (G1_Edit_F.justPressed()){
    G1_Edit_F.drawButton(false);
    Change_Menu(11);
  }
  
  Return_Button.press(Cursor_Pressed && Return_Button.contains(Cursor_X, Cursor_Y));
  if (Return_Button.justPressed()){
    Return_Button.drawButton(false);
    Change_Menu(1);
  }
  Ok_Button.press(Cursor_Pressed && Ok_Button.contains(Cursor_X, Cursor_Y));
  if (Ok_Button.justPressed()){
    Ok_Button.drawButton(false);
    Add_Position_To_Buffer(G1_Menu_X, G1_Menu_Y, 0, G1_Menu_F);
    Change_Menu(1);
  }
}

void Draw_Main_Menu(){
  tft.fillScreen(BLACK);
  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.print("X: ");
  tft.setCursor(20, 40);
  tft.print("Y: ");
  tft.setTextColor(GREEN);
  tft.setTextSize(2);
  Convert_Positions_To_Chars();
  tft.setCursor(90, 10);
  tft.print(Position_X_ch);
  tft.setCursor(90, 35);
  tft.print(Position_Y_ch);
  Zero_position.drawButton(true);
  Units.drawButton(true);
  Control_Jog.drawButton(true);
  G1_Move_Butt.drawButton(true);
}
void Refresh_Main_Menu(){
//  tft.fillRect(75, 3, tft.width()- 2*75, 55, BLACK);
  tft.setTextSize(2);
  tft.setTextColor(BLACK);
  tft.setCursor(90, 10);
  tft.print(Position_X_ch);
  tft.setCursor(90, 35);
  tft.print(Position_Y_ch);

  tft.setTextColor(GREEN);
  tft.setCursor(90, 10);
  Convert_Positions_To_Chars();
  tft.print(Position_X_ch);
  tft.setCursor(90, 35);
  tft.print(Position_Y_ch);
  Zero_position.press(Cursor_Pressed && Zero_position.contains(Cursor_X, Cursor_Y));
  if (Zero_position.justPressed()){
    Zero_position.drawButton(false);
    Change_Menu(2);
  }
  Units.press(Cursor_Pressed && Units.contains(Cursor_X, Cursor_Y));
  if (Units.justPressed()){
    Units.drawButton(false);
    Change_Menu(3);
  }
  Control_Jog.press(Cursor_Pressed && Control_Jog.contains(Cursor_X, Cursor_Y));
  if (Control_Jog.justPressed()){
    Control_Jog.drawButton(false);
    Change_Menu(4);
  }
  G1_Move_Butt.press(Cursor_Pressed && G1_Move_Butt.contains(Cursor_X, Cursor_Y));
  if (G1_Move_Butt.justPressed()){
    G1_Move_Butt.drawButton(false);
    Change_Menu(8);
  }
}

void Draw_Zero_Position_Menu(){
  tft.fillScreen(BLACK);
  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.print("Set current position to 0,0,0?");
  Yes_Button.drawButton(true);
  No_Button.drawButton(true);
}
void Refresh_Zero_Position_Menu(){
  Yes_Button.press(Cursor_Pressed && Yes_Button.contains(Cursor_X, Cursor_Y));
  if (Yes_Button.justPressed()){
    Yes_Button.drawButton(false);
    Set_Current_Pos_To_0();
    Change_Menu(1);
  }
  No_Button.press(Cursor_Pressed && No_Button.contains(Cursor_X, Cursor_Y));
  if (No_Button.justPressed()){
    No_Button.drawButton(false);
    Change_Menu(1);
  }
}

void Draw_Units_Menu(){
  tft.fillScreen(BLACK);
  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  tft.print("X axis current pulses/10mm is:");
  tft.setCursor(20, 65);
  tft.print("Y axis current pulses/10mm is:");
  if (Z_Axis_Enabled){
    tft.setCursor(20, 115);
    tft.print("Z axis current pulses/10mm is:");
  }
  
  tft.setTextColor(GREEN);
  tft.setCursor(170, 35);
  tft.print(Pulses_per_centimeter_X);
  tft.setCursor(170, 85);
  tft.print(Pulses_per_centimeter_Y);
  tft.setCursor(170, 135);
  tft.print(Pulses_Per_100u_Z);
  // EEPROM.write(EEPROM_Z_ENABLED, Z_Axis_Enabled);
//  Yes_Button.drawButton(true);
//  No_Button.drawButton(true);
  Change_X_Pulses.drawButton(true);
  Change_Y_Pulses.drawButton(true);
  Change_Z_Pulses.drawButton(true);
  Ok_Button.drawButton(true);
}
void Refresh_Units_Menu(){
/*  tft.setTextColor(BLACK);
  tft.setTextSize(2);
  tft.setCursor(170, 35);
  tft.print(Pulses_per_centimeter_X);
  tft.setCursor(170, 85);
  tft.print(Pulses_per_centimeter_Y);
  tft.setCursor(170, 135);
  tft.print(Pulses_Per_100u_Z);
  tft.setTextColor(GREEN);
  tft.setCursor(170, 35);
  tft.print(Pulses_per_centimeter_X);
  tft.setCursor(170, 85);
  tft.print(Pulses_per_centimeter_Y);
  tft.setCursor(170, 135);
  tft.print(Pulses_Per_100u_Z);
*/
  Change_X_Pulses.press(Cursor_Pressed && Change_X_Pulses.contains(Cursor_X, Cursor_Y));
  if (Change_X_Pulses.justPressed()){
    Change_X_Pulses.drawButton(false);
    Change_Menu(5);
  }
  Change_Y_Pulses.press(Cursor_Pressed && Change_Y_Pulses.contains(Cursor_X, Cursor_Y));
  if (Change_Y_Pulses.justPressed()){
    Change_Y_Pulses.drawButton(false);
    Change_Menu(6);
  }
  Change_Z_Pulses.press(Cursor_Pressed && Change_Z_Pulses.contains(Cursor_X, Cursor_Y));
  if (Change_Z_Pulses.justPressed()){
    Change_Z_Pulses.drawButton(false);
    Change_Menu(7);
  }
  Ok_Button.press(Cursor_Pressed && Ok_Button.contains(Cursor_X, Cursor_Y));
  if (Ok_Button.justPressed()){
    Ok_Button.drawButton(false);
    Change_Menu(1);
  }
  
/*  Yes_Button.press(Cursor_Pressed && Yes_Button.contains(Cursor_X, Cursor_Y));
  if (Yes_Button.justPressed()){
    Yes_Button.drawButton(false);
    Change_Menu(1);
  }
  No_Button.press(Cursor_Pressed && No_Button.contains(Cursor_X, Cursor_Y));
  if (No_Button.justPressed()){
    No_Button.drawButton(false);
    Change_Menu(1);
  }*/
}

void Draw_Control_Jog_Menu(){
  tft.fillScreen(BLACK);
  tft.setTextColor(YELLOW);
  tft.setTextSize(2);
  tft.setCursor(20, 15);
  if (Jog_Sensitivity_Type == 1) Jog_Sens0032.drawButton(false);
  else Jog_Sens0032.drawButton(true);
  if (Jog_Sensitivity_Type == 2) Jog_Sens001.drawButton(false);
  else Jog_Sens001.drawButton(true);
  if (Jog_Sensitivity_Type == 3) Jog_Sens032.drawButton(false);
  else Jog_Sens032.drawButton(true);
  if (Jog_Sensitivity_Type == 4) Jog_Sens1.drawButton(false);
  else Jog_Sens1.drawButton(true);
  if (Jog_Sensitivity_Type == 5) Jog_Sens3p2.drawButton(false);
  else Jog_Sens3p2.drawButton(true);
  if (Jog_Setup == 0) Set_Jog_XY.drawButton(false);
  else Set_Jog_XY.drawButton(true);
  if (Jog_Setup == 1) Set_Jog_X.drawButton(false);
  else Set_Jog_X.drawButton(true);
  if (Jog_Setup == 2) Set_Jog_Y.drawButton(false);
  else Set_Jog_Y.drawButton(true);
  if (Jog_Setup == 3) Set_Jog_Z.drawButton(false);
  else Set_Jog_Z.drawButton(true);
  Ok_Button.drawButton(true);
}
void Refresh_Control_Jog_Menu(){
  Jog_Sens0032.press(Cursor_Pressed && Jog_Sens0032.contains(Cursor_X, Cursor_Y));
  if (Jog_Sens0032.justPressed()){
    Jog_Sens0032.drawButton(false);
    Set_Jog_Sensitivity(1);
    Draw_Control_Jog_Menu();
  }
  Jog_Sens001.press(Cursor_Pressed && Jog_Sens001.contains(Cursor_X, Cursor_Y));
  if (Jog_Sens001.justPressed()){
    Jog_Sens001.drawButton(false);
    Set_Jog_Sensitivity(2);
    Draw_Control_Jog_Menu();
  }
  Jog_Sens032.press(Cursor_Pressed && Jog_Sens032.contains(Cursor_X, Cursor_Y));
  if (Jog_Sens032.justPressed()){
    Jog_Sens032.drawButton(false);
    Set_Jog_Sensitivity(3);
    Draw_Control_Jog_Menu();
  }
  Jog_Sens1.press(Cursor_Pressed && Jog_Sens1.contains(Cursor_X, Cursor_Y));
  if (Jog_Sens1.justPressed()){
    Jog_Sens1.drawButton(false);
    Set_Jog_Sensitivity(4);
    Draw_Control_Jog_Menu();
  }
  Jog_Sens3p2.press(Cursor_Pressed && Jog_Sens3p2.contains(Cursor_X, Cursor_Y));
  if (Jog_Sens3p2.justPressed()){
    Jog_Sens3p2.drawButton(false);
    Set_Jog_Sensitivity(5);
    Draw_Control_Jog_Menu();
  }
 
  Set_Jog_XY.press(Cursor_Pressed && Set_Jog_XY.contains(Cursor_X, Cursor_Y));
  if (Set_Jog_XY.justPressed()){
    Set_Jog_XY.drawButton(false);
    Set_Jog_To(0);
    Draw_Control_Jog_Menu();
  }
  Set_Jog_X.press(Cursor_Pressed && Set_Jog_X.contains(Cursor_X, Cursor_Y));
  if (Set_Jog_X.justPressed()){
    Set_Jog_X.drawButton(false);
    Set_Jog_To(1);
    Draw_Control_Jog_Menu();
  }
  Set_Jog_Y.press(Cursor_Pressed && Set_Jog_Y.contains(Cursor_X, Cursor_Y));
  if (Set_Jog_Y.justPressed()){
    Set_Jog_Y.drawButton(false);
    Set_Jog_To(2);
    Draw_Control_Jog_Menu();
  }
  Set_Jog_Z.press(Cursor_Pressed && Set_Jog_Z.contains(Cursor_X, Cursor_Y));
  if (Set_Jog_Z.justPressed()){
    Set_Jog_Z.drawButton(false);
    Set_Jog_To(3);
    Draw_Control_Jog_Menu();
  }
  Ok_Button.press(Cursor_Pressed && Ok_Button.contains(Cursor_X, Cursor_Y));
  if (Ok_Button.justPressed()){
    Ok_Button.drawButton(false);
    Change_Menu(1);
  }
}

void Change_Menu(uint8_t no){
    Current_Menu = no;
    switch(no){
      case 1:
        Draw_Main_Menu();
        break;
      case 2:
        Draw_Zero_Position_Menu();
        break;
      case 3:
        Draw_Units_Menu();
        break;
      case 4:
        Draw_Control_Jog_Menu();
        break;
      case 5:
        Draw_X_Pulses_Menu();
        break;
      case 6:
        Draw_Y_Pulses_Menu();
        break;
      case 7:
        Draw_Z_Pulses_Menu();
        break;
      case 8:
        Draw_G1_Menu();
        break;
      case 9:
        Draw_G1_X_Menu();
        break;
      case 10:
        Draw_G1_Y_Menu();
        break;
      case 11:
        Draw_G1_F_Menu();
        break;
    }
}
  void Refresh_Menu(){
    switch(Current_Menu){
      case 1:
        Refresh_Main_Menu();
        break;
      case 2:
        Refresh_Zero_Position_Menu();
        break;
      case 3:
        Refresh_Units_Menu();
        break;
      case 4:
        Refresh_Control_Jog_Menu();
        break;
      case 5:
        Refresh_X_Pulses_Menu();
        break;
      case 6:
        Refresh_Y_Pulses_Menu();
        break;
      case 7:
        Refresh_Z_Pulses_Menu();
        break;
      case 8:
        Refresh_G1_Menu();
        break;
      case 9:
        Refresh_G1_X_Menu();
        break;
      case 10:
        Refresh_G1_Y_Menu();
        break;
      case 11:
        Refresh_G1_F_Menu();
        break;
    }
}



void loop() {
    
    X_Axis.Reset_Position_To_0();
    Y_Axis.Reset_Position_To_0();
    int16_t SpeedX;
    int16_t SpeedY;
    uint16_t mlc = 0;
    uint32_t Current_Time_Micros = (uint32_t)(micros());
    G1_Move* Current_Move;
    
    while(1){
      Current_Time_Micros = (uint32_t)(micros());
      if (mlc % 14 == 0){
        if (Jog_Setup == 0 || Jog_Setup == 1){
          uint16_t JogX = analogRead(JOG_X_ANALOG_PIN);
          if (JogX >= JogX_Mid+10) SpeedX = map (JogX, JogX_Mid+10, 1023, 0, Jog_Sensitivity_Micrometers);
           else{
             if (JogX <= JogX_Mid-10)SpeedX = map (JogX, JogX_Mid-10, 0, 0, -Jog_Sensitivity_Micrometers);
             else SpeedX = 0;
          }
        }
        else SpeedX = 0;
        X_Axis.Set_Speed(SpeedX, Current_Time_Micros);
        
        if (Jog_Setup == 0 || Jog_Setup == 2){
          uint16_t JogY = analogRead(JOG_Y_ANALOG_PIN);
          if (JogY >= JogY_Mid+10) SpeedY = map (JogY, JogY_Mid+10, 1023, 0, -Jog_Sensitivity_Micrometers);
           else{
             if (JogY <= JogY_Mid-10)SpeedY = map (JogY, JogY_Mid-10, 0, 0, Jog_Sensitivity_Micrometers);
             else SpeedY = 0;
          }
        }
        else SpeedY = 0;
        Y_Axis.Set_Speed(SpeedY, Current_Time_Micros);
      }
      
      if (Moves_Has_Finished == false){
        if (Current_Move->Refresh(Current_Time_Micros)) {
          delete Current_Move;
          Moves_Has_Finished = true;
          if (Moves_Buffer_Add_Counter != Moves_Buffer_Exe_Counter) {
            if (Moves_Buffer_Exe_Counter >= MAX_BUFFER_POSITION - 1) Moves_Buffer_Exe_Counter = 0;
            Current_Move = new G1_Move(&X_Axis, &Y_Axis, Moves_Buffer[Moves_Buffer_Exe_Counter][0], Moves_Buffer[Moves_Buffer_Exe_Counter][1], Moves_Buffer[Moves_Buffer_Exe_Counter][3], Current_Time_Micros);
            Moves_Buffer_Exe_Counter++;
            Moves_Has_Finished = false;
          }
        }
      }
      else if (Moves_Buffer_Add_Counter != Moves_Buffer_Exe_Counter) {
            if (Moves_Buffer_Exe_Counter >= MAX_BUFFER_POSITION - 1) Moves_Buffer_Exe_Counter = 0;
            Current_Move = new G1_Move(&X_Axis, &Y_Axis, Moves_Buffer[Moves_Buffer_Exe_Counter][0], Moves_Buffer[Moves_Buffer_Exe_Counter][1], Moves_Buffer[Moves_Buffer_Exe_Counter][3], Current_Time_Micros);
            Moves_Buffer_Exe_Counter++;
            Moves_Has_Finished = false;
          }
      if (mlc==50) {
        if (Moves_Has_Finished == true && SpeedX == 0 && SpeedY == 0){
          Cursor_Pressed = Touch_getXY();
          Refresh_Menu();
        }
      } 
      X_Axis.Refresh(Current_Time_Micros);
      Y_Axis.Refresh(Current_Time_Micros);
      if(mlc++ == 1600) mlc = 0;
    }
}
