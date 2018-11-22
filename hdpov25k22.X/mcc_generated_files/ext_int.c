/**
   EXT_INT Generated Driver File
 
   @Company
     Microchip Technology Inc.
 
   @File Name
     ext_int.c
 
   @Summary
     This is the generated driver implementation file for the EXT_INT driver using PIC10 / PIC12 / PIC16 / PIC18 MCUs
 
   @Description
     This source file provides implementations for driver APIs for EXT_INT.
     Generation Information :
	 Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.65
	 Device            :  PIC18F25K22
	 Driver Version    :  1.00
     The generated drivers are tested against the following:
	 Compiler          :  XC8 1.45
	 MPLAB             :  MPLAB X 4.10
 */

/**
  Section: Includes
 */
#include <xc.h>
#include "ext_int.h"
#include "ccp4.h"
#include "tmr5.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../pov_mon.h"
#include <string.h>
#include "../ringbufs.h"

void (*INT0_InterruptHandler)(void);
extern struct V_data V;
extern struct L_data L[strobe_max], *L_ptr;

void INT0_ISR(void)
{
	EXT_INT0_InterruptFlagClear();

	// Callback function gets called everytime this ISR executes
	INT0_CallBack();
}

void INT0_CallBack(void)
{
	// Add your custom callback code here
	if (INT0_InterruptHandler) {
		INT0_InterruptHandler();
	}
}

void INT0_SetInterruptHandler(void (* InterruptHandler)(void))
{
	INT0_InterruptHandler = InterruptHandler;
}

void INT0_DefaultInterruptHandler(void)
{
	LED1 = ~LED1;
	LED2 = 1;
	LED3 = 1;
	LED4 = 1;
	if (PIR4bits.CCP4IF) {
		LED5 = ~LED5;
		PIR4bits.CCP4IF = 0;
	}
	if (!V.rpm_overflow) {
		if (!V.rpm_update) {
			V.rpm_counts = CCP4_CaptureRead();
			V.rpm_update = true;
		}
		LED5 = ~LED5;
	}
	TMR5_WriteTimer(0);

	// line rotation sequencer
	// Hall effect index signal, start of rotation

	if (V.l_state == ISR_STATE_LINE) { // off state too long for full rotation, hall signal while in state
		V.l_full += strobe_adjust; // off state lower limit adjustments for smooth strobe rotation
	}
	V.l_state = ISR_STATE_FLAG; // restart lamp flashing sequence, off time

	L_ptr = &L[V.line_num]; // select line strobe data
	V.rotations++;

	/* limit rotational timer values during offsets */
	switch (L_ptr->sequence.down) {
	case false:
		L_ptr->strobe += L_ptr->sequence.offset;
		if (L_ptr->strobe < V.l_full)
			L_ptr->strobe = V.l_full; // set to sliding lower limit
		break;
	default:
		L_ptr->strobe -= L_ptr->sequence.offset;
		if (L_ptr->strobe < V.l_full)
			L_ptr->strobe = strobe_limit_h;
		break;
	}
	V.line_num++;
	if (L_ptr->sequence.end || (V.line_num >= strobe_max)) { // rollover for sequence patterns
		V.line_num = 0;
		V.sequences++;
	}

	// start line RGB pulsing state machine
	WRITETIMER1(L_ptr->strobe); // strobe positioning during rotation
	T1CONbits.TMR1ON = 1;
	G_OUT = 0;
	R_OUT = 0;
	B_OUT = 0;
	A_OUT = 0;
	V.l_state = ISR_STATE_LINE; // off time after index to start time
	V.rpm_overflow = false;
	BLINKLED = ~BLINKLED;
	LED2 = 0;
}

void EXT_INT_Initialize(void)
{

	// Clear the interrupt flag
	// Set the external interrupt edge detect
	EXT_INT0_InterruptFlagClear();
	EXT_INT0_fallingEdgeSet();
	// Set Default Interrupt Handler
	INT0_SetInterruptHandler(INT0_DefaultInterruptHandler);
	EXT_INT0_InterruptEnable();

}

