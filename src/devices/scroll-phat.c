#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <fftw3.h>
#include "../pivumeter.h"

static int i2c = 0;

static unsigned int input_size;
static unsigned int output_size;

//static double* window;

static double* input_buffer;
static fftw_complex* output_buffer;
static fftw_plan plan;

// Thank you Daniel J for the pointers and borrowed code from https://github.com/daniel-j/unicorn-fft/

// from impulse. I have no idea how these numbers were generated
// https://github.com/ianhalpern/Impulse/blob/master/src/Impulse.c#L30
static const long fft_max[] = { 12317168L, 7693595L, 5863615L, 4082974L, 5836037L, 4550263L, 3377914L, 3085778L, 3636534L, 3751823L, 2660548L, 3313252L, 2698853L, 2186441L, 1697466L, 1960070L, 1286950L, 1252382L, 1313726L, 1140443L, 1345589L, 1269153L, 897605L, 900408L, 892528L, 587972L, 662925L, 668177L, 686784L, 656330L, 1580286L, 785491L, 761213L, 730185L, 851753L, 927848L, 891221L, 634291L, 833909L, 646617L, 804409L, 1015627L, 671714L, 813811L, 689614L, 727079L, 853936L, 819333L, 679111L, 730295L, 836287L, 1602396L, 990827L, 773609L, 733606L, 638993L, 604530L, 573002L, 634570L, 1015040L, 679452L, 672091L, 880370L, 1140558L, 1593324L, 686787L, 781368L, 605261L, 1190262L, 525205L, 393080L, 409546L, 436431L, 723744L, 765299L, 393927L, 322105L, 478074L, 458596L, 512763L, 381303L, 671156L, 1177206L, 476813L, 366285L, 436008L, 361763L, 252316L, 204433L, 291331L, 296950L, 329226L, 319209L, 258334L, 388701L, 543025L, 396709L, 296099L, 190213L, 167976L, 138928L, 116720L, 163538L, 331761L, 133932L, 187456L, 530630L, 131474L, 84888L, 82081L, 122379L, 82914L, 75510L, 62669L, 73492L, 68775L, 57121L, 94098L, 68262L, 68307L, 48801L, 46864L, 61480L, 46607L, 45974L, 45819L, 45306L, 45110L, 45175L, 44969L, 44615L, 44440L, 44066L, 43600L, 57117L, 43332L, 59980L, 55319L, 54385L, 81768L, 51165L, 54785L, 73248L, 52494L, 57252L, 61869L, 65900L, 75893L, 65152L, 108009L, 421578L, 152611L, 135307L, 254745L, 132834L, 169101L, 137571L, 141159L, 142151L, 211389L, 267869L, 367730L, 256726L, 185238L, 251197L, 204304L, 284443L, 258223L, 158730L, 228565L, 375950L, 294535L, 288708L, 351054L, 694353L, 477275L, 270576L, 426544L, 362456L, 441219L, 313264L, 300050L, 421051L, 414769L, 244296L, 292822L, 262203L, 418025L, 579471L, 418584L, 419449L, 405345L, 739170L, 488163L, 376361L, 339649L, 313814L, 430849L, 275287L, 382918L, 297214L, 286238L, 367684L, 303578L, 516246L, 654782L, 353370L, 417745L, 392892L, 418934L, 475608L, 284765L, 260639L, 288961L, 301438L, 301305L, 329190L, 252484L, 272364L, 261562L, 208419L, 203045L, 229716L, 191240L, 328251L, 267655L, 322116L, 509542L, 498288L, 341654L, 346341L, 451042L, 452194L, 467716L, 447635L, 644331L, 1231811L, 1181923L, 1043922L, 681166L, 1078456L, 1088757L, 1221378L, 1358397L, 1817252L, 1255182L, 1410357L, 2264454L, 1880361L, 1630934L, 1147988L, 1919954L, 1624734L, 1373554L, 1865118L, 2431931L, 2431931L };


static void clear_display(void){
	unsigned int n;
	for(n = 0; n < 11; n++){
		wiringPiI2CWriteReg8(i2c, 0x1 + n, 0b00000000);
	}
	wiringPiI2CWriteReg8(i2c, 0x0c, 0xff);
}

static void cleanup(void){
	fftw_destroy_plan(plan);
	free(input_buffer);
	fftw_free(output_buffer);
	clear_display();
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

    atexit(cleanup);
   
    
    input_size = 1024;
    output_size = (input_size/2 + 1);
    
    input_buffer = (double*)(fftw_malloc(input_size * sizeof(double)));
    output_buffer = (fftw_complex*)(fftw_malloc(output_size * sizeof(fftw_complex)));
    
    /*window = (double*)(fftw_malloc(input_size * sizeof(double)));
    
    unsigned int i;
    double bhtmp;
    for(i = 0; i < input_size; i++){
		bhtmp = (double)(2 * M_PI * i / input_size);
		window[i] = (double)(0.35875 - (0.48829*cos(bhtmp)) + (0.14128 * cos(2*bhtmp)) - (0.01168*cos(3*bhtmp)));
	}*/
    
    int flags = FFTW_ESTIMATE;
	plan = fftw_plan_dft_r2c_1d(input_size, input_buffer, output_buffer, flags);

    return 0;
}

static void update(int meter_level_l, int meter_level_r, snd_pcm_scope_ameter_t *level){
    int16_t *ptr_left, *ptr_right;
    snd_pcm_t *pcm = level->pcm;
    snd_pcm_sframes_t size;
    snd_pcm_uframes_t size1;
    snd_pcm_uframes_t offset, cont;
    
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
    
    unsigned int buffer_index = 0;
    
    ptr_left = snd_pcm_scope_s16_get_channel_buffer(level->s16, 0) + offset;
    ptr_right = snd_pcm_scope_s16_get_channel_buffer(level->s16, 1) + offset;
    
    unsigned int n;
    for (n = size1; n > 0; n--) {
        input_buffer[buffer_index] = *ptr_left;
        if(*ptr_right > input_buffer[buffer_index]){
			input_buffer[buffer_index] = *ptr_right;
		}
		//input_buffer[buffer_index] *= window[buffer_index];
		ptr_right++;
        ptr_left++;
        buffer_index++;
        if(buffer_index >= input_size) {break;}
    }
    
    fftw_execute(plan);
	
	unsigned int bins[11] = {0,0,0,0,0,0,0,0,0,0,0};
	unsigned int groupsize = 5;

	for(n = 0; n < 11; n++){
		unsigned int x;
		double y = 0;
		for(x = 0; x < groupsize; x++){
			
			double r = (double) sqrt( pow( output_buffer[ (groupsize*n) + x ][ 0 ], 2 ) + pow( output_buffer[ (groupsize*n) + x ][ 1 ], 2 ) ) /  (double)fft_max[ (groupsize*n) + x ];
			
			if(r < 0){r = 0;}
			if(r > 1.0){r = 1.0;}
			
			if(r > y){
			    y = r;
			}
		}
		bins[n] = (unsigned int)(y * 5);
		
		unsigned int display = (0b1111100000 >> bins[n]) & 0b00011111;
		
		if(level->bar_reverse == 1){
			display = ((0b0000011111 << bins[n]) >> 5) & 0b00011111;
		}
		
		wiringPiI2CWriteReg8(i2c, 0x1 + n, display);
		
	}
	
	wiringPiI2CWriteReg8(i2c, 0x0c, 0xff);
}

device scroll_phat(){
    struct device _scroll_phat;
    _scroll_phat.init = &init;
    _scroll_phat.update = &update;
    return _scroll_phat;
}
