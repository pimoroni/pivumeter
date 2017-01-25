#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "../pimeter.h"

#define DAT 23
#define CLK 24
#define NUM_PIXELS 8

static unsigned int pixels[NUM_PIXELS] = {0,0,0,0,0,0,0,0};

static void blinkt_set_pixel(unsigned char index, unsigned char r, unsigned char g, unsigned char b, unsigned char brightness){
  pixels[index] = (brightness & 31);
  pixels[index] <<= 5;
  pixels[index] |= r;
  pixels[index] <<= 8;
  pixels[index] |= g;
  pixels[index] <<= 8;
  pixels[index] |= b;
}

static void blinkt_write_byte(unsigned char byte){
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

static void blinkt_sof(void){
  int x;
  for(x = 0; x<4; x++){
	  blinkt_write_byte(0);
  }
}

static void blinkt_eof(void){
  int x;
  for(x = 0; x<5; x++){
	  blinkt_write_byte(0);
  }
}

static void blinkt_show(void){
  int x;
  blinkt_sof();
  for(x = 0; x < NUM_PIXELS; x++){
    unsigned char r, g, b, brightness;
    brightness = (pixels[x] >> 22) & 31;
    r = (pixels[x] >> 16) & 255;
    g = (pixels[x] >> 8)  & 255;
    b = (pixels[x])       & 255;
    blinkt_write_byte(0b11100000 | brightness);
    blinkt_write_byte(b);
    blinkt_write_byte(g);
    blinkt_write_byte(r);
  }
  blinkt_eof();
}
static void blinkt_clear_display(void){
  int x;
  for(x = 0; x < NUM_PIXELS; x++){
	  pixels[x] = 0;
  }
  blinkt_show();
}

static int blinkt_init(void){
  system("gpio export 23 output");
  system("gpio export 24 output");
  wiringPiSetupSys();
  //pinMode(DAT, OUTPUT);
  //pinMode(CLK, OUTPUT);
 
  blinkt_clear_display();
  
  atexit(blinkt_clear_display);
  
  return 0;
}

static void blinkt_update(int meter_level, snd_pcm_scope_ameter_t *level){
  int x;
  for(x = 0; x < NUM_PIXELS; x++){
	  pixels[x] = 0;
  }
	
  int brightness = level->led_brightness;
  int bar = (meter_level / 15000.0f) * (brightness * NUM_PIXELS);

    if(bar < 0) bar = 0;
    if(bar > (brightness*NUM_PIXELS)) bar = (brightness*NUM_PIXELS);

    int led;
    for(led = 0; led < NUM_PIXELS; led++){
       int val = 0, index = led;

       if(bar > brightness){
           val = brightness;
           bar -= brightness;
       }
       else if(bar > 0){
       	   val = bar;
           bar = 0;
       }
       
       if(level->bar_reverse == 1){
		   index = 7 - led;
	   }
       
       blinkt_set_pixel(index, (int)(val*(led/7.0f)), (int)(val-(val*(led/7.0f))), 0, 16);
    }
    
  blinkt_show();
}

device blinkt(){
	struct device _blinkt;
	_blinkt.init = &blinkt_init;
	_blinkt.update = &blinkt_update;
	return _blinkt;
}
