#include "lcd/lcd.h"
#include "utils.h"
#include "game.h"

/* Configure the joystick / buttons exactly as the lab template does. */
static void Inp_init(void) {
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOC);
    gpio_init(GPIOA, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ,
              GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3);
    gpio_init(GPIOC, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ,
              GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);
}

static void IO_init(void) {
    Inp_init();
    Lcd_Init();
    BACK_COLOR = BLACK;
    LCD_Clear(BLACK);
}

int main(void) {
    IO_init();

    for (;;) {
        int level = menu_loop();          /* difficulty / level select    */
        int score = play_level(level);    /* play until death or SW1      */
        if (score >= 0)                   /* died -> settle + scoreboard   */
            death_screen(level, score);
        /* score < 0 means SW1 forced-exit: straight back to the menu      */
    }
}
