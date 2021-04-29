/*I2C demo program to read a single byte of data from from a slave. Master is an MSP430FR5969 Launchpad.
 * Slave is a MSP430FR2355 Launchpad. If the master reads data byte = 0x03, flash green LED.
 * Otherwise flash red LED. Slave is polled at rate set by the VLO timer. I2C is on SMCLK. This is the MASTER code. */
#include <msp430.h> 
#include <stdio.h>
#include <stdint.h>

//Define times for VLO clock
# define PERIOD 20000 //Samping period. 10000 count is approximately 1 second; maximum is 65535
# define ON 2000 //Illumination time for LEDs
volatile uint8_t RxData;

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;   //Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //Unlocks GPIO pins at power-up
    //P1.6  UCB0SDA with 10k pullup resistor
    //P1.7  UCB0SCL with 10k pullup resistor
    P1DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5;
    P1SEL1 |= BIT6 + BIT7; //Setup I2C on UCB0
    P1OUT &= ~BIT0; //Green LED off
    P4DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5 + BIT6 + BIT7;
    P4OUT &= ~BIT6; //Red LED off
    //Set ACLK
    CSCTL0 = CSKEY; //Password to unlock the clock registers
    //Default frequency ~ 10 kHz
    CSCTL2 |= SELA__VLOCLK;  //Set ACLK to VLO
    CSCTL0_H = 0xFF; //Re-lock the clock registers
    //Enable the timer interrupt, MC_1 to count up to TA0CCR0, Timer A set to ACLK (VLO)
    TA0CCTL0 = CCIE;
    TA0CTL |= MC_1 + TASSEL_1;
    // Configure the eUSCI_B0 module for I2C at 100 kHz
    UCB0CTLW0 |= UCSWRST;
    //Select SMCLK, Master, synchronous, I2C
    UCB0CTLW0 |=  UCSSEL__SMCLK + UCMST + UCSYNC + UCMODE_3;
    UCB0BRW = 10;  //Divide SMCLK by 10 to get ~100 kHz
    UCB0I2CSA = 0x77; // Address of FR2355 slave
    UCB0CTLW0 &= ~UCSWRST; // Clear reset

    _BIS_SR(GIE); //Enable global interrupts

    while(1)
    {
        TA0CCR0 = PERIOD; //Looping period with VLO
        LPM3;       //Wait in low power mode
        //Timeout occurred.
        UCB0IE |= UCRXIE0; //Enable I2C receive interrupt
        UCB0CTL1 |= UCTXSTT; //Wake up slave by setting start bit
        LPM0;   // Remain in LPM0 until data received
        while (UCB0CTL1 & UCTXSTP); // Ensure stop condition sent; data byte received
        if(RxData==0x03) P1OUT |= BIT0; //Green LED
        else P4OUT |= BIT6; //Red LED
        UCB0IE &= ~UCRXIE0; //Disable I2C interrupt
        TA0CCR0 = ON; //LED illumination time
        LPM3;
        P1OUT &= ~BIT0; //Turn off LEDs
        P4OUT &= ~BIT6;
    }
}
#pragma vector=TIMER0_A0_VECTOR
 __interrupt void timerfoo (void)
{
    LPM3_EXIT;
}

#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
    switch(__even_in_range(UCB0IV,30))
    {
        case 0: break;         // Vector 0: No interrupts
        case 2: break;         // Vector 2: ALIFG
        case 4: break;          // Vector 4: NACKIFG
        case 6: break;         // Vector 6: STTIFG
        case 8: break;         // Vector 8: STPIFG
        case 10: break;         // Vector 10: RXIFG3
        case 12: break;         // Vector 12: TXIFG3
        case 14: break;         // Vector 14: RXIFG2
        case 16: break;         // Vector 16: TXIFG2
        case 18: break;         // Vector 18: RXIFG1
        case 20: break;         // Vector 20: TXIFG1
        case 22:                 // Vector 22: RXIFG0
             RxData = UCB0RXBUF;                   // Get RX data
             LPM0_EXIT;             // Exit active CPU
             break;
        case 24: break;         // Vector 24: TXIFG0
        case 26: break;        // Vector 26: BCNTIFG
        case 28: break;         // Vector 28: clock low timeout
        case 30: break;         // Vector 30: 9th bit
        default: break;
    }
}
