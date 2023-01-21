#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "../pivumeter.h"

#define DAT 23
#define CLK 24
#define NUM_PIXELS 16

static unsigned int pixels[NUM_PIXELS] = {0,0,0,0,0,0,0,0};

static void set_pixel(unsigned char index, unsigned char r, unsigned char g, unsigned char b, unsigned char brightness){
    pixels[index] = (brightness & 31);
    pixels[index] <<= 5;
    pixels[index] |= r;
    pixels[index] <<= 8;
    pixels[index] |= g;
    pixels[index] <<= 8;
    pixels[index] |= b;
}

static void write_byte(unsigned char byte){
    int x;
    for(x = 0; x<8; x++){
        digitalWrite(DAT, byte & 0b10000000);
        digitalWrite(CLK, 1);
        byte <<= 1;
        asm("NOP");
        asm("NOP");
        asm("NOP");
        digitalWrite(CLK, 0);
    }
}

static void sof(void){
    int x;
    for(x = 0; x<4; x++){
        write_byte(0);
    }
}

static void eof(void){
    int x;
    for(x = 0; x<5; x++){
        write_byte(0);
    }
}

static void show(void){
    int x;
    sof();
    for(x = 0; x < NUM_PIXELS; x++){
        unsigned char r, g, b, brightness;
        brightness = (pixels[x] >> 22) & 31;
        r = (pixels[x] >> 16) & 255;
        g = (pixels[x] >> 8)  & 255;
        b = (pixels[x])       & 255;
        write_byte(0b11100000 | brightness);
        write_byte(b);
        write_byte(g);
        write_byte(r);
    }
    eof();
}
static void clear_display(void){
    int x;
    for(x = 0; x < NUM_PIXELS; x++){
        pixels[x] = 0;
    }
    show();
}

static int init(void){
    system("gpio export 23 output");
    system("gpio export 24 output");
    wiringPiSetupSys();

    clear_display();

    atexit(clear_display);

    return 0;
}

static void set_level(int meter_level, int brightness, int reverse, int vu_scale, int meter){
    int bar = meter_level;
    if(bar > vu_scale){
        bar = vu_scale;
    }
    int step = vu_scale / (NUM_PIXELS / 2);
    int offset = meter * 8;

    int led;
    for(led = 0; led < (NUM_PIXELS/2); led++){
        int val = 0, index = led;

        if(bar > step){
            val = step;
            bar -= step;
        }
        else if(bar > 0){
            val = bar;
            bar = 0;
        }

        /*
         *  0  1  2  3  4  5  6  7
         * 15 14 13 12 11 10  9  8
         *
         */

        if(reverse == 0 || meter == 1){
            index = 7 - led;
            if(reverse == 0 && meter == 1){
                index = led;
            }
        }

        val = (val * brightness) / step;

        set_pixel(offset + index, (int)(val*(led/7.0f)), (int)(val-(val*(led/7.0f))), 0, 16);
    }
}

static void update(int meter_level_l, int meter_level_r, snd_pcm_scope_ameter_t *level){
    int x;

    int brightness = level->led_brightness;
    int reverse = level->bar_reverse;
    int vu_scale = level->vu_scale;


    for(x = 0; x < NUM_PIXELS; x++){
        pixels[x] = 0;
    }

    if(reverse == 0){
        set_level(meter_level_l, brightness, reverse, vu_scale, 1);
        set_level(meter_level_r, brightness, reverse, vu_scale, 0);
    }
    else
    {
        set_level(meter_level_l, brightness, reverse, vu_scale, 0);
        set_level(meter_level_r, brightness, reverse, vu_scale, 1);
    }

    show();
}

static void phatbeat_close(void){
    clear_display();
    return;
}

device phat_beat(){
    struct device _phat_beat;
    _phat_beat.init = &init;
    _phat_beat.update = &update;
    _phat_beat.close = &phatbeat_close;
    return _phat_beat;
}
