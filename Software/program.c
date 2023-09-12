#include <mega32.h>
#include <alcd.h>
#include <delay.h>

int cooler = 0, heater = 0;
float step = 22.5;

unsigned char int_to_char(int x) {
	return x + '0';
}

void temp_on_lcd(int value, int c) {
	unsigned int r = 0;
	unsigned char tens = int_to_char(value / 10);
	unsigned char ones = int_to_char(value % 10);
	lcd_gotoxy(c, r);
	lcd_putchar(tens);
	lcd_gotoxy(c + 1, r);
	lcd_putchar(ones);
}

void status_on_lcd(int cooler, int heater) {
	lcd_gotoxy(2,1);  
	if (!cooler) lcd_putsf("Cooler--> OFF");   
	else lcd_putsf("Cooler--> ON "); 
	 
    lcd_gotoxy(2,2);  
    if (!heater) lcd_putsf("Heater--> OFF");   
	else lcd_putsf("Heater--> ON ");  
}

// Voltage Reference: AVCC pin
#define ADC_VREF_TYPE ((0<<REFS1) | (1<<REFS0) | (0<<ADLAR))

// Read the AD conversion result
unsigned int read_adc(unsigned char adc_input) {
	ADMUX = adc_input | ADC_VREF_TYPE;
	// Delay needed for the stabilization of the ADC input voltage
	delay_us(10);
    // Start the AD conversion
    ADCSRA |= (1 << ADSC);
    // Wait for the AD conversion to complete
    while ((ADCSRA & (1 << ADIF)) == 0);
    ADCSRA |= (1 << ADIF);
    return ADCW;
}

float computeDelay(float RPS) {
    return ((1 / RPS) / (4 * (90 / step))) * 1000; // seconds to miliseconds    
}

void main(void) { 
    int temperature;                                        
    unsigned int CRS = 4; // initial state: S1
    float coolerDelay = computeDelay(CRS);    
    int i = 0; 
                      
    // ADC initialization
    // ADC Clock frequency: 250.000 kHz
    // ADC Voltage Reference: AVCC pin
    // ADC Auto Trigger Source: ADC Stopped
    ADMUX |= (1<<MUX0);
    //ADMUX=ADC_VREF_TYPE;
    ADCSRA=(1<<ADEN) | (0<<ADSC) | (0<<ADATE) | (0<<ADIF) | (0<<ADIE) | (1<<ADPS2) | (0<<ADPS1) | (1<<ADPS0);
    SFIOR=(0<<ADTS2) | (0<<ADTS1) | (0<<ADTS0);
    
    DDRD=0xFF;
	PORTD=0x00;
	
    lcd_init(16);
    
    lcd_gotoxy(2,0);
    lcd_puts("Temp :");
    
    lcd_gotoxy(12, 0);
    lcd_putchar(223);
    
    lcd_gotoxy(13, 0);
    lcd_puts("C");
    
    delay_ms(100);   
    
    while (1) {
        unsigned delayInput = 35;
    	
        temperature = read_adc(0);
        temperature = temperature * 0.48898;
        temp_on_lcd(temperature, 9);
        

		if(!cooler && !heater && temperature < 15) heater = 1;     

		else if(!cooler && !heater && temperature > 35) {cooler = 1; CRS = 4;}     
        
    	else if(cooler && !heater && temperature < 25) cooler = 0;   
       
        else if(!cooler && heater && temperature > 30) heater = 0; 
        
        status_on_lcd(cooler, heater);

        /* S1 -> CRS = 4
           S2 -> CRS = 6
           S3 -> CRS = 8
        */


        if (CRS == 4 && temperature > 40) { // S1 -> S2
            CRS = 6;
        } else if (CRS == 6 && temperature < 35) { // S2 -> S1
            CRS = 4;        
        } else if (CRS == 6 && temperature > 45) { // S2 -> S3
            CRS = 8;        
        } else if (CRS == 8 && temperature < 40) { // S3 -> S2
            CRS = 6;
        }
        
        coolerDelay = computeDelay(CRS / ((float) 5));
        if (cooler) delayInput = coolerDelay;
        
        for (i = 0; i < 90 / step; i++) {
            PORTD = 0; // reset port
            if (cooler) PORTD |= 9;
            if (heater) PORTD |= 144; // 9 << 4  
            delay_ms(delayInput);
            if (cooler) {PORTD &= 0xF0; PORTD |= 12;}
            if (heater) {PORTD &= 0x0F; PORTD |= 192; } // 12 << 4
            delay_ms(delayInput);                                       
            if (cooler) {PORTD &= 0xF0; PORTD |= 6;}
            if (heater) {PORTD &= 0x0F; PORTD |= 96; } // 6 << 4 
            delay_ms(delayInput); 
            if (cooler) {PORTD &= 0xF0; PORTD |= 3;}
            if (heater) {PORTD &= 0x0F; PORTD |= 48; } // 3 << 4  
            delay_ms(delayInput);        
        }    
    }
}
