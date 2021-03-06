/*
 *      2013 Elia Ritterbusch
 *      http://eliaselectronics.com
 *
 *      This work is licensed under the Creative Commons Attribution 3.0 Unported License.
 *      To view a copy of this license, visit http://creativecommons.org/licenses/by/3.0/
 */
#define _XTAL_FREQ 16000000UL
#include <htc.h>
#include <stdlib.h>
#include "spi.h"
#include "nRF24L01.h"
#include "wl_module.h"

#define MAX_STRLEN 14
char received_string[MAX_STRLEN+1];

void _delay_10ms(int length){
    while(length){
        __delay_ms(10);
        length--;
    }
}

int main(void){
    unsigned char payload[wl_module_PAYLOAD]; //Array for Payload
    unsigned char maincounter =0;
    unsigned char k;

    // status pin to indicate when master
    // sends a message only for debugging 
    TRISDbits.TRISD4 = 0;
    LATDbits.LATD4 = 1;

    wl_module_init();	//initialise nRF24L01+ Module
    __delay_ms(50);	//wait for nRF24L01+ Module

    INTCONbits.PEIE = 1; // peripheral interrupts enabled
    INTCONbits.GIE = 1;  // global interrupt enable

    wl_module_tx_config(wl_module_TX_NR_0); //Config Module

    while(1){
        LATDbits.LATD4 = 0; // turn indicator LED on
        for (k=0; k<=wl_module_PAYLOAD-1; k++){
            payload[k] = k;
	}

	payload[0] = maincounter;
	payload[1] = maincounter+1;

	wl_module_send(payload,wl_module_PAYLOAD);

	maincounter++;
	if (maincounter > 0x0F){
            maincounter = 0;
        }
        __delay_ms(500);
        LATDbits.LATD4 = 1; // turn indicator LED off
        __delay_ms(500);
    }

    return 0;
}

void interrupt ISR(void){
    // external interrupt IRQ pin of NRF24L01
    if( INTCON3bits.INT2IF ){
        unsigned char status;

        // Read wl_module status
        wl_module_CSN_lo; // Pull down chip select
        status = spi_fast_shift(NOOP); // Read status register
        wl_module_CSN_hi; // Pull up chip select


        if (status & (1<<TX_DS)){ // IRQ: Package has been sent
                    wl_module_config_register(STATUS, (1<<TX_DS)); //Clear Interrupt Bit
                    PTX=0;
        }

	if (status & (1<<MAX_RT)){ // IRQ: Package has not been sent, send again
		wl_module_config_register(STATUS, (1<<MAX_RT));	// Clear Interrupt Bit
		wl_module_CE_hi; // Start transmission
		__delay_us(10);
		wl_module_CE_lo;
	}

	if (status & (1<<TX_FULL)){ //TX_FIFO Full <-- this is not an IRQ
		wl_module_CSN_lo; // Pull down chip select
		spi_fast_shift(FLUSH_TX); // Flush TX-FIFO
		wl_module_CSN_hi; // Pull up chip select
	}
        // reset INT2 flag
        INTCON3bits.INT2IF = 0;
    }
}


