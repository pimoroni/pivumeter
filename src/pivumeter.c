/*
 *   pivumeter :  level meter ALSA plugin for Raspberry Pi HATs and pHATs
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
#include "pivumeter.h"
#include <alsa/asoundlib.h>

#include "devices/all.h"

/* milliseconds to go from 32767 to 0 */
#define DECAY_MS 200
/* milliseconds for peak to disappear */
#define PEAK_MS 200

/* Value against which the VU intensity is scaled */
#define VU_SCALE 32767

#define LED_BRIGHTNESS 255

#define MAX_METERS 2

struct device output_device;

int num_meters, num_scopes;

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

static int get_channel_level(int channel, snd_pcm_scope_ameter_t *level, snd_pcm_uframes_t offset, snd_pcm_uframes_t size1, snd_pcm_uframes_t size2,
        int max_decay, int max_decay_temp){
    int16_t *ptr;
    int s, lev = 0;
    snd_pcm_uframes_t n;
    snd_pcm_scope_ameter_channel_t *l;
    l = &level->channels[channel];


    // Iterate through the channel buffer and find the highest level value
    ptr = snd_pcm_scope_s16_get_channel_buffer(level->s16, channel) + offset;
    for (n = size1; n > 0; n--) {
        s = *ptr;
        if (s < 0) s = -s;
        if (s > lev) lev = s;
        ptr++;
    }

    ptr = snd_pcm_scope_s16_get_channel_buffer(level->s16, channel);
    for (n = size2; n > 0; n--) {
        s = *ptr;
        if (s < 0) s = -s;
        if (s > lev) lev = s;
        ptr++;
    }

    /* limit the decay */
    if (lev < l->levelchan) {
        /* make max_decay go lower with level */
        max_decay_temp = max_decay / (32767 / (l->levelchan));
        lev = l->levelchan - max_decay_temp;
        max_decay_temp = max_decay;
    }

    l->levelchan = lev;

    l->previous =lev;

    return lev;
}

static void level_update(snd_pcm_scope_t * scope)
{
    snd_pcm_scope_ameter_t *level = snd_pcm_scope_get_callback_private(scope);
    snd_pcm_t *pcm = level->pcm;
    snd_pcm_sframes_t size;
    snd_pcm_uframes_t size1, size2;
    snd_pcm_uframes_t offset, cont;
    unsigned int channels;
    unsigned int ms;
    int max_decay, max_decay_temp;

    int meter_level_l = 0;
    int meter_level_r = 0; 

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

    channels = snd_pcm_meter_get_channels(pcm);

    if(channels > 2){channels = 2;}

    meter_level_l = get_channel_level(0, level, offset, size1, size2, max_decay, max_decay_temp);
    meter_level_r = meter_level_l;
    if(channels > 1){
        meter_level_r = get_channel_level(1, level, offset, size1, size2, max_decay, max_decay_temp);
    }

    output_device.update(meter_level_l, meter_level_r, level);

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

int snd_pcm_scope_pivumeter_open(snd_pcm_t * pcm,
        const char *name,
        unsigned int decay_ms,
        unsigned int peak_ms,
        unsigned int led_brightness,
        unsigned int bar_reverse,
        unsigned int vu_scale,
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
    level->vu_scale = vu_scale;
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

int set_output_device(const char *output_device_name){
#ifdef WITH_DEVICE_SPEAKER_PHAT
    if(strcmp(output_device_name, "speaker-phat") == 0){
        fprintf(stderr, "Using device: speaker-phat\n");
        output_device = speaker_phat();
        return 0;
    }
#endif
#ifdef WITH_DEVICE_PHAT_BEAT
    if(strcmp(output_device_name, "phat-beat") == 0){
        fprintf(stderr, "Using device: phat-beat\n");
        output_device = phat_beat();
        return 0;
    }
#endif
#ifdef WITH_DEVICE_BLINKT
    if(strcmp(output_device_name, "blinkt") == 0){
        fprintf(stderr, "Using device: blinkt\n");
        output_device = blinkt();
        return 0;
    }
#endif
#ifdef WITH_DEVICE_SCROLL_PHAT
    if(strcmp(output_device_name, "scroll-phat") == 0){
        fprintf(stderr, "Using device: scroll-phat\n");
        output_device = scroll_phat();
        return 0;
    }
#endif
#ifdef WITH_DEVICE_SCROLL_PHAT_HD
    if(strcmp(output_device_name, "scroll-phat-hd") == 0){
        fprintf(stderr, "Using device: scroll-phat-hd\n");
        output_device = scroll_phat_hd();
        return 0;
    }
#endif
    return -1;
}

int _snd_pcm_scope_pivumeter_open(snd_pcm_t * pcm, const char *name,
        snd_config_t * root, snd_config_t * conf)
{
    snd_config_iterator_t i, next;
    snd_pcm_scope_t *scope;
    long decay_ms = -1, peak_ms = -1, led_brightness = -1, bar_reverse = -1, vu_scale = -1;
    const char *output_device_name = "";
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
        if (strcmp(id, "output_device") == 0){
            err = snd_config_get_string(n, &output_device_name);
            if (err < 0) {
                SNDERR("Invalid type for %", id);
                return -EINVAL;  
            }
            continue;
        }
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
        if (strcmp(id, "vu_scale") == 0) {
            err = snd_config_get_integer(n, &vu_scale);
            if (err < 0) {
                SNDERR("Invalid type for %s", id);
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

    if (bar_reverse < 0) {
        bar_reverse = 0;
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
    if (vu_scale < 0) {
        vu_scale = VU_SCALE;
    }


    if (strlen(output_device_name) == 0){
        fprintf(stderr, "No device specified, defaulting to speaker-phat!\n");
        output_device_name = "speaker-phat";
    }

    if(set_output_device(output_device_name) == -1){
        SNDERR("Invalid output device! %s", output_device_name);
        return -EINVAL;
    }

    output_device.init();

    return snd_pcm_scope_pivumeter_open(
            pcm,
            name,
            decay_ms,
            peak_ms,
            led_brightness,
            bar_reverse,
            vu_scale,
            &scope);

}
