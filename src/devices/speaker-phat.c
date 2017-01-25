#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "../pimeter.h"

static int stupid_led_mappings[10] = {0, 1, 2, 4, 6, 8, 10, 12, 14, 16};
static int i2c = 0;

static void clear_display(void){
  int led;

  for(led = 0; led < 18; led++){
      wiringPiI2CWriteReg8(i2c, 0x01 + led, 0x0);
  }

  wiringPiI2CWriteReg8(i2c, 0x16, 0x01);
}

static int init(void){
	  i2c = wiringPiI2CSetup(0x54);
  if(i2c == -1){
    fprintf(stderr, "Unable to connect to Speaker pHAT");
    return -1;
  }

  wiringPiI2CWriteReg8(i2c, 0x00, 0x01);
  wiringPiI2CWriteReg8(i2c, 0x13, 0xff);
  wiringPiI2CWriteReg8(i2c, 0x14, 0xff);
  wiringPiI2CWriteReg8(i2c, 0x15, 0xff);

  clear_display();

  atexit(clear_display);
  
  return 0;
}

static void update(int meter_level, snd_pcm_scope_ameter_t *level){
	
	int brightness = level->led_brightness;
    int bar = (meter_level / 10000.0f) * (brightness * 10.0f);

    if(bar < 0) bar = 0;
    if(bar > (brightness*10)) bar = (brightness*10);

    int led;
    for(led = 0; led < 10; led++){
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
		   index = 9 - led;
	   }
       
       wiringPiI2CWriteReg8(i2c, 0x01 + stupid_led_mappings[index], val);
    }
    wiringPiI2CWriteReg8(i2c, 0x16, 0x01);

}

device speaker_phat(){
	struct device _speaker_phat;
	_speaker_phat.init = &init;
	_speaker_phat.update = &update;
	return _speaker_phat;
}
