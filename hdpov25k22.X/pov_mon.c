
#include  <xc.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "pov_mon.h"
#include <string.h>
#include "ringbufs.h"

int16_t sw_work(void);
void init_povmon(void);
uint8_t init_hov_params(void);

/* line strobes in 16-bit timer values for spacing */
/* for an interrupt driven state machine */
/*
 * 8.35ms per rotation
 * 235us for sense tab width
 * 7 tabs for first led position.
 */

struct L_data L[strobe_max] = {
	{
		.strobe = 62350,
		.sequence.G = true,
		.sequence.offset = 0,
		.sequence.end = true,
	},
	{
		.strobe = 50000,
		.sequence.R = true,
		.sequence.offset = 0,
	},
	{
		.strobe = 40000,
		.sequence.B = true,
		.sequence.offset = 0,
	},
	{
		.strobe = 30000,
		.sequence.R = true,
		.sequence.G = true,
		.sequence.B = true,
		.sequence.offset = 0,
		.sequence.end = true,
	}
}, *L_ptr;

struct V_data V = {
	.rpm_overflow = true,
	.rpm_update = false,
	.line_num = 0,
	.comm_state = APP_STATE_INIT,
	.l_size = sizeof(L[0]),
	.l_state = ISR_STATE_WAIT,
	.l_full = strobe_limit_l,
	.l_width = strobe_line,
};



/* RS232 command buffer */
struct ringBufS_t ring_buf1;

const char build_date[] = __DATE__, build_time[] = __TIME__, versions[] = "2.00";
const uint16_t TIMEROFFSET = 18000, TIMERDEF = 60000;

/*
 * THIS CODE NOT USED
 * interrupt code moved to the needed ISR routines for each device module
 */
void tm_handler(void) // timer/serial functions are handled here
{
	LED1 = 1;
	// line rotation sequencer
	if (INTCONbits.INT0IF) { // Hall effect index signal, start of rotation
		INTCONbits.INT0IF = false;
		RPMLED = (uint8_t)!RPMLED;
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
		case true:
			L_ptr->strobe -= L_ptr->sequence.offset;
			if (L_ptr->strobe < V.l_full)
				L_ptr->strobe = strobe_limit_h;
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
	}

	// line RGB pulsing state machine
	if (PIR1bits.TMR1IF || (V.l_state == ISR_STATE_FLAG)) { // Timer1 int handler, for strobe rotation timing
		PIR1bits.TMR1IF = false;

		switch (V.l_state) {
		case ISR_STATE_FLAG:
			WRITETIMER1(L_ptr->strobe); // strobe positioning during rotation
			T1CONbits.TMR1ON = 1;
			G_OUT = 0;
			R_OUT = 0;
			B_OUT = 0;
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
			}

			V.l_state = ISR_STATE_WAIT; // on start time duration for strobe pulse
			break;
		case ISR_STATE_WAIT: // waiting for next HALL sensor pulse
		default:
			T1CONbits.TMR1ON = 0; // idle timer
			G_OUT = 0; // blank RGB
			R_OUT = 0;
			B_OUT = 0;
			break;
		}
	}

	// remote command data buffer
	if (PIR1bits.RCIF) { // is data from RS-232 port
		V.rx_data = RCREG; // save in state machine register
		if (RCSTAbits.OERR) {
			RCSTAbits.CREN = 0; // clear overrun
			RCSTAbits.CREN = 1; // re-enable
		}
		ringBufS_put(&ring_buf1, V.rx_data); // buffer RS232 data
	}

	// check timer0 for blinker led
	if (INTCONbits.TMR0IF) {
		INTCONbits.TMR0IF = false;
		WRITETIMER0(TIMEROFFSET);
		LED5 = ~LED5; // active LED blinker
	}
	LED1 = 0;
}

void uitoa(uint8_t * Buffer, uint16_t Value)
{
	uint8_t i;
	uint16_t Digit;
	uint16_t Divisor;
	bool Printed = false;

	if (Value) {
		for (i = 0, Divisor = 10000; i < 5u; i++) {
			Digit = Value / Divisor;
			if (Digit || Printed) {
				*Buffer++ = '0' + Digit;
				Value -= Digit*Divisor;
				Printed = true;
			}
			Divisor /= 10;
		}
	} else {
		*Buffer++ = '0';
	}

	*Buffer = '\0';
}

void USART_putc(uint8_t c)
{
	while (!TXSTAbits.TRMT);
	TXREG = c;
}

void USART_puts(uint8_t *s)
{
	while (*s) {
		USART_putc(*s);
		s++;
	}
}

void USART_putsr(const char *s)
{
	while (*s) {
		USART_putc(*s);
		s++;
	}
}

void puts_ok(uint16_t size)
{
	//	itoa(V.str, size, 10);
	uitoa(V.str, size);
	USART_putsr("\r\n OK");
	USART_puts(V.str); // send size of data array
}

/* main loop work routine */
int16_t sw_work(void)
{
	static uint8_t position = 0, offset = 0, rx_data;
	static uint8_t *L_tmp_ptr;

	static union L_union_type { // so we can access each byte of the command structure
		uint8_t L_bytes[sizeof(L[0]) + 1];
		L_data L_tmp;
	} L_union;
	int16_t ret = 0;

	if (V.l_state != ISR_STATE_WAIT)
		ret = -1;

	if (!SW1) {
		USART_putsr("\r\n RPM counts,");
		uitoa(V.str, V.rpm_counts);
		USART_puts(V.str);
		V.rpm_update = false;
		USART_putsr(" strobe,");
		uitoa(V.str, L_ptr->strobe);
		USART_puts(V.str);
	}

	/* command state machine 
	 * u/U update the current display buffer with remote RS232 data
	 * d/D display the current display buffer on RS232 port
	 * e/E clear/set end of lines flag on display buffer
	 * i/I timer info command
	 * z/Z null command
	 */
	if (!ringBufS_empty(&ring_buf1)) {
		rx_data = ringBufS_get(&ring_buf1);
		switch (V.comm_state) {
		case APP_STATE_INIT:
			switch (rx_data) {
			case 'u':
			case 'U':
				V.comm_state = APP_STATE_WAIT_FOR_UDATA;
				break;
			case 'd':
			case 'D':
				V.comm_state = APP_STATE_WAIT_FOR_DDATA;
				break;
			case 'e':
				V.comm_state = APP_STATE_WAIT_FOR_eDATA;
				puts_ok(V.l_size);
				break;
			case 'E':
				V.comm_state = APP_STATE_WAIT_FOR_EDATA;
				puts_ok(V.l_size);
				break;
			case 'i':
			case 'I': // info command
				USART_putsr(" Timer limit,");
				//				itoa(V.str, V.l_full, 10);
				uitoa(V.str, V.l_full);
				USART_puts(V.str);
				USART_putsr(" OK");
				break;
			case 'z':
			case 'Z': // null command for fillers, silent
				break;
			default:
				USART_putsr("\r\n NAK_I");
				ret = -1;
				break;
			}
			break;
		case APP_STATE_WAIT_FOR_eDATA:
		case APP_STATE_WAIT_FOR_EDATA:
		case APP_STATE_WAIT_FOR_DDATA:
		case APP_STATE_WAIT_FOR_UDATA:
			position = rx_data;
			if (position >= strobe_max) {
				USART_putsr(" NAK_P");
				V.comm_state = APP_STATE_INIT;
				ret = -1;
				break;
			}
			offset = 0;
			switch (V.comm_state) {
			case APP_STATE_WAIT_FOR_UDATA:
				V.comm_state = APP_STATE_WAIT_FOR_RDATA;
				break;
			case APP_STATE_WAIT_FOR_DDATA:
				V.comm_state = APP_STATE_WAIT_FOR_SDATA;
				break;
			case APP_STATE_WAIT_FOR_eDATA:
				INTCONbits.GIEH = 0;
				L[position].sequence.end = 0; // clear end flag
				INTCONbits.GIEH = 1;
				V.comm_state = APP_STATE_WAIT_FOR_SDATA;
				break;
			case APP_STATE_WAIT_FOR_EDATA:
				INTCONbits.GIEH = 0;
				L[position].sequence.end = 1; // set end flag
				INTCONbits.GIEH = 1;
				V.comm_state = APP_STATE_WAIT_FOR_SDATA;
				break;
			default:
				break;
			}
			USART_putsr(" OK");
			break;
		case APP_STATE_WAIT_FOR_RDATA: // receive
			L_union.L_bytes[offset] = rx_data;
			offset++;
			if (offset >= sizeof(L_union.L_tmp)) {
				INTCONbits.GIEH = 0;
				L[position] = L_union.L_tmp;
				INTCONbits.INT0IF = false;
				INTCONbits.GIEH = 1;
				USART_putsr(" OK,");
				//				utoa(V.str, (uint16_t) L_union.L_tmp.strobe, 10);
				uitoa(V.str, (uint16_t) L_union.L_tmp.strobe);
				USART_puts(V.str);
				V.comm_state = APP_STATE_INIT;
			}
			break;
		case APP_STATE_WAIT_FOR_SDATA: // send
			L_tmp_ptr = (void*) &L[position]; // set array start position
			do { // send ascii data to the rs232 port
				USART_putsr(" ,");
				if (offset) {
					//					itoa(V.str, *L_tmp_ptr, 16); // show hex
					uitoa(V.str, *L_tmp_ptr); // show dec
				} else {
					//					itoa(V.str, *L_tmp_ptr, 2); // show bits
					uitoa(V.str, *L_tmp_ptr); // show dec
				}
				USART_puts(V.str);
				L_tmp_ptr++;
				offset++;
			} while (offset < V.l_size);
			V.comm_state = APP_STATE_INIT;
			USART_putsr(" OK");
			break;
		default:
			USART_putsr(" NAK_C");
			V.comm_state = APP_STATE_INIT;
			if (ringBufS_full(&ring_buf1))
				ringBufS_flush(&ring_buf1, 0);
			ret = -1;
			break;
		}
	}

	return ret;
}

/* controller hardware setup */
void init_povmon(void)
{
	/*
	 * check for a clean POR
	 */
	V.boot_code = false;
	if (RCON != 0b0011100)
		V.boot_code = true;

	if (STKPTRbits.STKFUL || STKPTRbits.STKUNF) {
		V.boot_code = true;
		STKPTRbits.STKFUL = 0;
		STKPTRbits.STKUNF = 0;
	}

	init_hov_params();
	ringBufS_init(&ring_buf1);

}

/* program data setup */
uint8_t init_hov_params(void)
{

	USART_putsr("\r\nVersion ");
	USART_putsr(versions);
	USART_putsr(", ");
	//	itoa(V.str, sizeof(L[0]), 10);
	USART_puts(V.str);
	USART_putsr(", ");
	USART_putsr(build_date);
	USART_putsr(", ");
	USART_putsr(build_time);
	if (V.boot_code)
		USART_putsr(", dirty boot");

	L_ptr = &L[0];
	L[strobe_max - 1].sequence.end = 1;

	return 0;
}

void main_hdpov(void)
{
	/* configure system */
	init_povmon();

	/* Loop forever */
	while (true) { // busy work
		sw_work(); // run housekeeping for non-ISR tasks
	}
}
