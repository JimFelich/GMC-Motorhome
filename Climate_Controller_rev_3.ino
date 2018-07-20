
// Climate Controller Program  May 2018
/*
USE_GITHUB_USERNAME=JimFelich
MAKE_PRIVATE_ON_GITHUB
 */

#include <genieArduino.h>
#include <Wire.h>
#include <Adafruit_MCP9808.h>
#include <Servo.h> // Servo - Version: 1.1.2

// Object Instantiations
Servo ACservo;
Genie genie;
Adafruit_MCP9808 TempIn  = Adafruit_MCP9808();
Adafruit_MCP9808 TempOut = Adafruit_MCP9808();
Adafruit_MCP9808 TempAC  = Adafruit_MCP9808();

// Display
const int Dreset = 39; // Display Reset I/O pin

// Heater Water Valve
const int VpotP  = A2; // Analog input pin that the potentiometer is attached to
const int Vmotor =  2; // Analog output pin that the motor driver is attached to
const int VdirP  = 41; // Pin for motor direction
int Vdir         =  0; // Vdir = 1 Opens the valve, Vdir = 0 Closes the valve

//Heater Blower
const int Blow  = 49; // Pin for Low Speed;
const int Bmed  = 47; // Pin for Medium Speed;
const int Bhigh = 45; // Pin for High Speed;

// Recirculation System
int Gdir    = 43; // Pin for Recirculation Gate Motor Driver
int Gopen   = 18; // Pin for sensing the Gate open Switch
int Gclosed = 19; // Pin for sensing the Gate Closed Switch
int Gon     =  3; // HIGH means Move, LOW means Stop
int GCopen        =  0; // Current Value read from GopenP Switch   
int GPopen        =  0; // Previous value read from GopenP Switch   
int GCclosed      =  0; // Current read from GclosedP Switch   
int GPclosed      =  0; // Previous value read from GclosedP Switch 
int GCdir         =  0; // Current Direction of the Gate (0 = opening, 1 = closing)

// AC Blower
//const int ACmotor =  4; // Pin for AC Motor Driver PWM
const int ACon    = 44; // Pin to Enable AC Motor Driver
const int Clutch  = 42; // Pin to Energize the AC Clutch

// Defroster Gate
const int Trigger = 48; // Defroster Trigger Pin

// Buttons
int Bauto   = 0; // Climate Control Button
int Brecirc = 0; // B recirc Closes the Recirculation Gate (Linked with Bfresh)
int Bfresh  = 1; // Bfresh Opens the Recirculation Gate (Linked with Brecirc)
int Bdef    = 0; // HIGH Sets Defroster Gate On  
int Bmax    = 0; //Overrides the Climate Controller and sets the AC Blower to Bhigh

// Climate Control Vars
int Tdesired = 0; // Desired Temperature 
int Tinside  = 0; // Current Inside Temperature
int Toutside = 0; // Current Outside Temperature
int automagic= 0; // 0 = Manual, 1 = Automatic
    
// Event Handler
int max_val;
int AC_val; 

void myGenieEventHandler();  // Define prototype of myGenieEventHandler()

//*** BEGIN SETUP LOOP *****************************************
void setup()
{
// Initalize Communications
  Wire.begin(); // Initalize I2C communications
  Serial.begin(9600);   // Initialize Serial Communications for Terminal Monitor
  Serial2.begin(9600);  // Initialize Serial2 Communications for Main Computer Interface
  Serial3.begin(115200);// Initialize Serial Communications for Display
  genie.Begin(Serial3); // Use Serial3 for talking to the Genie Library, and to the 4D Systems display
  genie.AttachEventHandler(myGenieEventHandler); // Attach the user function Event Handler for processing events
  
// Initalize Defroster
  pinMode(Trigger, OUTPUT);
  digitalWrite(Trigger, HIGH);  // HIGH = Direct hot Air to Heater Ducts  LOW = Direct hot Air to Defroster

// Initalize AC System
  ACservo.attach(4,500,2500);
  ACservo.write(0);  // set servo to low point
  pinMode(ACon, OUTPUT);
  digitalWrite(ACon, HIGH);// AC is ON when LOW and OFF when HIGH
  
// Initalize Heater Blower
  pinMode(Blow, OUTPUT);
  digitalWrite(Blow, HIGH);

  pinMode(Bmed, OUTPUT);
  digitalWrite(Bmed, HIGH);
  
  pinMode(Bhigh, OUTPUT);
  digitalWrite(Bhigh, HIGH);

// Initalize Recirculation System
  pinMode(Gdir, OUTPUT);
  digitalWrite(Gdir, LOW);

  pinMode(Gon, OUTPUT);
  digitalWrite(Gon, LOW);
  Serial.println ( "HELLO"); //Write a string to the Terminal to show we are alive
  pinMode(Gopen, INPUT_PULLUP);
  pinMode(Gclosed, INPUT_PULLUP);
  Serial.println ( "I AM ALIVE"); //Write a string to the Terminal to show we are alive
  
//Initalize Valve
  pinMode(VdirP, OUTPUT);
  digitalWrite(VdirP, Vdir);

  pinMode(Vmotor, OUTPUT);
  digitalWrite(Vmotor, LOW);

// Initalize All Temperature Sensors
  if (!TempIn.begin(0x19)) Serial.println("Couldn't find TempIn");
  if (!TempOut.begin(0x1A)) Serial.println("Couldn't find TempOut");
  if (!TempAC.begin(0x18)) Serial.println("Couldn't find TempAC");

// Initalize the Display 
  pinMode(Dreset, OUTPUT);  // Set 39 to Output
  digitalWrite(Dreset, 0);  // Reset the Display
  delay(100);
  digitalWrite(Dreset,1);  // unReset the Display
  delay (3500); //let the display start up after the reset (This is important)
  //genie.WriteContrast(15); // 0 = Display OFF, though to 15 = Max Brightness ON.

}
//*** END SETUP LOOP *****************************************


static long waitPeriod = millis();
//*** BEGIN MAIN LOOP *********************************
void loop()
{
  genie.DoEvents(); // This calls the library each loop to process the queued responses from the display

// Update the Thermometers every 5 seconds or so
  if (millis() >= waitPeriod)
    {
      // Read and print out the temperature, then convert to *F
      float c1 = TempIn.readTempC();
      float f1 = c1 * 9.0 / 5.0 + 32;
      genie.WriteObject(GENIE_OBJ_THERMOMETER, 1, f1 - 50);
      float c2 = TempOut.readTempC();
      float f2 = c2 * 9.0 / 5.0 + 32;
      genie.WriteObject(GENIE_OBJ_THERMOMETER, 0, f2 - 20);
      waitPeriod = millis() + 5000; // rerun this code to update Temperature in another 5s time.
      Serial.println ( "Thermometers Updated");
    }
  
 // Check if we need to shut off the Recirculation Motor 
  int RecircMotor = digitalRead(Gon);
  GCdir = digitalRead(Gdir);

  if(RecircMotor == HIGH)
  {
    if (digitalRead(Gopen) == LOW &&  GCdir == LOW )
    {
      digitalWrite(Gon, LOW);
    }
  }
  
  if(RecircMotor == HIGH)
  {  
    if (digitalRead(Gclosed) == LOW && GCdir == HIGH )
    {
      digitalWrite(Gon, LOW);
    }
  }
// End of check if we need to shut off the Recirculation Motor 

if(automagic == 1)  //Do stuff to maintain the climate automaticly
{
  
}



}
//***END OF MAIN LOOP *****************************

void RecircOpen(int val)
{
  digitalWrite(Gdir, HIGH);  //Open the Gate
  digitalWrite(Gon, HIGH);
}

void RecircClose(int val)
{
  digitalWrite(Gdir, LOW);  //Close the Gate
  digitalWrite(Gon, HIGH);
}

void DesiredTemp(int tempval)
{
int tempval1 = ((tempval* 15)/100)+65;
Serial.print("DESIRED TEMP ");
Serial.println(tempval1);  
}

void HeaterBlower(int heatval)
{
  Serial.print("HEATER ");
  Serial.println(heatval); 
  switch(heatval)
  {
    case 0: // OFF
      digitalWrite(Blow, HIGH); // Low Speed OFF = HIGH
      digitalWrite(Bmed, HIGH);      // Medium Speed OFF = HIGH
      digitalWrite(Bhigh, HIGH);      // High Speed OFF = HIGH
      break;
    case 1:  // LOW
      digitalWrite(Blow, LOW); // Low Speed OFF = HIGH
      digitalWrite(Bmed, HIGH);       // Medium Speed OFF = HIGH
      digitalWrite(Bhigh, HIGH);      // High Speed OFF = HIGH
      break;
    case 2:  // MED
      digitalWrite(Blow, HIGH); // Low Speed OFF = HIGH
      digitalWrite(Bmed, LOW);       // Medium Speed OFF = HIGH
      digitalWrite(Bhigh, HIGH);      // High Speed OFF = HIGH
      break;
    case 3:  // HIGH
      digitalWrite(Blow, HIGH); // Low Speed OFF = HIGH
      digitalWrite(Bmed, HIGH);       // Medium Speed OFF = HIGH
      digitalWrite(Bhigh, LOW);      // High Speed OFF = HIGH
      break;    
  }
}

void ACBlower(int ACval)
{
  Serial.print("A/C ");
  Serial.println(ACval);
  switch(ACval)
  {
    case 0:  // Turn AC OFF
      ACservo.write(0);            // Move ACservo to 0
      digitalWrite(ACon, HIGH);    // Pin to Enable AC Motor Driver. LOW is ON
      digitalWrite(Clutch, HIGH);  // Pin to Energize the AC Clutch  
      break;
    case 1:  // Turn AC LOW
      ACservo.write(90);           // Move ACservo to 90                           1
      digitalWrite(ACon, LOW);     // Pin to Enable AC Motor Driver. LOW is ON
      digitalWrite(Clutch, LOW);   // Pin to Energize the AC Clutch    
      break;
    case 2:  // Turn AC MED
      ACservo.write(120);           // Move ACservo to 120
      digitalWrite(ACon, LOW);     // Pin to Enable AC Motor Driver. LOW is ON
      digitalWrite(Clutch, LOW);   // Pin to Energize the AC Clutch      
      break;
    case 3:  // Turn AC HIGH
      ACservo.write(150);          // Move ACservo to 150
      digitalWrite(ACon, LOW);     // Pin to Enable AC Motor Driver. LOW is ON
      digitalWrite(Clutch, LOW);   // Pin to Energize the AC Clutch      
      break;
    case 4:  // Turn AC MAX
      ACservo.write(180);          // Move ACservo to 180
      digitalWrite(ACon, LOW);     // Pin to Enable AC Motor Driver. LOW is ON
      digitalWrite(Clutch, LOW);   // Pin to Energize the AC Clutch      
      break;
  }
}

void Auto(int autoval)
{
  Serial.print("AUTO ");
  Serial.println(autoval);
  switch(autoval)
  {
    case 0: // Manual
      automagic = 0;
      break;
    case 1:  // Automatic
      automagic = 1;
      break;    
  }
}

void Defrost(int defrostval)
{
  digitalWrite(Trigger, !defrostval);  // HIGH = Direct hot Air to Heater Ducts  LOW = Direct hot Air to Defroster
  if (defrostval == LOW) Serial.println("HEATER "); 
  if (defrostval == HIGH) Serial.println("DEFROSTER ");
}  

void MaxAC(int maxval)
{
Serial.print("MAX A/C ");
Serial.println(maxval);
  switch(maxval)
  {
    case 0: // Not MAX
      ACBlower(AC_val);
      genie.WriteObject(GENIE_OBJ_TRACKBAR, 1, AC_val);
      break;
    case 1:  // MAX
      ACBlower(4);
      genie.WriteObject(GENIE_OBJ_TRACKBAR, 1, 4);
      break;    
  }  
}

void MoveValve(int knob_val) // this stuff is for the water valve
{
  int Vspeed = 255;        // motor speed value output to the PWM (analog out
  // Vdir  1 = opening   0 =m closing
  int SensorVal = analogRead(VpotP);   // read the analog in value:
  int TargetVal = (8.5 * knob_val) + 60 ;

  Serial.print("  Sensor ");
  Serial.print (SensorVal);
  Serial.print("  Target ");
  Serial.print(TargetVal) ;
  Serial.print("  Diff ");
  Serial.println(SensorVal - TargetVal);
 
  if (SensorVal - TargetVal  < 0)
    {
      Vdir = 0;
      digitalWrite(VdirP, Vdir);
      analogWrite(Vmotor, Vspeed);
      while (SensorVal - TargetVal < 0)
        {
          SensorVal = analogRead(VpotP);
        }
      analogWrite(Vmotor, 0); // stop moving the valve
    }

  if (SensorVal - TargetVal > 0 )
    {
      Vdir = 1;
      digitalWrite(VdirP, Vdir);
      analogWrite(Vmotor, Vspeed);
      while (SensorVal - TargetVal > 0)
        {
          SensorVal = analogRead(VpotP);
        }
      analogWrite(Vmotor, 0);  // stop moving the valve
    }

  Serial.print(" Diff ");
  Serial.println(SensorVal - TargetVal);
  Serial.println("Done");
}


void myGenieEventHandler()
{
  genieFrame Event;
  genie.DequeueEvent(&Event); // Remove the next queued event from the buffer, and process it below

  //If the cmd received is from a Reported Event (Events triggered from the Events tab of Workshop4 objects)
  if (Event.reportObject.cmd == GENIE_REPORT_EVENT);
  {
    if (Event.reportObject.object == GENIE_OBJ_SLIDER  &&  Event.reportObject.index == 0)  // If the Reported Message was from a Slider
    {
      int slider_val = genie.GetEventData(&Event);                      // Receive the event data from the Slider0
      DesiredTemp(slider_val);       // Write Slider0 value to to LED Digits 0
    }

    if (Event.reportObject.object == GENIE_OBJ_KNOB  &&  Event.reportObject.index == 0)  // If the Reported Message was from a Knob
    {
      int knob_val = genie.GetEventData(&Event);                      // Receive the event data from the Knob
      MoveValve(knob_val);  // Move the Valve
    }
    
    if (Event.reportObject.object == GENIE_OBJ_TRACKBAR &&  Event.reportObject.index == 0)  // If the Reported Message was from a Trackbar
    {
      int heat_val = genie.GetEventData(&Event);                      // Receive the event data from HEATER Trackbar
      HeaterBlower(heat_val);
    }

    if (Event.reportObject.object == GENIE_OBJ_TRACKBAR &&  Event.reportObject.index == 1)  // If the Reported Message was from a Trackbar
    {
      AC_val = genie.GetEventData(&Event);                      // Receive the event data from A/C Trackbar
      ACBlower(AC_val);
    }

    if (Event.reportObject.object == GENIE_OBJ_USERBUTTON)
    {

      if (Event.reportObject.object == GENIE_OBJ_USERBUTTON  &&  Event.reportObject.index == 1)  // If the Reported Message was from an Button
      {
        int fresh_val = genie.GetEventData(&Event);                      // Receive the event data from FRESH
        RecircOpen(fresh_val);
      }
    
      if (Event.reportObject.object == GENIE_OBJ_USERBUTTON  &&  Event.reportObject.index == 2)  // If the Reported Message was from a Button
      {
        int auto_val = genie.GetEventData(&Event);                      // Receive the event data from AUTO
        Auto(auto_val);
      }
    
      if (Event.reportObject.object == GENIE_OBJ_USERBUTTON  &&  Event.reportObject.index == 0)  // If the Reported Message was from a Button
      {
        int recirc_val = genie.GetEventData(&Event);                      // Receive the event data from RECIRC
        RecircClose( recirc_val);
      }
    
      if (Event.reportObject.object == GENIE_OBJ_USERBUTTON  &&  Event.reportObject.index == 3)  // If the Reported Message was from a Button
      {
        int defrost_val = genie.GetEventData(&Event);                      // Receive the event data from DEFROST
        Defrost(defrost_val);
      }
    
      if (Event.reportObject.object == GENIE_OBJ_USERBUTTON  &&  Event.reportObject.index == 4)  // If the Reported Message was from a Button
      {
        max_val = genie.GetEventData(&Event);                      // Receive the event data from MAX A/C
        MaxAC(max_val);
      }
    }
  }

  //If the cmd received is from a Reported Object, which occurs if a Read Object (genie.ReadOject) is requested in the main code, reply processed here.
  if (Event.reportObject.cmd == GENIE_REPORT_OBJ)
  {
    if (Event.reportObject.object == GENIE_OBJ_USER_LED)              // If the Reported Message was from a User LED
    {
      if (Event.reportObject.index == 0)                              // If UserLed0 (Index = 0)
      {
        bool UserLed0_val = genie.GetEventData(&Event);               // Receive the event data from the UserLed0
        UserLed0_val = !UserLed0_val;                                 // Toggle the state of the User LED Variable
        genie.WriteObject(GENIE_OBJ_USER_LED, 0, UserLed0_val);       // Write UserLed0_val value back to to UserLed0
      }
    }
  }
}
//   *************************************************************************************************
//  Event.reportObject.cmd is used to determine the command of that event, such as an reported event
//  Event.reportObject.object is used to determine the object type, such as a Slider
//  Event.reportObject.index is used to determine the index of the object, such as Slider0
//  genie.GetEventData(&Event) us used to save the data from the Event, into a variable.
//  *************************************************************************************************/
