// BORC firmware
// Updated 04/19/2021

// Developed by AKstudios

// =================================================================
// Libraries
// =================================================================
#include <RFM69.h>                    //https://www.github.com/lowpowerlab/rfm69
#include <SPIFlash.h>                 //https://github.com/LowPowerLab/SPIFlash          
#include <Adafruit_GFX.h>             //https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SHT31.h>           //https://github.com/adafruit/Adafruit_SHT31
#include <Adafruit_INA219.h>          //https://github.com/adafruit/Adafruit_INA219
#include <Adafruit_IS31FL3731.h>      //https://github.com/adafruit/Adafruit_IS31FL3731
#include <Adafruit_PWMServoDriver.h>  //https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include "config.h"

// =================================================================
// Library objects
// =================================================================
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(SERVO_DRIVER_ADDRESS);
Adafruit_IS31FL3731_Wing ledmatrix = Adafruit_IS31FL3731_Wing();
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_INA219 ina219(CURRENT_SENSE_ADDRESS);
SPIFlash flash(FLASH_SS, 0xEF30);
RFM69 radio;

// =================================================================
// Watchdog Interrupt Service Routine (ISR)
// =================================================================
ISR (WDT_vect)
{
  wdt_disable();  // disable watchdog timer
  if(WDTflag == false)
    WDTflag = true;
  if(servoWDTflag == true)
    systemReset();
}

// =================================================================
// Rotary Encoder Interrupt Service Routine (ISR)
// =================================================================
void encoderISR()  // Interrupt Service Routine for encoder
{
  PCICR &= ~(1<<PCIE1); // disable interrupts on PCINT[15:8]
  knobCounter++;
  disableKnobInterrupts();
  if (knobFlag == true)
  {
    current_A_state = digitalRead(CHANNEL_A);
    if (current_A_state != last_A_state  && current_A_state == 1)
    {
      if (digitalRead(CHANNEL_B) != current_A_state)
      {
        setpoint--;
        menuOption--;
        knobDirection = 1;   // knob moves anticlockwise
      }
      else
      {
        setpoint++;
        menuOption++;
        knobDirection = 2;   // knob moves clockwise
      }
      
      if (setpoint > setpointMaxValue)
      {
        setpoint = setpointMaxValue;
        displayMax = true;
      }
      else if (setpoint < setpointMinValue)
      {
        setpoint = setpointMinValue;
        displayMin = true;
      }

      if (menuOption > menuOptionMaxValue)
      {
        menuOption = menuOptionMaxValue;
      }
      else if (menuOption < 0)
      {
        menuOption = 0;
      }

    }
    manualOverride = 1;
    last_A_state = current_A_state;
  }
  else if (knobFlag == false)
  {
    knobFlag = true;
  }
  
  enableKnobInterrupts();
}

// =================================================================
// Knob click Interrupt Service Routine (ISR)
// =================================================================
ISR(PCINT1_vect)  // ISR for knob click pin
{
  knobCounter = 10;
  knobFlag = true;
  PCICR &= ~(1<<PCIE1); // disable interrupts on PCINT[15:8]
  if(knobClickFlag == false)
    knobClickFlag = true;
}

// =================================================================
// Setup function
// =================================================================
void setup()
{
  // disable watchdog timer just in case
  MCUSR = 0;
  wdt_disable();  

  // turn LEDs off
  toggleLED(0,0,0);

  // disable all devices on power up
  controlDevices(99, LOW);
  
  Serial.begin(SERIAL_BAUD);
  Serial.println("Power on");
  
  fadeLED(BLUE);    // this indicates a power up

  // check device configuration status
  EEPROM.get(CONFIGFLAGADDRESS, configFlag);
  if (configFlag == true) // if device not configured, use default values
  {
    Serial.println("Device configured, using EEPROM variables to init radio");
    getNodeParams(); // this will update default variables using data stored in EEPROM
  }
  else if (configFlag == false)
  {
    Serial.println("Device not configured, using default variables to init radio");
  }

  Serial.println(NODEID);
  Serial.println(NETWORKID);
  Serial.println(ENCRYPTKEY);

  // initialize radio
  initializeRadio();
  
  // initialize flash
  if(!flash.initialize())
    errorCode |= (1<<FLASH_ERR);
  else
    errorCode &= ~(1<<FLASH_ERR);
  
  // get UID from flash
  uint8_t* MACID = flash.readUniqueId();  // get unique 64-bit ID from flash
  for (int i=0; i<8; i++)
    _MACID +=  String(MACID[i], HEX); // convert uint8_t* to HEX String
  _MACID.toCharArray(UID, _MACID.length()+1); // convert String to char array to use in datapackets
  Serial.println(UID);

  // enable all hardware devices
  controlDevices(99, HIGH);
  delay(10);
  
  // enable servo driver and set servo to min
  enableServo();
  if (SERVO_TYPE == 0)  // 270 servo
    pwm.setPWM(0, 0, servoMinPosition); // start from min position for 270 servo
  
  // initialize LED matrix
  initializeLEDmatrix();

  // enable current sensor
  currentSense();

  // enable temp/RH sensor
  readTempRH();

  // Read the initial state of A
  last_A_state = digitalRead(CHANNEL_A);

  // setup interrupts
  enableKnobInterrupts();

  // get initial time
  last_time = millis();
  
  Serial.println("Ready");
  
  delay(1000); //wait 1 second till everything settles in
}

// =================================================================
// Main loop
// =================================================================
void loop()
{
  // device not configured -----------------------------------------
  if (configFlag == false)
  {
    configurationMode();  // go back to config mode if knob clicked once
    sleep('f'); // sleep forever
    if(knobFlag == true && knobClickFlag == true)
    {
      last_time = millis();
      while(digitalRead(KNOB_CLICK) == 0) // button still pressed
      {
        current_time = millis();
        
        // enable demo mode when pressed and held for 1.5 secs
        if(current_time - last_time >= 1500 && current_time - last_time <= 4000)
        {
          Serial.println("now in demo mode");
          configFlag = true;  // temporarily set configFlag to true. Actual value still in EEPROM, resets on reboot
          toggleLED(1,1,1); // toggle white LED to indicate temporary demo mode active
          delay(100);
          break;
        }

        if(current_time - last_time >= 4000)  // timeout
        {
          Serial.println("timeout of button press");
          break;
        }

        // if knob is no longer being held, exit
        if(digitalRead(KNOB_CLICK) == 1)
        {
          break;
        }
      }
    }
    delay(1000);
  }

  // device already configured -------------------------------------
  else if(configFlag == true)
  {
    last_time = millis(); // keep the last millis time in memory
    wdt_reset();
  
    // Go to sleep ---------------------------------------------------
    if(knobFlag == false && knobClickFlag == false && WDTflag == false && knobDirection == 0)
    {
      last_time = millis(); // save last millis time
      Serial.println("00 sleep");
      delay(5);
      debug();
      sleep('w'); // sleep every loop
    }
  
    // WDT triggered, knob untouched ---------------------------------
    else if(knobFlag == false && knobClickFlag == false && WDTflag == true && knobDirection == 0)
    {
      Serial.print("actions counter: ");
      Serial.println(actionsIntervalCounter);
      Serial.print("transmit counter: ");
      Serial.println(transmitIntervalCounter);
      if(actionsIntervalCounter == 3) // if enough time has passed, do actions (3 intervals = ~32 seconds),
      {
        readTempRH();
        Serial.print("Temp: ");
        Serial.println(temp);
        Serial.print("RH: ");
        Serial.println(rh);
        // do automatic servo control here
        actionsIntervalCounter = 0; // reset watchdog counter to 0
      }
      else if(actionsIntervalCounter < 3)
      {
        actionsIntervalCounter++;
      }
      
      if(transmitIntervalCounter == (transmitInterval/8)-1) // send data every ~56 secs
      {
        Serial.println("sending data");
        createDataPacket('s');  // create a configuration packet
        sendData(true);   // true argument waits for ACK to come back
        transmitIntervalCounter = 0;
      }
      else if(transmitIntervalCounter < (transmitInterval/8)-1)
      {
        transmitIntervalCounter++;
      }
      last_time = millis(); // save last millis time
      WDTflag = false;
    }
  
    // knob interacted with ------------------------------------------
    else
      checkKnobStatus();
  }
}

// =================================================================
