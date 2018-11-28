
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

struct S_seq S = {
	.zero_offset = z_offset,
	.slot_count = s_count,
	.disk_count = d_count,
};

/*
 * pattern test data
 */
struct L_data L0[strobe_max] = {
	{
		.strobe = z_offset - d_count,
		.sequence.G = true,
		.sequence.offset = 0,
		.sequence.end = false,
	},
	{
		.strobe = z_offset - s_count,
		.sequence.R = true,
		.sequence.offset = 0,
	},
	{
		.strobe = (z_offset - d_count) - s_count * 2,
		.sequence.B = true,
		.sequence.offset = 0,
	},
	{
		.strobe = z_offset - s_count * 3,
		.sequence.A = true,
		.sequence.offset = 0,
		.sequence.end = true,
	}
}, *L_ptr, *L_ptr_buf,
	L1[strobe_max] = {
	{
		.strobe = z_offset - d_count,
		.sequence.G = true,
		.sequence.offset = 0,
		.sequence.end = false,
	},
	{
		.strobe = z_offset - s_count,
		.sequence.R = false,
		.sequence.offset = 0,
	},
	{
		.strobe = (z_offset - d_count) - s_count * 2,
		.sequence.B = true,
		.sequence.offset = 0,
	},
	{
		.strobe = z_offset - s_count * 3,
		.sequence.A = false,
		.sequence.offset = 0,
		.sequence.end = true,
	}
}
;

struct V_data V = {
	.rpm_overflow = true,
	.rpm_update = false,
	.line_num = 0,
	.comm_state = APP_STATE_INIT,
	.l_size = sizeof(L0[0]),
	.l_state = ISR_STATE_WAIT,
	.l_full = strobe_limit_l,
	.l_width = strobe_line,
	.l_buffer = 0,
};



/* RS232 command buffer */
struct ringBufS_t ring_buf1;

const char build_date[] = __DATE__, build_time[] = __TIME__, versions[] = "2.00";
const uint16_t TIMEROFFSET = 18000, TIMERDEF = 60000;

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
		uint8_t L_bytes[sizeof(L0[0]) + 1];
		L_data L_tmp;
	} L_union;
	int16_t ret = 0;

	if (V.l_state != ISR_STATE_DONE)
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
			/*
			 * swap working buffer into pointer for updates
			 */

			switch (V.l_buffer) {
			case 0:
				L_ptr_buf = &L1[position]; // select line strobes data 1
				break;
			default:
				L_ptr_buf = &L0[position]; // select line strobes data 0
				break;
			}
			switch (V.comm_state) {
			case APP_STATE_WAIT_FOR_UDATA:
				V.comm_state = APP_STATE_WAIT_FOR_RDATA;
				break;
			case APP_STATE_WAIT_FOR_DDATA:
				V.comm_state = APP_STATE_WAIT_FOR_SDATA;
				break;
			case APP_STATE_WAIT_FOR_eDATA:
				INTCONbits.GIEH = 0;
				L_ptr_buf->sequence.end = 0; // clear end flag
				INTCONbits.GIEH = 1;
				V.comm_state = APP_STATE_WAIT_FOR_SDATA;
				break;
			case APP_STATE_WAIT_FOR_EDATA:
				INTCONbits.GIEH = 0;
				L_ptr_buf->sequence.end = 1; // set end flag
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
				*L_ptr_buf = L_union.L_tmp;
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
			L_tmp_ptr = (void*) L_ptr_buf; // set array start position
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

	L_ptr = &L0[0];
	L_ptr_buf = &L1[0];
	L0[strobe_max - 1].sequence.end = 1;
	L1[strobe_max - 1].sequence.end = 1;

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
