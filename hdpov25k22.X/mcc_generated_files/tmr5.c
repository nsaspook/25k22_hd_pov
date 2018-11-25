/**
  TMR5 Generated Driver File

  @Company
    Microchip Technology Inc.

  @File Name
    tmr5.c

  @Summary
    This is the generated driver implementation file for the TMR5 driver using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  @Description
    This source file provides APIs for TMR5.
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

/**
  Section: Included Files
 */

#include <xc.h>
#include "tmr5.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../pov_mon.h"

/**
  Section: Global Variable Definitions
 */
volatile uint16_t timer5ReloadVal;
void (*TMR5_InterruptHandler)(void);
extern struct V_data V;

/**
  Section: TMR5 APIs
 */

void TMR5_Initialize(void)
{
	//Set the Timer to the options selected in the GUI

	//T5GSS T5G_pin; TMR5GE disabled; T5GTM disabled; T5GPOL low; T5GGO done; T5GSPM disabled; 
	T5GCON = 0x00;

    //TMR5H 125; 
    TMR5H = 0x7D;

    //TMR5L 136; 
    TMR5L = 0x88;

	// Load the TMR value to reload variable
    timer5ReloadVal=TMR5;

	// Clearing IF flag before enabling the interrupt.
	PIR5bits.TMR5IF = 0;

	// Enabling TMR5 interrupt.
	PIE5bits.TMR5IE = 1;

	// Set Default Interrupt Handler
	TMR5_SetInterruptHandler(TMR5_DefaultInterruptHandler);

	// T5CKPS 1:4; T5SOSCEN disabled; T5SYNC synchronize; TMR5CS FOSC/4; TMR5ON enabled; T5RD16 enabled; 
	T5CON = 0x23;
}

void TMR5_StartTimer(void)
{
	// Start the Timer by writing to TMRxON bit
	T5CONbits.TMR5ON = 1;
}

void TMR5_StopTimer(void)
{
	// Stop the Timer by writing to TMRxON bit
	T5CONbits.TMR5ON = 0;
}

uint16_t TMR5_ReadTimer(void)
{
	uint16_t readVal;
	uint8_t readValHigh;
	uint8_t readValLow;

	T5CONbits.T5RD16 = 1;

	readValLow = TMR5L;
	readValHigh = TMR5H;

    readVal = ((uint16_t)readValHigh << 8) | readValLow;

	return readVal;
}

void TMR5_WriteTimer(uint16_t timerVal)
{
    if (T5CONbits.T5SYNC == 1)
    {
		// Stop the Timer by writing to TMRxON bit
		T5CONbits.TMR5ON = 0;

		// Write to the Timer5 register
		TMR5H = (timerVal >> 8);
		TMR5L = (uint8_t) timerVal;

		// Start the Timer after writing to the register
        T5CONbits.TMR5ON =1;
    }
    else
    {
		// Write to the Timer5 register
		TMR5H = (timerVal >> 8);
		TMR5L = (uint8_t) timerVal;
	}
}

void TMR5_Reload(void)
{
	TMR5_WriteTimer(timer5ReloadVal);
}

void TMR5_StartSinglePulseAcquisition(void)
{
	T5GCONbits.T5GGO = 1;
}

uint8_t TMR5_CheckGateValueStatus(void)
{
	return T5GCONbits.T5GVAL;
}

void TMR5_ISR(void)
{

	// Clear the TMR5 interrupt flag
	PIR5bits.TMR5IF = 0;
	TMR5_WriteTimer(timer5ReloadVal);

    if(TMR5_InterruptHandler)
    {
		TMR5_InterruptHandler();
	}
}


void TMR5_SetInterruptHandler(void (* InterruptHandler)(void)){
	TMR5_InterruptHandler = InterruptHandler;
}

void TMR5_DefaultInterruptHandler(void){
	// add your TMR5 interrupt custom code
	// or set custom function using TMR5_SetInterruptHandler()
	LED4 = ~LED4;
	V.rpm_overflow = true;
}


/**
 End of File
 */
