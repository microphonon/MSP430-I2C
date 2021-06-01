/*  Send alternating string of 3 control bytes to slave on I2C. Data sent with button press on P1.1. 
    Slave runs in a timed loop. Code 123 stops loop, and 456 starts it. Green and red LEDs also flash 
    with alternating strings. This is the MASTER code on MSP430FR5969 Launchpad.
    P1.6  UCB0SDA with 10k pullup
    P1.7  UCB0SCL with 10k pullup
  */
#include <msp430.h> 
#include <stdio.h>
#include <stdint.h>

volatile uint8_t TxCount, Control_Byte, i;
volatile uint8_t *PTxData;   // Pointer to TX data
volatile uint8_t TxData[3], Msg1[]={1,2,3}, Msg2[]={4,5,6};

void main(void) {

    WDTCTL = WDTPW | WDTHOLD;   //Stop watchdog timer

    PM5CTL0 &= ~LOCKLPM5; //Unlocks GPIO pins at power-up
    P1DIR |= BIT0 + BIT2 + BIT3 + BIT4 + BIT5;
    P1SEL1 |= BIT6 + BIT7; //Setup I2C on UCB0
    P1OUT = BIT1; // Pull-up resistor on P1.1
    P1REN = BIT1; // Select pull-up mode for P1.1
    P1IES = BIT1; // P1.1 Hi/Lo edge
    P1IFG = 0;    // Clear all P1 interrupt flags
    P1IE = BIT1;  // P1.1 interrupt enabled
    P1OUT &= ~BIT0; //green LED off

    P4DIR |= BIT0 + BIT1 + BIT2 + BIT3 + BIT4 + BIT5 + BIT6 + BIT7;
    P4OUT &= ~BIT6; //red LED off

    // Configure the eUSCI_B0 module for I2C at 100 kHz
    UCB0CTLW0 |= UCSWRST;
    UCB0CTLW0 |=  UCSSEL__SMCLK + UCMST + UCTR + UCSYNC + UCMODE_3; //Select SMCLK, master, transmitter, synchronous, I2C
    UCB0BRW = 10;  //Divide SMCLK by 10 to get ~100 kHz
    UCB0I2CSA = 0x77; // FR2355 address
    UCB0CTLW0 &= ~UCSWRST; // Clear reset

    UCB0IE |= UCTXIE0; //Enable I2C transmission interrupt
    __enable_interrupt(); //Enable global interrupts.
    Control_Byte=0x01;

    while(1)
    {
        LPM4;       //Wait for pushbutton interrupt in low power mode
        Control_Byte ^= BIT0; //Toggle control byte
        if(Control_Byte == 0x01)
        {
            for(i=0;i<3;i++)TxData[i]=Msg1[i];
            P4OUT |= BIT6; //Red LED on
        }
        else
        {
            for(i=0;i<3;i++)TxData[i]=Msg2[i];
            P1OUT |= BIT0; //Green LED on
        }
        PTxData = (uint8_t *)TxData; //Set pointer to start of TX array
        TxCount = 3; //Send bytes to slave
        UCB0CTLW0 |= UCTXSTT; // Set to transmit and start
        LPM0;    // Remain in LPM0 until all data transmitted
        while (UCB0CTLW0 & UCTXSTP);  // Ensure stop condition got sent
        __delay_cycles(10000);
        P1OUT &= ~BIT0; //LEDs off
        P4OUT &= ~BIT6;
    }
}

#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void)
{
    switch(__even_in_range(UCB0IV,30))
    {
        case 0: break;         // Vector 0: No interrupts
        case 2: break;         // Vector 2: ALIFG
        case 4: break;         // Vector 4: NACKIFG
        case 6: break;         // Vector 6: STTIFG
        case 8: break;        // Vector 8: STPIFG
        case 10: break;         // Vector 10: RXIFG3
        case 12: break;         // Vector 12: TXIFG3
        case 14: break;         // Vector 14: RXIFG2
        case 16: break;         // Vector 16: TXIFG2
        case 18: break;         // Vector 18: RXIFG1
        case 20: break;         // Vector 20: TXIFG1
        case 22: break;         // Vector 22: RXIFG0
        case 24:                // Vector 24: TXIFG0
                        if (TxCount)      // Check if TX byte counter not empty
                            {
                                UCB0TXBUF = *PTxData++; // Load TX buffer
                                TxCount--;            // Decrement TX byte counter
                            }
                        else
                            {
                                UCB0CTL1 |= UCTXSTP; // I2C stop condition
                                UCB0IFG &= ~UCTXIFG0;  // Clear USCI_B0 TX int flag
                                LPM0_EXIT;      // Exit LPM0
                            }
                        break;
        case 26: break;        // Vector 26: BCNTIFG
        case 28: break;         // Vector 28: clock low timeout
        case 30: break;         // Vector 30: 9th bit
        default: break;
    }
}

#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
{
  P1IFG &= ~BIT1;  // Clear P1.1 IFG
  LPM4_EXIT;     // Exit LPM4
}




