/**
  Generated Interrupt Manager Header File

  @Company:
    Microchip Technology Inc.

  @File Name:
    interrupt_manager.h

  @Summary:
    This is the Interrupt Manager file generated using PIC10 / PIC12 / PIC16 / PIC18 MCUs

  @Description:
    This header file provides implementations for global interrupt handling.
    For individual peripheral handlers please see the peripheral driver for
    all modules selected in the GUI.
    Generation Information :
	Product Revision  :  PIC10 / PIC12 / PIC16 / PIC18 MCUs - 1.65
        Device            :  PIC18F45K22
	Driver Version    :  1.02
    The generated drivers are tested against the following:
	Compiler          :  XC8 1.45 or later
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

#include "interrupt_manager.h"
#include "mcc.h"
#include "../pov_mon.h"

void INTERRUPT_Initialize(void)
{
	// Enable Interrupt Priority Vectors
	RCONbits.IPEN = 1;

	// Assign peripheral interrupt priority vectors

	// Interrupt INT0I has no priority bit. It will always be called from the High Interrupt Vector

	// RCI - high priority
	IPR1bits.RC1IP = 1;

	// TMRI - high priority
	IPR1bits.TMR1IP = 1;


	// TMRI - low priority
	IPR2bits.TMR3IP = 0;

	// TMRI - low priority
	IPR5bits.TMR5IP = 0;

}

void __interrupt(high_priority) INTERRUPT_InterruptManagerHigh(void)
{
	LED2 = 1;
	// interrupt handler
    if(INTCONbits.INT0IE == 1 && INTCONbits.INT0IF == 1)
    {
		INT0_ISR();
	}
    if(PIE1bits.RC1IE == 1 && PIR1bits.RC1IF == 1)
    {
		EUSART1_Receive_ISR();
	}
    if(PIE1bits.TMR1IE == 1 && PIR1bits.TMR1IF == 1)
    {
		TMR1_ISR();
	}
	LED2 = 0;
}

void __interrupt(low_priority) INTERRUPT_InterruptManagerLow(void)
{
	LED2 = ~LED2;
	// interrupt handler
    if(PIE2bits.TMR3IE == 1 && PIR2bits.TMR3IF == 1)
    {
		TMR3_ISR();
	}
    if(PIE5bits.TMR5IE == 1 && PIR5bits.TMR5IF == 1)
    {
		TMR5_ISR();
	}
	LED2 = ~LED2;
}
/**
 End of File
 */
