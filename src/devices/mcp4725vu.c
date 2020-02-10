#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include "../pivumeter.h"

#include "mcp4725.h"

static void zero_display(void){
    analogWrite(100,0);
}

static int mcp4725vu_init(void){
    mcp4725Setup(100,MCP4725);
    zero_display();

    atexit(zero_display);

    return 0;
}

static void meter_update(int meter_level_l, int meter_level_r, snd_pcm_scope_ameter_t *level){
    int meter_level = meter_level_l;
    if(meter_level_r > meter_level){meter_level = meter_level_r;}

    analogWrite(100,((meter_level/32767.0f)*290));

}

device mcp4725vu(){
    struct device _mcp4725vu;
    _mcp4725vu.init = &mcp4725vu_init;
    _mcp4725vu.update = &meter_update;
    return _mcp4725vu;
}
