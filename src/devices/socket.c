#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/socket.h>
#include <linux/un.h>
#include <netinet/in.h>

#include "../pivumeter.h"

#define SOCKET_PATH "/tmp/pivumeter.socket"

static struct sockaddr_un address;
static int socket_fd, connection_fd;
static socklen_t address_length;

static void clean_up(void){
    close(socket_fd);
}

static int init(void){
    atexit(clean_up);

    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0){
        printf("socket() failed\n");
        return 1;
    }

    //unlink(SOCKET_PATH);
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    snprintf(address.sun_path, UNIX_PATH_MAX, SOCKET_PATH);

    /*if (bind(socket_fd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0) {
        printf("bind() failed\n");
        return 1;
    }*/

    /*if(listen(socket_fd, 5) != 0) {
        printf("listen() failed\n");
        return 1;
    }*/

    if(connect(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        printf("connect() failed\n");
        return 1;
    }

    return 0;
}

static void _close(void){
    printf("Closing socket\n");
    close(socket_fd);
}

static void update(int meter_level_l, int meter_level_r, snd_pcm_scope_ameter_t *level){

    //int brightness = level->led_brightness;
    //int reverse = level->bar_reverse;
    //int vu_scale = level->vu_scale;

    //if((connection_fd = accept(socket_fd, (struct sockaddr *) &address, &address_length)) > -1){

    int data[20];
    memset(&data, 0, sizeof(int) * 20);
    data[0] = meter_level_l;
    data[1] = meter_level_r;

    write(socket_fd, (const void *) & data, sizeof(int) * 20);
    //write(socket_fd, (const void *) & meter_level_l, sizeof(int));
    //write(socket_fd, (const void *) & meter_level_r, sizeof(int));
}

device pivumeter_socket(){
    struct device _pivumeter_socket;
    _pivumeter_socket.init = &init;
    _pivumeter_socket.update = &update;
    _pivumeter_socket.close = &_close;
    return _pivumeter_socket;
}
