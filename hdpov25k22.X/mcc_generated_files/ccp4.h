/**
  CCP4 Generated Driver API Header File

  @Company
    Microchip Technology Inc.

  @File Name
    ccp4.h

  @Summary
    This is the generated header file for the CCP4 driver using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  @Description
    This header file provides APIs for driver for CCP4.
    Generation Information :
        Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.65
        Device            :  PIC18F25K22
        Driver Version    :  2.00
    The generated drivers are tested against the following:
        Compiler          :  XC8 1.45
        MPLAB 	          :  MPLAB X 4.10
*/

/*
    (c) 2016 Microchip Technology Inc. and its subsidiaries. You may use this
    software and any derivatives exclusively with Microchip products.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE, OR ITS INTERACTION WITH MICROCHIP PRODUCTS, COMBINATION
    WITH ANY OTHER PRODUCTS, OR USE IN ANY APPLICATION.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.

    MICROCHIP PROVIDES THIS SOFTWARE CONDITIONALLY UPON YOUR ACCEPTANCE OF THESE
    TERMS.
*/
 
#ifndef _CCP4_H
#define _CCP4_H

/**
  Section: Included Files
*/

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus  // Provide C++ Compatibility

    extern "C" {

#endif

/** 
   Section: Data Type Definition
*/
        
/**
 @Summary
   Defines the values to convert from 16bit to two 8 bit and vice versa

 @Description
   This routine used to get two 8 bit values from 16bit also
   two 8 bit value are combine to get 16bit.

 Remarks:
   None
 */

typedef union CCPR4Reg_tag
{
   struct
   {
      uint8_t ccpr4l;
      uint8_t ccpr4h;
   };
   struct
   {
      uint16_t ccpr4_16Bit;
   };
} CCP_PERIOD_REG_T ;

/**
  Section: Capture Module APIs
*/

/**
  @Summary
    Initializes the CCP4

  @Description
    This routine initializes the CCP4_Initialize.
    This routine must be called before any other CCP4 routine is called.
    This routine should only be called once during system initialization.

  @Preconditions
    None

  @Param
    None

  @Returns
    None

  @Comment
    

 @Example
    <code>
    CCP4_Initialize();
    </code>
 */
void CCP4_Initialize(void);

/**
  @Summary
    Determines the completion of the data captured.

  @Description
    This routine is used to determine if data capture is completed.
    When data capture is complete routine returns 1. It returns 0 otherwise.

  @Preconditions
    CCP4_Initialize()function should have been called before calling this function.

  @Returns
    Returns 1 if data capture is completed
    true - Indicates data capture complete
    false - Indicates data capture is not complete

  @Param
    None

  @Example
    <code>
    CCP4_Initialize();
    while(!CCP4_IsCapturedDataReady());
    </code>
*/
bool CCP4_IsCapturedDataReady(void);

/**
  @Summary
    Reads the 16 bit capture value.

  @Description
    This routine reads the 16 bit capture value.

  @Preconditions
    CCP4_Initialize() and
    CCP4_IsCapturedDataReady() function should have been
    called before calling this function.

  @Param
    None.

  @Returns
    Returns 16 bit captured value

  @Example
    <code>
    uint16_t capture;

    CCP4_Initialize();

    while(!CCP4_IsCapturedDataReady());   //Used only for polling method
    capture = CCP4_CaptureRead();
    </code>
*/
uint16_t CCP4_CaptureRead(void);

#ifdef __cplusplus  // Provide C++ Compatibility

    }

#endif

#endif  //_CCP4_H
/**
 End of File
*/

