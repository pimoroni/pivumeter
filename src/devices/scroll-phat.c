#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <fftw3.h>
#include "../pimeter.h"

static int i2c = 0;

static unsigned int input_size;
static unsigned int output_size;

double* input_buffer;
fftw_complex* output_buffer;
fftw_plan plan;

static void clear_display(void){
}

static int init(void){
    i2c = wiringPiI2CSetup(0x60);
    if(i2c == -1){
        fprintf(stderr, "Unable to connect to Scroll pHAT");
        return -1;
    }
    
    wiringPiI2CWriteReg8(i2c, 0x0, 0b00000011);
    wiringPiI2CWriteReg8(i2c, 0x19, 32);

    clear_display();

    atexit(clear_display);
   
    
    input_size = 1024;
    output_size = (input_size/2 + 1);
    
    input_buffer = (double*)(fftw_malloc(input_size * sizeof(double)));
    output_buffer = (fftw_complex*)(fftw_malloc(output_size * sizeof(fftw_complex)));
    
    int flags = FFTW_ESTIMATE;
	plan = fftw_plan_dft_r2c_1d(input_size, input_buffer, output_buffer, flags);

    return 0;
}

static void update(int meter_level_l, int meter_level_r, snd_pcm_scope_ameter_t *level){
    int16_t *ptr, *ptr2;
    snd_pcm_t *pcm = level->pcm;
    snd_pcm_sframes_t size;
    snd_pcm_uframes_t size1, size2;
    snd_pcm_uframes_t offset, cont;
    unsigned int channel = 0;
    unsigned int n;
    
    size = snd_pcm_meter_get_now(pcm) - level->old;
    if (size < 0){
        size += snd_pcm_meter_get_boundary(pcm);
    }

    offset = level->old % snd_pcm_meter_get_bufsize(pcm);
    cont = snd_pcm_meter_get_bufsize(pcm) - offset;
    size1 = size;
    if (size1 > cont){
        size1 = cont;
    }

    size2 = size - size1;
    
    unsigned int buffer_index = 0;
    
    ptr = snd_pcm_scope_s16_get_channel_buffer(level->s16, 0) + offset;
    ptr2 = snd_pcm_scope_s16_get_channel_buffer(level->s16, 1) + offset;
    for (n = size1; n > 0; n--) {
        //s = *ptr;
        input_buffer[buffer_index] = *ptr;
        if(*ptr2 > input_buffer[buffer_index]){
			input_buffer[buffer_index] = *ptr2;
		}
		ptr2++;
        ptr++;
        buffer_index++;
        if(buffer_index >= input_size) {break;}
    }
    
    fftw_execute(plan);

	//fprintf(stdout, "%f,", output_buffer[1][0]); // 1=86.12hz?
	//fprintf(stdout, "%f,", output_buffer[2][0]);
	
	//fprintf(stdout, "%f,", output_buffer[400][0]);
	
	unsigned int bins[11] = {0,0,0,0,0,0,0,0,0,0,0};
	unsigned int groupsize = 8; // 33 gives up to 16k hz
	for(n = 0; n < 11; n++){
		int x;
		double y = 0;
		for(x = 0; x < groupsize; x++){
			double r = output_buffer[(groupsize*n) + x][0];
			if(r < 0){r = 0;}
			y += r;
		}
		bins[n] = (unsigned int)(y/8.0f);
		
		int value = 0b00011111 >> (bins[n] / 1000);
		if(bins[n] > 30000){
			wiringPiI2CWriteReg8(i2c, 0x1 + n, 0b00011111);
		}
		else if(bins[n] > 24000){
			wiringPiI2CWriteReg8(i2c, 0x1 + n, 0b00011110);
		}
		else if(bins[n] > 18000){
			wiringPiI2CWriteReg8(i2c, 0x1 + n, 0b00011100);
		}
		else if(bins[n] > 12000){
			wiringPiI2CWriteReg8(i2c, 0x1 + n, 0b00011000);
		}
		else if(bins[n] > 6000){
			wiringPiI2CWriteReg8(i2c, 0x1 + n, 0b00010000);
		}
		else {
			wiringPiI2CWriteReg8(i2c, 0x1 + n, 0b00000000);
		}
	}
	
	wiringPiI2CWriteReg8(i2c, 0x0c, 0xff);
	
	//fprintf(stdout, "%d\n", bins[10]);
}

device scroll_phat(){
    struct device _scroll_phat;
    _scroll_phat.init = &init;
    _scroll_phat.update = &update;
    return _scroll_phat;
}
