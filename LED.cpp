#define LOG_OUT 1 
#define FFT_N 256
//#define DEBUG
 
#include <FFT.h>
#include <LedControl.h>
 
#define SIZE_WIDTH  8
#define SIZE_HEIGHT 8
#define MAX_SPECTRUM 32
#define GAIN 2.3
#define FREQUENCY_INDEX(I) ((I) * 3 + 10)
 
#define SET_SPECTRUMS() \
    do { \
        for(int i = 0; i < SIZE_WIDTH; i++) { \
            g_lc.setRow(0, i, g_spectrums[i]); \
        } \
    }while(0)
     
#define GET_SPECTRUM(VAL) 0xff << ((8 - ((VAL) > MAX_SPECTRUM ? (VAL) : ((VAL) < 0 ? 0 : (VAL))) / 8))
 
LedControl g_lc = LedControl(7, 5, 6, 1); //LOAD, CS, CLK
 
uint8_t g_spectrums[SIZE_WIDTH] = { 0 };
uint8_t g_fft_init_log[FFT_N << 1] = { 0 };
 
bool g_first_flag = true;
 
 
void setup()
{
    #ifdef DEBUG
    Serial.begin(115200);
    #endif
 
    TIMSK0 = 0;     // turn off timer0 for lower jitter
    ADCSRA = 0xe5;  // set the adc to free running mode
    ADMUX  = 0x40;  // use adc0
    DIDR0  = 0x01;  // turn off the digital input for adc0
     
    g_lc.shutdown(0, false);
    g_lc.setIntensity(0, 8);
    g_lc.clearDisplay(0);
 
}
 
void loop()
{
    while(1) {
        cli();
        for (int i = 0 ; i < 256 ; i += 2) {
            while(!(ADCSRA & 0x10));
            ADCSRA = 0xf5;
            byte m = ADCL;
            byte j = ADCH;
            int k = (j << 8) | m;
            k -= 0x0200;
            k <<= 6;
            fft_input[i] = k;
            fft_input[i+1] = 0;
        }
        fft_window();
        fft_reorder();
        fft_run();
        fft_mag_log();
        sei();
        if(g_first_flag) {
            g_first_flag = false;
            memcpy(g_fft_init_log, fft_log_out, sizeof(g_fft_init_log));
        }
        else {
            for(int i =0; i < sizeof(g_spectrums); i++) {
                int j = FREQUENCY_INDEX(i);
                int v = (int) (GAIN * (fft_log_out[j] - g_fft_init_log[j]));
                g_spectrums[i] = GET_SPECTRUM(v);
            }
            SET_SPECTRUMS();
        }
        #ifdef DEBUG
        Serial.write(255);
        Serial.write(fft_log_out, 128);
        #endif
    }
}
