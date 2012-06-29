//******************************************************************************
//
// Launchpad staircase switch application
//
// author: Ondrej Hejda
// date:   21.6.2012
//
// description:
//
//	 Swapping output state (OUT_ON/OUT_OFF) when pushbutton pressed (BTN_DOWN).
//   Output state is indicated by red and green LEDs (LED_RED..,LED_GREEN..),
//   or double led (red/green).
//   Input button is filtered (10 x 5ms). LEDs are smoothed (5 levels).
//
// used TI demos:
//
//   MSP430G2xx3 Demo - P1 Interrupt from LPM4 with Internal Pull-up
//   MSP430G2xx3 Demo - Timer_A, Toggle P1.0, CCR0 Cont. Mode ISR, DCO SMCLK
//
// hw: two variants of MCU supported (Launchpad equipment)
//   MSP430G2553, MSP430G2452 .. select by macro in include section
//
//                MSP4302x5x
//             -----------------
//         /|\|              XIN|-
//          | |                 |
//          --|RST          XOUT|-
//            |                 |
//            |    P1.0 (P2.3,4)|--> RED LED (active high)   |
//            |      P1.6 (P2.5)|--> GREEN LED (active high) | or R/G led two/w
//            |                 |
//            |             P1.3|<-- BUTTON WITH PULLUP (active low)
//            |                 |
//            |           P2.1,2|--> OUT (active low)
//            |                 |
//
// 2012.06.29: output port connected
//             leds pin and output pin doubled (tripled) to get more current
//
//******************************************************************************


// include section
#define MSP430G2452
#ifndef MSP430G2452
	#include "msp430g2553.h"
#else
	#include "msp430g2452.h"
#endif
#include "stdbool.h"


// board (leds, button)
#define LED_INIT() {P1DIR|=0x41;P1OUT&=~0x41;P2DIR|=0x38;P2OUT&=~0x38;}
#define LED_RED_ON() {P1OUT|=0x01;P2OUT|=0x18;}
#define LED_RED_OFF() {P1OUT&=~0x01;P2OUT&=~0x18;}
#define LED_GREEN_ON() {P1OUT|=0x40;P2OUT|=0x20;}
#define LED_GREEN_OFF() {P1OUT&=~0x40;P2OUT&=~0x20;}

#define BTN_INIT() {P1DIR&=~0x08;P1REN|=0x08;}
#define BTN_DOWN ((P1IN&0x08)==0)
#define OUT_INIT() {P2DIR|=0x06;P2OUT|=0x06;}
#define OUT_ON() {P2OUT&=~0x06;}
#define OUT_OFF() {P2OUT|=0x06;}

// time base 5ms (5000 ticks / 1MHz)
#define Timer_Const 5000
#define Led_PWM_Levels 5
#define Btn_Filter 10
#define Btn_Filter_MAX 100


// timer initialization
void timer_init(void)
{
	CCTL0 = CCIE;				// CCR0 interrupt enabled
	CCR0 = Timer_Const;
	TACTL = TASSEL_2 + MC_2;	// SMCLK, contmode
}


// hw depended initialization
void board_init(void)
{
	// oscillator
	BCSCTL1 = CALBC1_1MHZ;		// Set DCO
	DCOCTL = CALDCO_1MHZ;

	// unused pins (all but future init)
	P1OUT = 0x00; P1DIR = 0xFF;
	P2OUT = 0x00; P2DIR = 0xFF;
	#ifndef MSP430G2452
	P3OUT = 0x00; P3DIR = 0xFF;
	#endif

	LED_INIT(); // leds
	BTN_INIT(); // button
	OUT_INIT(); // output
}


// main program body
int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;	// Stop WDT

	board_init();
	timer_init();

	__bis_SR_register(LPM0_bits + GIE);		// Enter LPM0, interrupts enabled

    return -1;
}


// Timer A0 interrupt service routine (5ms)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	static unsigned int OnCnt=0, OffCnt=0; // on and off button state counter
	static unsigned int sw_seqv = 0; // switch sequential pointer
	static bool out_st = false; // output state (on/off)

	// button reading
	if (BTN_DOWN)
	{
		if (OnCnt<Btn_Filter_MAX) OnCnt++;
		OffCnt=0;
	}
	else
	{
		if (OffCnt<Btn_Filter_MAX) OffCnt++;
		OnCnt=0;
	}

	// button sequential
	switch (sw_seqv)
	{
		case 0: // Off (after reset) .. wait button
			if (OnCnt==Btn_Filter)
			{
				//LED_RED_ON();
				//LED_GREEN_OFF();
				OUT_ON();
				out_st=true;
				sw_seqv++;
			}
			break;
		case 1: // Wait button release
			if (OffCnt==Btn_Filter) sw_seqv++;
			break;
		case 2: // On .. wait button
			if (OnCnt==Btn_Filter)
			{
				//LED_RED_OFF();
				//LED_GREEN_ON();
				OUT_OFF();
				out_st=false;
				sw_seqv++;
			}
			break;
		case 3: // Wait button release
			if (OffCnt==Btn_Filter) sw_seqv=0;
			break;
		default:
			sw_seqv=0;
			break;
	}

	// led pwm
	static unsigned int led_pwm_cnt=0; // led pwm counter
	static int led_red_pwm=0, led_green_pwm=0; // led pwm actual value
	if (led_red_pwm>led_pwm_cnt) LED_RED_ON()
	else LED_RED_OFF();
	if (led_green_pwm>led_pwm_cnt) LED_GREEN_ON()
	else LED_GREEN_OFF();
	led_pwm_cnt++;
	if (led_pwm_cnt>=Led_PWM_Levels)
	{
		led_pwm_cnt=0;

		// led sequential
		static unsigned int led_seqv = 0; // led sequential pointer
		switch (led_seqv)
		{
			case 0: // leds off
				if (out_st) led_seqv=4;
				else led_seqv=1;
				break;
			case 1: // red led goes on
				led_red_pwm++;
				if (led_red_pwm>=Led_PWM_Levels)
				{
					led_red_pwm=Led_PWM_Levels;
					led_seqv++;
				}
				break;
			case 2: // red led is on
				if (out_st) led_seqv++;
				break;
			case 3: // red led goes of
				led_red_pwm--;
				if (led_red_pwm<=0)
				{
					led_red_pwm=0;
					led_seqv=0;
				}
				break;
			case 4: // green led goes on
				led_green_pwm++;
				if (led_green_pwm>=Led_PWM_Levels)
				{
					led_green_pwm=Led_PWM_Levels;
					led_seqv++;
				}
				break;
			case 5: // green led is on
				if (!out_st) led_seqv++;
				break;
			case 6: // green led goes off
				led_green_pwm--;
				if (led_green_pwm<=0)
				{
					led_green_pwm=0;
					led_seqv=0;
				}
				break;
			default:
				led_green_pwm=0;
				led_red_pwm=0;
				led_seqv=0;
				break;
		}
	}

	CCR0 += Timer_Const;		// Add Offset to CCR0
}


// unused interrupts handler
#ifndef MSP430G2452
#pragma vector=	ADC10_VECTOR,\
				COMPARATORA_VECTOR,\
				NMI_VECTOR,\
				PORT1_VECTOR,\
				PORT2_VECTOR,\
				TIMER0_A1_VECTOR,\
				TIMER1_A0_VECTOR,\
				TIMER1_A1_VECTOR,\
				USCIAB0RX_VECTOR,\
				USCIAB0TX_VECTOR,\
				WDT_VECTOR
#else
#pragma vector=	ADC10_VECTOR,\
				COMPARATORA_VECTOR,\
				NMI_VECTOR,\
				PORT1_VECTOR,\
				PORT2_VECTOR,\
				TIMER0_A1_VECTOR,\
				WDT_VECTOR
#endif
__interrupt void ISR_trap(void)
{
  // the following will cause an access violation which results in a PUC reset
  WDTCTL = 0;
}
