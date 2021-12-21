//=========================================================================================================
// misc_small.h - This file contains implementations for various small classes
//=========================================================================================================
#include "globals.h"

//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><
//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><
//                                   USB CONNECTION SENSOR
//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><
//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><



//=========================================================================================================
// execute() - State machine used to sense and report changes of state in the USB connection
//=========================================================================================================
void CUSBSensor::execute()
{
    // Find out of the USB is currently plugged in
    unsigned char is_connected = (analogRead(VBUS_SENSE) > 400) ? 1:0;    

    // If we don't have hardware, we're always in USB mode
    if (NO_HW) is_connected = 1;

    // If this state is different than the last sensed state...
    if (is_connected != m_state)
    {
        m_state = is_connected;
        on_change_of_state();        
    }
}
//=========================================================================================================


//=========================================================================================================
// on_change_of_state() - Called anytime the state of the USB connection changes
//=========================================================================================================
void CUSBSensor::on_change_of_state()
{
    if (m_state)
        SleepMgr.start_awake_mode();
    else
        SleepMgr.start_sleep_mode();
}
//=========================================================================================================



//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><
//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><
//      
//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><
//<><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><><
