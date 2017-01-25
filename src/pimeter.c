/*
 *   pimeter :  level meter ALSA plugin for Raspberry Pi HATs and pHATs
 *   Copyright (c) 2017 by Phil Howard <phil@pimoroni.com>
 *
 *   Derived from:
 *   ameter :  level meter ALSA plugin with SDL display
 *   Copyright (c) 2005 by Laurent Georget <laugeo@free.fr>
 *   Copyright (c) 2001 by Abramo Bagnara <abramo@alsa-project.org>
 *   Copyright (c) 2002 by Steve Harris <steve@plugin.org.uk>
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "pimeter.h"
#include <alsa/asoundlib.h>

#ifdef WITH_DEVICE_BLINKT
#include "devices/blinkt.h"
#endif
#ifdef WITH_DEVICE_SPEAKER_PHAT
#include "devices/speaker-phat.h"
#endif

#define BAR_WIDTH 70
/* milliseconds to go from 32767 to 0 */
#define DECAY_MS 1000
/* milliseconds for peak to disappear */
#define PEAK_MS 800

#define LED_BRIGHTNESS 255

#define MAX_METERS 2

struct device output_device;

int num_meters, num_scopes;
//unsigned int led_brightness = 255;

static int level_enable(snd_pcm_scope_t * scope)
{

  snd_pcm_scope_ameter_t *level =
    snd_pcm_scope_get_callback_private(scope);
  level->channels =
    calloc(snd_pcm_meter_get_channels(level->pcm),
	   sizeof(*level->channels));
  if (!level->channels) {
    if (level) free(level); 
    return -ENOMEM;
  }

  snd_pcm_scope_set_callback_private(scope, level);
  return (0);

}

static void level_disable(snd_pcm_scope_t * scope)
{
  snd_pcm_scope_ameter_t *level =
    snd_pcm_scope_get_callback_private(scope);
 
  if(level->channels) free(level->channels);
}

static void level_close(snd_pcm_scope_t * scope)
{

  snd_pcm_scope_ameter_t *level =
    snd_pcm_scope_get_callback_private(scope);
  if (level) free(level); 
}

static void level_start(snd_pcm_scope_t * scope ATTRIBUTE_UNUSED)
{
  sigset_t s;
  sigemptyset(&s);
  sigaddset(&s, SIGINT);
  pthread_sigmask(SIG_BLOCK, &s, NULL); 
}

static void level_stop(snd_pcm_scope_t * scope)
{
}

static void level_update(snd_pcm_scope_t * scope)
{
  snd_pcm_scope_ameter_t *level = snd_pcm_scope_get_callback_private(scope);
  snd_pcm_t *pcm = level->pcm;
  snd_pcm_sframes_t size;
  snd_pcm_uframes_t size1, size2;
  snd_pcm_uframes_t offset, cont;
  unsigned int c, channels;
  unsigned int ms;
  int max_decay, max_decay_temp;

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
  ms = size * 1000 / snd_pcm_meter_get_rate(pcm);
  max_decay = 32768 * ms / level->decay_ms;

  /* max_decay_temp=max_decay; */
  channels = snd_pcm_meter_get_channels(pcm);

  int meter_level = 0; 

  for (c = 0; c < channels; c++) {
    int16_t *ptr;
    int s, lev = 0;
    snd_pcm_uframes_t n;
    snd_pcm_scope_ameter_channel_t *l;
    l = &level->channels[c];
    ptr = snd_pcm_scope_s16_get_channel_buffer(level->s16, c) + offset;

    for (n = size1; n > 0; n--) {
      s = *ptr;
      if (s < 0)
	s = -s;
      if (s > lev)
	lev = s;
      ptr++;
    }

    ptr = snd_pcm_scope_s16_get_channel_buffer(level->s16, c);
    for (n = size2; n > 0; n--) {
      s = *ptr;
      if (s < 0)
	s = -s;
      if (s > lev)
	lev = s;
      ptr++;
    }

    /* limit the decay */
    if (lev < l->levelchan) {	  
      /* make max_decay go lower with level */
      max_decay_temp =
	max_decay / (32767 / (l->levelchan));
      lev = l->levelchan - max_decay_temp;
      max_decay_temp = max_decay;
    }

    l->levelchan = lev;

    if(lev > meter_level){
        meter_level = lev;
    }
    
    l->previous= lev;  
  }
    
    //printf("Level: %d\n", meter_level);

  output_device.update(meter_level, level);

  level->old = snd_pcm_meter_get_now(pcm);

}

static void level_reset(snd_pcm_scope_t * scope)
{
  snd_pcm_scope_ameter_t *level = snd_pcm_scope_get_callback_private(scope);
  snd_pcm_t *pcm = level->pcm;
  memset(level->channels, 0, snd_pcm_meter_get_channels(pcm) * sizeof(*level->channels));
  level->old = snd_pcm_meter_get_now(pcm);
}

snd_pcm_scope_ops_t level_ops = {
  enable:level_enable,
  disable:level_disable,
  close:level_close,
  start:level_start,
  stop:level_stop,
  update:level_update,
  reset:level_reset,
};

int snd_pcm_scope_pimeter_open(snd_pcm_t * pcm,
                  const char *name,
			      unsigned int decay_ms,
			      unsigned int peak_ms,
                  unsigned int led_brightness,
                  unsigned int bar_reverse,
			      snd_pcm_scope_t ** scopep)
{
  snd_pcm_scope_t *scope, *s16;
  snd_pcm_scope_ameter_t *level;
  int err = snd_pcm_scope_malloc(&scope);
  if (err < 0){
    return err;
  }
  level = calloc(1, sizeof(*level));
  if (!level) {
    if (scope) free(scope);
    return -ENOMEM;
  }
  level->pcm = pcm;
  level->bar_reverse = bar_reverse;
  level->decay_ms = decay_ms;
  level->peak_ms = peak_ms;
  level->led_brightness = led_brightness;
  s16 = snd_pcm_meter_search_scope(pcm, "s16");
  if (!s16) {
    err = snd_pcm_scope_s16_open(pcm, "s16", &s16);
    if (err < 0) {
      if (scope) free(scope);
      if (level)free(level);
      return err;
    }
  }
  level->s16 = s16;
  snd_pcm_scope_set_ops(scope, &level_ops);
  snd_pcm_scope_set_callback_private(scope, level);
  if (name){
    snd_pcm_scope_set_name(scope, strdup(name));
  }
  snd_pcm_meter_add_scope(pcm, scope);
  *scopep = scope;
  return 0;
}

int _snd_pcm_scope_pimeter_open(snd_pcm_t * pcm, const char *name,
			       snd_config_t * root, snd_config_t * conf)
{
  snd_config_iterator_t i, next;
  snd_pcm_scope_t *scope;
  long decay_ms = -1, peak_ms = -1, led_brightness = -1, bar_reverse = -1;
  int err;

  num_meters = MAX_METERS;
  num_scopes = MAX_METERS;
  
  snd_config_for_each(i, next, conf) {
    snd_config_t *n = snd_config_iterator_entry(i);
    const char *id;
    if (snd_config_get_id(n, &id) < 0)
      continue;
    if (strcmp(id, "comment") == 0)
      continue;
    if (strcmp(id, "type") == 0)
      continue;
    /*if (strcmp(id, "bar_width") == 0) {
      err = snd_config_get_integer(n, &bar_width);
      if (err < 0) {
	SNDERR("Invalid type for %s", id);
	return -EINVAL;
      }
      continue;
    }*/
    if (strcmp(id, "bar_reverse") == 0) {
      err = snd_config_get_integer(n, &bar_reverse);
      if (err < 0) {
        SNDERR("Invalid type for %", id);
        return -EINVAL;
      }
      continue;
    }
    if (strcmp(id, "brightness") == 0) {
      err = snd_config_get_integer(n, &led_brightness);
      if (err < 0) {
        SNDERR("Invalid type for %", id);
        return -EINVAL;
      }
      continue;
    }
    if (strcmp(id, "decay_ms") == 0) {
      err = snd_config_get_integer(n, &decay_ms);
      if (err < 0) {
	SNDERR("Invalid type for %s", id);
	return -EINVAL;
      }
      continue;
    }
    if (strcmp(id, "peak_ms") == 0) {
      err = snd_config_get_integer(n, &peak_ms);
      if (err < 0) {
	SNDERR("Invalid type for %s", id);
	return -EINVAL;
      }
      continue;
    }
    SNDERR("Unknown field %s", id);
    return -EINVAL;
  }

  if (decay_ms < 0) {
    decay_ms = DECAY_MS;
  }
  if (peak_ms < 0) {
    peak_ms = PEAK_MS;
  }
  if (led_brightness < 0) {
    led_brightness = LED_BRIGHTNESS;
  }
 
  output_device = speaker_phat();
  output_device.init();

  return snd_pcm_scope_pimeter_open(pcm, name, 
                		   decay_ms,
				   peak_ms, 
				   led_brightness, 
				   bar_reverse,
				   &scope);

}
