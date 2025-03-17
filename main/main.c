#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"


const int TRIGGER = 15;
const int ECHO = 14;

volatile int alarme_disparou;

int64_t alarm_callback(alarm_id_t id, void *user_data) {
    alarme_disparou = 1;
    // Can return a value here in us to fire in the future
    return 0;
}

void init_all(){
    gpio_init(TRIGGER);
    gpio_set_dir(TRIGGER, GPIO_OUT);
    gpio_init(ECHO);
    gpio_set_dir(ECHO, GPIO_IN);
    datetime_t t = {
        .year = 2025,
        .month = 3,
        .day = 12,
        .dotw = 3,
        .hour = 17,
        .min = 0,
        .sec = 0
    };
    rtc_init();
    rtc_set_datetime(&t);
}

void send_pulse(){
    gpio_put(TRIGGER, 1);
    sleep_us(10);
    gpio_put(TRIGGER, 0);
}

double get_distance(){
    send_pulse();
    while(gpio_get(ECHO) == 0){
        if(alarme_disparou){
            return -1;
        }
    };
    absolute_time_t start = get_absolute_time();
    while(gpio_get(ECHO) == 1);
    absolute_time_t end = get_absolute_time();
    double duration = absolute_time_diff_us(start, end);
    return (duration * 0.0343) / 2;
}

int main() {
    stdio_init_all();
    init_all();
    int rodar = 0;
    int alarm_id;
    int alarme_ativo = 0;
    alarme_disparou = 0;

    while(1){
        char letra = getchar_timeout_us(1000000);
        if(letra=='s'){
            rodar = 1;
        }else if(letra=='p'){
            rodar = 0;
        }
        if(rodar){
            if(!alarme_ativo){
                alarme_ativo = 1;
                alarme_disparou = 0;
                alarm_id = add_alarm_in_ms(5000, &alarm_callback, NULL, true);
            }
            double distance = get_distance();
            datetime_t current_time;
            rtc_get_datetime(&current_time);
            char datetime_buf[256];
            char *datetime_str = &datetime_buf[0];
            datetime_to_str(datetime_str, sizeof(datetime_buf), &current_time);
            if(alarme_disparou && distance == -1){
                printf("%s -- Distance: Fail \n", datetime_str);
            }else{
                printf("%s -- Distance: %f \n", datetime_str, distance);
            }
            if(alarme_ativo){
                alarme_ativo = 0;
                alarme_disparou = 0;
                cancel_alarm(alarm_id);
            }
            sleep_ms(500);
        }
        }
    }