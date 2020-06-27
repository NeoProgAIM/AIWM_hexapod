//  ***************************************************************************
/// @file    indication.c
/// @author  NeoProg
//  ***************************************************************************
#include "indication.h"
#include "stm32f373xc.h"
#include "system_monitor.h"
#include "systimer.h"
#include <stdint.h>
#include <stdbool.h>

#define LED_R_PIN                   (5) // PB5
#define LED_G_PIN                   (6) // PB6
#define LED_B_PIN                   (7) // PB7
#define BUZZER_PIN                  (0) // PC0

#define LED_TURN_OFF(pin)           ( GPIOB->BRR  = (uint32_t)(0x01 << ((pin) * 1)) )
#define LED_TURN_ON(pin)            ( GPIOB->BSRR = (uint32_t)(0x01 << ((pin) * 1)) )
#define BUZZER_TURN_OFF()           ( GPIOC->BRR  = (uint32_t)(0x01 << (BUZZER_PIN * 1)) )
#define BUZZER_TURN_ON()            ( GPIOC->BSRR = (uint32_t)(0x01 << (BUZZER_PIN * 1)) )


static void blink_red_led_with_buzzer(uint32_t period);
static void blink_red_led(uint32_t period);
static void blink_blue_led(uint32_t period);
//static void blink_green_led(uint32_t period);
static void blink_yellow_led(uint32_t period);


//  ***************************************************************************
/// @brief  Driver initialization
/// @param  none
/// @return none
//  ***************************************************************************
void indication_init(void) {
    
    // Front LED red pin (PB5): output mode, push-pull, high speed, no pull
    GPIOB->BRR      =  (0x01u << (LED_R_PIN * 1u));
    GPIOB->MODER   |=  (0x01u << (LED_R_PIN * 2u));
    GPIOB->OSPEEDR |=  (0x03u << (LED_R_PIN * 2u));
    GPIOB->PUPDR   &= ~(0x03u << (LED_R_PIN * 2u));
    
    // Front LED green pin (PB6): output mode, push-pull, high speed, no pull
    GPIOB->BRR      =  (0x01u << (LED_G_PIN * 1u));
    GPIOB->MODER   |=  (0x01u << (LED_G_PIN * 2u));
    GPIOB->OSPEEDR |=  (0x03u << (LED_G_PIN * 2u));
    GPIOB->PUPDR   &= ~(0x03u << (LED_G_PIN * 2u));
    
    // Front LED blue pin (PB7): output mode, push-pull, high speed, no pull
    GPIOB->BRR      =  (0x01u << (LED_B_PIN * 1u));
    GPIOB->MODER   |=  (0x01u << (LED_B_PIN * 2u));
    GPIOB->OSPEEDR |=  (0x03u << (LED_B_PIN * 2u));
    GPIOB->PUPDR   &= ~(0x03u << (LED_B_PIN * 2u));
    
    // Buzzer pin (PC0): output mode, push-pull, high speed, no pull
    GPIOC->BRR      =  (0x01u << (0 * 1u));
    GPIOC->MODER   |=  (0x01u << (0 * 2u));
    GPIOC->OSPEEDR |=  (0x03u << (0 * 2u));
    GPIOC->PUPDR   &= ~(0x03u << (0 * 2u));
    
    // Disable all
    LED_TURN_OFF(LED_R_PIN);
    LED_TURN_OFF(LED_G_PIN);
    LED_TURN_OFF(LED_B_PIN);
    BUZZER_TURN_OFF();
    
    // Buzzer
    BUZZER_TURN_ON();
    delay_ms(80);
    BUZZER_TURN_OFF();
}

//  ***************************************************************************
/// @brief  Driver process
/// @param  none
/// @return none
//  ***************************************************************************
void indication_process(void) {
    
    if (sysmon_is_error_set(SYSMON_ANY_ERROR) == false) {
        LED_TURN_OFF(LED_R_PIN);
        LED_TURN_ON(LED_G_PIN);
        LED_TURN_OFF(LED_B_PIN);
        BUZZER_TURN_OFF();
    }
    else {
        
        if (sysmon_is_error_set(SYSMON_VOLTAGE_ERROR) == true) {
             blink_red_led_with_buzzer(500);
        }
        else if (sysmon_is_error_set(SYSMON_FATAL_ERROR | SYSMON_CONFIG_ERROR | SYSMON_MEMORY_ERROR | SYSMON_MATH_ERROR) == true) {
             blink_red_led(100);
        }
        else if (sysmon_is_error_set(SYSMON_SYNC_ERROR | SYSMON_I2C_ERROR) == true) {
            blink_yellow_led(500);
        }
        else if (sysmon_is_error_set(SYSMON_CONN_LOST_ERROR) == true) {
             blink_blue_led(500);
        }
        else {
            blink_red_led_with_buzzer(100);
        }
    }
}





//  ***************************************************************************
/// @brief  Blink red LED with buzzer beep
/// @param  period: blink and buzzer beep switch time
/// @return none
//  ***************************************************************************
static void blink_red_led_with_buzzer(uint32_t period) {
    
    static uint32_t start_time = 0;
    static bool state = false;
    
    if (get_time_ms() - start_time > period) {
        
        if (state == false) {
            LED_TURN_ON(LED_R_PIN);
            BUZZER_TURN_ON();
        }
        else {
            LED_TURN_OFF(LED_R_PIN);
            BUZZER_TURN_OFF();
        }
        LED_TURN_OFF(LED_G_PIN);
        LED_TURN_OFF(LED_B_PIN);
        
        state = !state;
        start_time = get_time_ms();
    }
}

//  ***************************************************************************
/// @brief  Blink red LED
/// @param  period: blink and buzzer beep switch time
/// @return none
//  ***************************************************************************
static void blink_red_led(uint32_t period) {
    
    static uint32_t start_time = 0;
    static bool state = false;
    
    if (get_time_ms() - start_time > period) {
        
        if (state == false) {
            LED_TURN_ON(LED_R_PIN);
        }
        else {
            LED_TURN_OFF(LED_R_PIN);
        }
        LED_TURN_OFF(LED_G_PIN);
        LED_TURN_OFF(LED_B_PIN);
        BUZZER_TURN_OFF();
        
        state = !state;
        start_time = get_time_ms();
    }
}

//  ***************************************************************************
/// @brief  Blink yellow LED
/// @param  period: LED switch time
/// @return none
//  ***************************************************************************
static void blink_yellow_led(uint32_t period) {
    
    static uint32_t start_time = 0;
    static bool state = false;
    
    if (get_time_ms() - start_time > period) {
        
        if (state == false) {
            LED_TURN_OFF(LED_R_PIN);
            LED_TURN_OFF(LED_G_PIN);
        }
        else {
            LED_TURN_ON(LED_R_PIN);
            LED_TURN_ON(LED_G_PIN);
        }
        LED_TURN_OFF(LED_B_PIN);
        BUZZER_TURN_OFF();
        
        state = !state;
        start_time = get_time_ms();
    }
}

//  ***************************************************************************
/// @brief  Blink blue LED
/// @param  period: LED switch time
/// @return none
//  ***************************************************************************
static void blink_blue_led(uint32_t period) {
    
    static uint32_t start_time = 0;
    static bool state = false;
    
    if (get_time_ms() - start_time > period) {
        
        if (state == false) {
            LED_TURN_OFF(LED_B_PIN);
        }
        else {
            LED_TURN_ON(LED_B_PIN);
        }
        LED_TURN_OFF(LED_R_PIN);
        LED_TURN_OFF(LED_G_PIN);
        BUZZER_TURN_OFF();
        
        state = !state;
        start_time = get_time_ms();
    }
}

//  ***************************************************************************
/// @brief  Blink green LED
/// @param  period: LED switch time
/// @return none
//  ***************************************************************************
/*static void blink_green_led(uint32_t period) {
    
    static uint32_t start_time = 0;
    static bool state = false;
    
    if (get_time_ms() - start_time > period) {
        
        if (state == false) {
            LED_TURN_OFF(LED_G_PIN);
        }
        else {
            LED_TURN_ON(LED_G_PIN);
        }
        LED_TURN_OFF(LED_R_PIN);
        LED_TURN_OFF(LED_B_PIN);
        BUZZER_TURN_OFF();
        
        state = !state;
        start_time = get_time_ms();
    }
}*/