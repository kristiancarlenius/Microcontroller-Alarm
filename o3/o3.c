#include "o3.h"
#include "gpio.h"
#include "systick.h"

typedef struct {
    int hours;
    int minutes;
    int seconds;
} timer_t; //definerer tid

typedef struct {
    volatile word CTRL;
    volatile word MODEL;
    volatile word MODEH;
    volatile word DOUT;
    volatile word DOUTSET;
    volatile word DOUTCLR;
    volatile word DOUTTGL;
    volatile word DIN;
    volatile word PINLOCKN;
} gpio_port_map_t; //gitt kode

typedef struct {
    volatile gpio_port_map_t ports[6];
    volatile word unused_space[10];
    volatile word EXTIPSELL;
    volatile word EXTIPSELH;
    volatile word EXTIRISE;
    volatile word EXTIFALL;
    volatile word IEN;
    volatile word IF;
    volatile word IFS;
    volatile word IFC;
    volatile word ROUTE;
    volatile word INSENSE;
    volatile word LOCK;
    volatile word CTRL;
    volatile word CMD;
    volatile word EM4WUEN;
    volatile word EM4WUPOL;
    volatile word EM4WUCAUSE;
} gpio_map_t; //gitt kode

typedef struct {
    volatile word CTRL;
    volatile word LOAD;
    volatile word VAL;
    volatile word CALIB;
} systick_t; //gitt kode

//alt her er bare å sette definisjoner på pins
#define LED0_PORT GPIO_PORT_E
#define LED0_PIN 2
#define PB0_PORT GPIO_PORT_B
#define PB0_PIN 9
#define PB1_PORT GPIO_PORT_B
#define PB1_PIN 10

#define STATE_SET_SECONDS 0
#define STATE_SET_MINUTES 1
#define STATE_SET_HOURS 2
#define STATE_COUNTDOWN 3
#define STATE_ALARM 4


#define MIN(l,r) (l<r?l:r)
#define MAX(l,r) (l>r?l:r)

void int_to_string(char *timestamp, unsigned int offset, int i) { //gitt funksjon
    if (i > 99) {
        timestamp[offset]   = '9';
        timestamp[offset+1] = '9';
        return;
    }

    while (i > 0) {
      if (i >= 10) {
        i -= 10;
        timestamp[offset]++;

      } else {
        timestamp[offset+1] = '0' + i;
        i=0;
      }
    }
}

void time_to_string(char *timestamp, int a, int b, int c) { //gitt funksjon
    timestamp[0] = '0';
    timestamp[1] = '0';
    timestamp[2] = '0';
    timestamp[3] = '0';
    timestamp[4] = '0';
    timestamp[5] = '0';
    timestamp[6] = '\0';

    int_to_string(timestamp, 0, a);
    int_to_string(timestamp, 2, b);
    int_to_string(timestamp, 4, c);
}

struct gpio_port_map_t {
    word CTRL;
    word MODEL;
    word MODEH;
    word DOUT;
    word DOUTSET;
    word DOUTCLR;
    word DOUTTGL;
    word DOUTIN;
    word PINLOCKN;
}; //vedlagt kode

volatile struct gpio_map_t {
    struct gpio_port_map_t ports[6];
    word unused_space[10], EXTIPSELL, EXTIPSELH, EXTIRISE, EXTIFALL, IEN, IF, IFS, IFC, ROUTE, INSENSE, LOCK, CTRL, CMD, EM4WUEN, EM4WUPOL, EM4WUCAUSE;
} *GPIO = (struct gpio_map_t*)GPIO_BASE; //definerer gipobase

volatile struct systick_t {
    word CTRL;
    word LOAD;
    word VAL;
    word CALIB;
} *SYSTICK = (struct systick_t*)SYSTICK_BASE; //her definerer vi bare systick

struct timer_t { int hours, minutes, seconds; }; //formateringen på nedtelleren

static struct timer_t cd_t = {0,0,0}; //her har vi nedtelleren

static int st = STATE_SET_SECONDS;

static char displayStr[8] = "0000000\0"; // setter display til null

void i_s() { //sekunder
    ++cd_t.seconds;
    if (cd_t.seconds >= 60) {
        cd_t.seconds = 0;

    }
}

void i_m() { //minutter
    ++cd_t.minutes;
    if (cd_t.minutes >= 60) {
        cd_t.minutes = 0;

    }

}

void i_h(){ //timer
    ++cd_t.hours;
    if (cd_t.hours >= 24) {
        cd_t.hours = 0;
    }
}


void u_dis() { //funksjonene for display
    time_to_string(displayStr, cd_t.hours, cd_t.minutes, cd_t.seconds);
    lcd_write(displayStr);
}


void cd_st() { //starte
  SYSTICK->VAL = SYSTICK->LOAD;
  SYSTICK->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

void cd_stop() { //stoppe
  SYSTICK->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);
}


void GPIO_ODD_IRQHandler() {
    switch (st) {
        case STATE_SET_SECONDS:
            i_s();//sekunder
            u_dis();//oppdaterer display
            break;
        case STATE_SET_MINUTES:
            i_m();//minutter
            u_dis();//oppdaterer display
            break;
        case STATE_SET_HOURS:
            i_h();//timer
            u_dis();//oppdaterer display
            break;
        case STATE_COUNTDOWN:
          break;
        case STATE_ALARM:
          break;
            GPIO->ports[LED0_PORT].DOUTSET = 1 << LED0_PIN;
    }
    GPIO->IFC = 1 << PB0_PIN;
}


void GPIO_EVEN_IRQHandler() {
    switch (st) {
        case STATE_SET_SECONDS://sekunder
          st = STATE_SET_MINUTES;
            break;
        case STATE_SET_MINUTES://minutter
          st = STATE_SET_HOURS;
            break;
        case STATE_SET_HOURS://timer
          st = STATE_COUNTDOWN;
            cd_st();
            break;
        case STATE_COUNTDOWN: break;

        case STATE_ALARM:
          st = STATE_SET_SECONDS;

            GPIO->ports[LED0_PORT].DOUTCLR = 1 << LED0_PIN;
            break;
    }
    GPIO->IFC = 1 << PB1_PIN;
}

void func(volatile word *a, int b, word c) {
  *a &= ~(0b1111 << (b * 4));
  *a |= c << (b*4);
}

void gpio_setup() {
  func(&GPIO->ports[LED0_PORT].MODEL, LED0_PIN, GPIO_MODE_OUTPUT);
  func(&GPIO->ports[PB0_PORT].MODEH, PB0_PIN-8, GPIO_MODE_INPUT);
  func(&GPIO->ports[PB1_PORT].MODEH, PB1_PIN-8, GPIO_MODE_INPUT);

  func(&GPIO->EXTIPSELH, PB0_PIN-8, 0b0001);
  func(&GPIO->EXTIPSELH, PB1_PIN-8, 0b0001);

  GPIO->EXTIFALL |= 1 << PB0_PIN;
  GPIO->EXTIFALL |= 1 << PB1_PIN;

  GPIO->IEN |= 1 << PB0_PIN;
  GPIO->IEN |= 1 << PB1_PIN;

  SYSTICK->LOAD = FREQUENCY;
  SYSTICK->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk;
}
void SysTick_Handler() {
  if (st == STATE_COUNTDOWN) {
    if (cd_t.seconds <= 0) {//sekunder
      if (cd_t.minutes <= 0) {//minutter
        if (cd_t.hours <= 0) {//timer
            st = STATE_ALARM;
            cd_stop();
          GPIO->ports[LED0_PORT].DOUTSET = 1 << LED0_PIN;
          u_dis(); //oppdaterer display
          return;
        }
        --cd_t.hours;//timer
        cd_t.minutes = 60;
      }
      --cd_t.minutes;//minutter
      cd_t.seconds = 60;
    }
    --cd_t.seconds; //sekunder
    u_dis(); //oppdaterer display
  }
}





void init2() { //kjører setup og "alt" samt
    gpio_setup();
    u_dis(); //oppdaterer display
}

int main(void) { //her kjøres selve programmet
    init(); //den innebygde funksjonen
    init2(); //kjører setup og derfor alt
    while (true);
    return 0;
}
