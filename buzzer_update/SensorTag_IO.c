/*******************************************************************************
  Filename:       SensorTag_IO.c
  Revised:        $Date: 2013-08-23 20:45:31 +0200 (fr, 23 aug 2013) $
  Revision:       $Revision: 35100 $

  Description:    This file contains the Sensor Tag sample application,
                  Input/Output control.

  Copyright 2014  Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED,
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE,
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com.
*******************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "gatt.h"
#include "gattservapp.h"
#include "SensorTag_IO.h"
#include "ioservice.h"

#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Queue.h> // Needed for util.h

#include "Board.h"
#include "peripheral.h"
#include "sensor.h"
#include "util.h"
#include "sensor_mpu9250.h"
#include "ext_flash.h"
#include "buzzer.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define IO_DATA_LED1            0x01
#define IO_DATA_LED2            0x02
#define IO_DATA_BUZZER          0x04
#define IO_DATA_MPU_POWER       0x08
#define IO_DATA_MIC_POWER       0x10
#define IO_DATA_FLASH_POWER     0x20

#define BUZZER_FREQUENCY1       2730
#define BUZZER_FREQUENCY2       3000
#define BUZZER_FREQUENCY3       4000

#define BUZZER_FREQUENCY        BUZZER_FREQUENCY3

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL VARIABLES
 */

/*********************************************************************
 * EXTERNAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static uint8_t ioMode;
static uint8_t ioValue;

static bool buzzerOn;

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void ioChangeCB(uint8_t newParamID);
#if 0
static void initBuzzTimer(void);
#endif /* 0 */
/*********************************************************************
 * PROFILE CALLBACKS
 */
static sensorCBs_t sensorTag_ioCBs =
{
  ioChangeCB,               // Characteristic value change callback
};


/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      SensorTagIO_init
 *
 * @brief   Initialization function for the SensorTag IO
 *
 */
void SensorTagIO_init(void)
{
  // Add service
  Io_addService();
  Io_registerAppCBs(&sensorTag_ioCBs);

  // Initialize the module state variables
  ioMode = IO_MODE_LOCAL;
  ioValue = 0;

  // Set internal state
  SensorTagIO_reset();
}


/*********************************************************************
 * @fn      SensorTagIO_processCharChangeEvt
 *
 * @brief   Process a change in the IO characteristics
 *
 * @return  none
 */
void SensorTagIO_processCharChangeEvt(uint8_t paramID)
{ 
  if( paramID == SENSOR_CONF )
  {
    
    Io_getParameter(SENSOR_CONF, &ioMode);
    if (ioMode == IO_MODE_SELFTEST)
    {
      ioValue = sensorTestResult();
      Io_setParameter(SENSOR_DATA, 1, &ioValue);
    }
    else 
    {
      // Mode change: make sure LEDs and buzzer are off
      Io_setParameter(SENSOR_DATA, 1, &ioValue);
      
      PIN_setOutputValue(hGpioPin, Board_LED1, Board_LED_OFF);
      PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_OFF);
      
      if (buzzerOn) { 
        buzzerClose();
        buzzerOn = false;
      }
    }
  } 
  else if (paramID == SENSOR_DATA)
  {
    Io_getParameter(SENSOR_DATA, &ioValue);
  }
  
  if (ioMode == IO_MODE_REMOTE)
  {
    // Control by remote client: 
    // - possible to operate the LEDs and buzzer
    // - right key functionality overridden (will not terminate connection)
    if (!!(ioValue & IO_DATA_LED1))
    {
      PIN_setOutputValue(hGpioPin, Board_LED1, Board_LED_ON);
    }
    else
    {
      PIN_setOutputValue(hGpioPin, Board_LED1, Board_LED_OFF);
    }
    
    if (!!(ioValue & IO_DATA_LED2))
    {
      PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_ON);
    }
    else
    {
      PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_OFF);
    }
    
    if (!!((ioValue & IO_DATA_BUZZER)))
    {
      if (!buzzerOn) {
        buzzerOpen(hGpioPin);
        buzzerSetFrequency(BUZZER_FREQUENCY);
        buzzerOn = true;
      }
    }
    else
    {
      if (buzzerOn) {
        buzzerClose();
        buzzerOn = false;
      }
    }
  }
}


/*********************************************************************
 * @fn      SensorTagIO_reset
 *
 * @brief   Reset characteristics
 *
 * @param   none
 *
 * @return  none
 */
void SensorTagIO_reset(void)
{
  ioValue = sensorTestResult();
  Io_setParameter( SENSOR_DATA, 1, &ioValue);

  ioMode = IO_MODE_LOCAL;
  Io_setParameter( SENSOR_CONF, 1, &ioMode);
  
  // Normal mode; make sure LEDs and buzzer are off
  PIN_setOutputValue(hGpioPin, Board_LED1, Board_LED_OFF);
  PIN_setOutputValue(hGpioPin, Board_LED2, Board_LED_OFF);
  
  if (buzzerOn) {
    buzzerClose();
    buzzerOn = false;
  }
}


/*********************************************************************
 * @fn      SensorTagIO_GetIoMode
 *
 * @brief   Get the current IO mode
 *
 */
uint8_t SensorTagIO_GetMode( void )
{
  return ioMode;
}


/*********************************************************************
* Private functions
*/

/*********************************************************************
 * @fn      ioChangeCB
 *
 * @brief   Callback from IO service indicating a value change
 *
 * @param   paramID - parameter ID of the value that was changed.
 *
 * @return  none
 */
static void ioChangeCB( uint8_t paramID )
{
  // Wake up the application thread
  SensorTag_charValueChangeCB(SERVICE_ID_IO, paramID);
}

/*********************************************************************
*********************************************************************/
