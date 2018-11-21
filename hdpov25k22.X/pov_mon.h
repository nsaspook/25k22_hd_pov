#ifndef PAT_H_INCLUDED
#define PAT_H_INCLUDED 

void init_povmon(void);
int16_t sw_work(void);

typedef enum {
	/* rs232 Application's state machine's initial state. */
	APP_STATE_INIT = 0,
	APP_STATE_WAIT_FOR_UDATA,
	APP_STATE_WAIT_FOR_RDATA,
	APP_STATE_WAIT_FOR_DDATA,
	APP_STATE_WAIT_FOR_SDATA,
	APP_STATE_WAIT_FOR_eDATA,
	APP_STATE_WAIT_FOR_EDATA,
	/* Application Error state*/
	APP_STATE_ERROR

} APP_STATES;

typedef enum {
	/* rotation state machine */
	ISR_STATE_FLAG = 0,
	ISR_STATE_LINE,
	ISR_STATE_WAIT,
	ISR_STATE_ERROR

} ISR_STATES;

typedef struct V_data { // control data structure with possible volatile issues
	APP_STATES comm_state;
	volatile ISR_STATES l_state;
	uint8_t boot_code : 1;
	volatile uint8_t line_num : 2;
	volatile uint8_t rx_data;
	uint16_t l_size;
	volatile uint16_t rotations, sequences, rpm_counts;
	volatile bool rpm_overflow, rpm_update;
	volatile uint16_t l_full, l_width;
	uint8_t str[24];
} V_data;

typedef volatile struct L_seq {
	uint8_t down : 1; // rotation direction
	uint8_t R : 1;
	uint8_t G : 1;
	uint8_t B : 1;
	uint8_t end : 1; // last line in sequence
	uint8_t skip : 1; // don't light led
	uint8_t rot : 1; // rotation and sequence flags
	uint8_t seq : 1;
	uint16_t offset; // line movement 
} L_seq;

typedef volatile struct S_seq {
	uint16_t zero_offset;	// flag to first led position
	uint16_t slot_count;	// counts to next led position
	uint16_t disk_slot_count, disk_next_count;
} S_seq;

/* data for one complete rotation*/
typedef volatile struct L_data {
	struct L_seq sequence;
	struct S_seq slot;
	uint16_t strobe;
} L_data;

#define	ON      true
#define	OFF     false

//	hardware defines
#define RMSPORTA	TRISA
#define RMSPORTB	TRISB
#define RMSPORT_IOA	0b00010000		// SW1 input RA4
#define RMSPORT_IOB	0b00010001		// Rs-232 transmit on B1, receive on B4, hall gear sensor on B0

#define LED1		LATCbits.LATC3
#define LED2		LATCbits.LATC2
#define LED3		LATCbits.LATC1
#define LED4		LATCbits.LATC0
#define LED5		LATAbits.LATA6
#define LED6		LATAbits.LATA7		

#define G_OUT		LATAbits.LATA0
#define R_OUT		LATAbits.LATA1
#define B_OUT		LATAbits.LATA2
#define A_OUT		LATAbits.LATA3
#define BLINKLED	PORTBbits.RB5
#define RPMLED		LATCbits.LATC3
#define SW1		PORTAbits.RA4

#define PAT2		// display patterns

#ifdef PAT1
#define strobe_up	67
#define strobe_down	31
#define strobe_around	109
#endif

#ifdef PAT2
#define strobe_up	60
#define strobe_down	360
#define strobe_around	1080
#endif

/* rotation params for 40mHZ PIC18f1320 */
#define strobe_adjust	11
#define strobe_limit_l	10400 // this limit is the starting point for the calc'd value from the rs-232 port
#define strobe_limit_h	65534
#define strobe_line	62400 // line width timer count
#define strobe_max	16
#endif 