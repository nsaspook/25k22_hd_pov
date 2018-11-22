/**
  TMR1 Generated Driver File

  @Company
    Microchip Technology Inc.

  @File Name
    tmr1.c

  @Summary
    This is the generated driver implementation file for the TMR1 driver using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  @Description
    This source file provides APIs for TMR1.
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
#include "tmr1.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../pov_mon.h"
#include <string.h>
#include "../ringbufs.h"

/**
  Section: Global Variable Definitions
 */
volatile uint16_t timer1ReloadVal;
void (*TMR1_InterruptHandler)(void);

extern struct V_data V;
extern struct L_data L[strobe_max], *L_ptr;

/**
  Section: TMR1 APIs
 */

void TMR1_Initialize(void)
{
	//Set the Timer to the options selected in the GUI

	//T1GSS T1G_pin; TMR1GE disabled; T1GTM disabled; T1GPOL low; T1GGO done; T1GSPM disabled; 
	T1GCON = 0x00;

	//TMR1H 193; 
	TMR1H = 0xC1;

	//TMR1L 128; 
	TMR1L = 0x80;

	// Load the TMR value to reload variable
	timer1ReloadVal = TMR1;

	// Clearing IF flag before enabling the interrupt.
	PIR1bits.TMR1IF = 0;

	// Enabling TMR1 interrupt.
	PIE1bits.TMR1IE = 1;

	// Set Default Interrupt Handler
	TMR1_SetInterruptHandler(TMR1_DefaultInterruptHandler);

	// T1CKPS 1:1; T1OSCEN disabled; T1SYNC synchronize; TMR1CS FOSC/4; TMR1ON enabled; T1RD16 enabled; 
	T1CON = 0x03;
}

void TMR1_StartTimer(void)
{
	// Start the Timer by writing to TMRxON bit
	T1CONbits.TMR1ON = 1;
}

void TMR1_StopTimer(void)
{
	// Stop the Timer by writing to TMRxON bit
	T1CONbits.TMR1ON = 0;
}

uint16_t TMR1_ReadTimer(void)
{
	uint16_t readVal;
	uint8_t readValHigh;
	uint8_t readValLow;

	T1CONbits.T1RD16 = 1;

	readValLow = TMR1L;
	readValHigh = TMR1H;

	readVal = ((uint16_t) readValHigh << 8) | readValLow;

	return readVal;
}

void TMR1_WriteTimer(uint16_t timerVal)
{
	if (T1CONbits.T1SYNC == 1) {
		// Stop the Timer by writing to TMRxON bit
		T1CONbits.TMR1ON = 0;

		// Write to the Timer1 register
		TMR1H = (timerVal >> 8);
		TMR1L = (uint8_t) timerVal;

		// Start the Timer after writing to the register
		T1CONbits.TMR1ON = 1;
	} else {
		// Write to the Timer1 register
		TMR1H = (timerVal >> 8);
		TMR1L = (uint8_t) timerVal;
	}
}

void TMR1_Reload(void)
{
	TMR1_WriteTimer(timer1ReloadVal);
}

void TMR1_StartSinglePulseAcquisition(void)
{
	T1GCONbits.T1GGO = 1;
}

uint8_t TMR1_CheckGateValueStatus(void)
{
	return T1GCONbits.T1GVAL;
}

void TMR1_ISR(void)
{

	// Clear the TMR1 interrupt flag
	PIR1bits.TMR1IF = 0;
	TMR1_WriteTimer(timer1ReloadVal);

	if (TMR1_InterruptHandler) {
		TMR1_InterruptHandler();
	}
}

void TMR1_SetInterruptHandler(void (* InterruptHandler)(void))
{
	TMR1_InterruptHandler = InterruptHandler;
}

void TMR1_DefaultInterruptHandler(void)
{
	// add your TMR1 interrupt custom code
	// or set custom function using TMR1_SetInterruptHandler()
	// line RGB pulsing state machine

	LED3 = ~LED3;
	switch (V.l_state) {
	case ISR_STATE_FLAG:
		WRITETIMER1(L_ptr->strobe); // strobe positioning during rotation
		T1CONbits.TMR1ON = 1;
		G_OUT = 0;
		R_OUT = 0;
		B_OUT = 0;
		A_OUT = 0;
		V.l_state = ISR_STATE_LINE; // off time after index to start time
		break;
	case ISR_STATE_LINE:
		WRITETIMER1(V.l_width);
		if (!L_ptr->sequence.skip) {
			if (L_ptr->sequence.R)
				R_OUT = 1;
			if (L_ptr->sequence.G)
				G_OUT = 1;
			if (L_ptr->sequence.B)
				B_OUT = 1;
			if (L_ptr->sequence.A)
				A_OUT = 1;

		}

		V.l_state = ISR_STATE_WAIT; // on start time duration for strobe pulse
		break;
	case ISR_STATE_WAIT: // waiting for next HALL sensor pulse
	default:
		T1CONbits.TMR1ON = 0; // idle timer
		G_OUT = 0; // blank RGB
		R_OUT = 0;
		B_OUT = 0;
		A_OUT = 0;
		break;
	}
}


/**
 End of File
 */
