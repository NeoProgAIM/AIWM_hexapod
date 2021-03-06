//  ***************************************************************************
/// @file    servo_driver.c
/// @author  NeoProg
//  ***************************************************************************
#include "servo_driver.h"
#include "project_base.h"
#include "configurator.h"
#include "cli.h"
#include "pwm.h"
#include "system_monitor.h"
#include "systimer.h"

#define SERVO_POWER_EN_PIN                      (2) // PC2
#define SERVO_TURN_POWER_ON()                   (GPIOC->BSRR |= 0x01u << SERVO_POWER_EN_PIN)
#define SERVO_TURN_POWER_OFF()                  (GPIOC->BRR  |= 0x01u << SERVO_POWER_EN_PIN)


#define SERVO_DS3218MG_270_ID                   (0x00)
#define       DS3218MG_MIN_PULSE_WIDTH          (500)
#define       DS3218MG_MAX_PULSE_WIDTH          (2500)
#define       DS3218MG_MAX_PHYSIC_ANGLE         (270)
#define       DS3218MG_LOGIC_ZERO               (DS3218MG_MAX_PHYSIC_ANGLE / 2)


typedef enum {
    OVERRIDE_LEVEL_NO,
    OVERRIDE_LEVEL_LOGIC_ANGLE,
    OVERRIDE_LEVEL_PHYSIC_ANGLE,
    OVERRIDE_LEVEL_PULSE_WIDTH
} override_level_t;

typedef struct {
    uint8_t  config;                        // Servo mode configuration
    int16_t  zero_trim;                     // Zero trim
                                   
    uint16_t logic_zero;                    // Logic zero
    uint16_t min_pulse_width;               // Min pulse width
    uint16_t max_pulse_width;               // Max pulse width
    uint16_t max_physic_angle;              // Max physic angle
} servo_config_t;

typedef struct {
    float logic_angle;
    float physic_angle;
    uint32_t pulse_width;
    
    override_level_t override_level;
    int32_t override_value;
} servo_info_t;


static servo_config_t servo_config_list[SUPPORT_SERVO_COUNT] = {0};
static servo_info_t   servo_info_list[SUPPORT_SERVO_COUNT] = {0};


static bool read_configuration(void);
static float calculate_physic_angle(float logic_angle, const servo_config_t* config);
static uint32_t convert_angle_to_pulse_width(float physic_angle, const servo_config_t* config);


//  ***************************************************************************
/// @brief  Servo driver initialization
/// @return none
//  ***************************************************************************
void servo_driver_init(void) {
    
    // Init servo power enable pin PC2): output mode, push-pull, high speed, no pull
    SERVO_TURN_POWER_OFF();
    GPIOC->MODER   |=  (0x01u << (SERVO_POWER_EN_PIN * 2u));
    GPIOC->OSPEEDR |=  (0x03u << (SERVO_POWER_EN_PIN * 2u));
    GPIOC->PUPDR   &= ~(0x03u << (SERVO_POWER_EN_PIN * 2u));
    
    if (read_configuration() == false) {
        sysmon_set_error(SYSMON_CONFIG_ERROR);
        sysmon_disable_module(SYSMON_MODULE_SERVO_DRIVER);
        return;
    }
    
    pwm_init();
}

//  ***************************************************************************
/// @brief  Start move servo to new angle
/// @param  ch:    servo channel
/// @param  angle: new angle
/// @return none
//  ***************************************************************************
void servo_driver_move(uint32_t ch, float angle) {

    if (ch >= SUPPORT_SERVO_COUNT) {
        sysmon_set_error(SYSMON_FATAL_ERROR);
        sysmon_disable_module(SYSMON_MODULE_SERVO_DRIVER);
        return;
    }
    
    // Set new logic angle
    servo_info_list[ch].logic_angle = angle;
}

//  ***************************************************************************
/// @brief  Servo driver power ON
//  ***************************************************************************
void servo_driver_power_on(void) {
    SERVO_TURN_POWER_ON();
    pwm_enable();
}

//  ***************************************************************************
/// @brief  Servo driver power OFF
/// @note   Call before system reset
//  ***************************************************************************
void servo_driver_power_off(void) {
    pwm_disable();
    SERVO_TURN_POWER_OFF();
}

//  ***************************************************************************
/// @brief  Servo driver process
/// @note   Call from main loop
//  ***************************************************************************
void servo_driver_process(void) {

    if (sysmon_is_module_disable(SYSMON_MODULE_SERVO_DRIVER) == true) {
        SERVO_TURN_POWER_OFF();
        pwm_disable();
        return;
    }
    
    
    static uint64_t prev_synchro_value = 0;
    if (synchro != prev_synchro_value) {

        // We skip PWM period - this is error and calculations have long time
        if (synchro - prev_synchro_value > 1 && prev_synchro_value != 0) {
            sysmon_set_error(SYSMON_SYNC_ERROR);
        }

        //
        // Calculate servo state and update PWM driver
        //
        pwm_set_shadow_buffer_lock_state(false);
        for (uint32_t i = 0; i < SUPPORT_SERVO_COUNT; ++i) {

            servo_info_t* info = &servo_info_list[i];

            // Override logic angle if need
            if (info->override_level == OVERRIDE_LEVEL_LOGIC_ANGLE) {
                info->logic_angle = info->override_value;
            }

            // Calculate physic angle
            info->physic_angle = calculate_physic_angle(info->logic_angle, &servo_config_list[i]);
            if (info->override_level == OVERRIDE_LEVEL_PHYSIC_ANGLE) {
                info->physic_angle = info->override_value;
            }

            // Calculate pulse width
            info->pulse_width = convert_angle_to_pulse_width(info->physic_angle, &servo_config_list[i]);
            if (info->override_level == OVERRIDE_LEVEL_PULSE_WIDTH) {
                info->pulse_width = info->override_value;
            }

            // Load pulse width
            pwm_set_width(i, info->pulse_width);
        }
        pwm_set_shadow_buffer_lock_state(false);

        prev_synchro_value = synchro;
    }
}

//  ***************************************************************************
/// @brief  CLI command process
/// @param  cmd: command string
/// @param  argv: argument list
/// @param  argc: arguments count
/// @param  response: response
/// @retval response
/// @return true - success, false - fail
//  ***************************************************************************
bool servo_driver_cli_command_process(const char* cmd, const char (*argv)[CLI_ARG_MAX_SIZE], uint32_t argc, char* response) {
    
    // Process commands without args
    if (strcmp(cmd, "power-off") == 0 && argc == 0) {
        servo_driver_power_off();
        return true;
    }
    else if (strcmp(cmd, "power-on") == 0 && argc == 0) {
        servo_driver_power_on();
        return true;
    }

    // Get servo index
    if (argc == 0) {
        strcpy(response, CLI_ERROR("Servo index is not found"));
        return false;
    }
    uint32_t servo_index = atoi(argv[0]);
    if (servo_index > SUPPORT_SERVO_COUNT) {
        strcpy(response, CLI_ERROR("Servo index is out of range"));
        return false;
    }
    
    // Get servo info
    servo_info_t* info = &servo_info_list[servo_index];
    
    //
    // Process command
    //
    if (strcmp(cmd, "status") == 0 && argc == 1) {

        sprintf(response, CLI_OK("servo status report")
                          CLI_OK("    - override level: %ld")
                          CLI_OK("    - override value: %ld")
                          CLI_OK("    - logic angle: %ld")
                          CLI_OK("    - physic angle: %ld")
                          CLI_OK("    - pulse width: %lu"),
                info->override_level, info->override_value,
                (int32_t)info->logic_angle, (int32_t)info->physic_angle, info->pulse_width);
    }
    else if (strcmp(cmd, "set-logic") == 0 && argc == 2) {
        info->override_level = OVERRIDE_LEVEL_LOGIC_ANGLE;
        info->override_value = atoi(argv[1]);
        sprintf(response, CLI_OK("[%lu] has new override level %lu"), servo_index, info->override_level);
    }
    else if (strcmp(cmd, "set-physic") == 0 && argc == 2) {
        info->override_level = OVERRIDE_LEVEL_LOGIC_ANGLE;
        info->override_value = atoi(argv[1]);
        sprintf(response, CLI_OK("[%lu] has new override level %lu"), servo_index, info->override_level);
    }
    else if (strcmp(cmd, "set-pulse") == 0 && argc == 2) {
        info->override_level = OVERRIDE_LEVEL_LOGIC_ANGLE;
        info->override_value = atoi(argv[1]);
        sprintf(response, CLI_OK("[%lu] has new override level %lu"), servo_index, info->override_level);
    }
    else if (strcmp(cmd, "zero") == 0 && argc == 1) {
        info->override_level = OVERRIDE_LEVEL_PHYSIC_ANGLE;
        info->override_value = (int32_t)calculate_physic_angle(0, &servo_config_list[servo_index]);
        sprintf(response, CLI_OK("[%lu] moved to zero"), servo_index);
    }
    else if (strcmp(cmd, "reset") == 0 && argc == 1) {
        info->override_level = OVERRIDE_LEVEL_NO;
        info->override_value = 0;
        sprintf(response, CLI_OK("[%lu] has new override level %lu"), servo_index, info->override_level);
    }
    else {
        strcpy(response, CLI_ERROR("Unknown command or format for servo driver"));
        return false;
    }
    return true;
}





//  ***************************************************************************
/// @brief  Read configuration
/// @param  none
/// @return true - read success, false - fail
//  ***************************************************************************
static bool read_configuration(void) {
    
    for (uint32_t servo_index = 0; servo_index < SUPPORT_SERVO_COUNT; ++servo_index) {

        servo_config_t* servo_config = &servo_config_list[servo_index];

        // Calculate start address of servo configuration
        uint32_t base_address = MM_SERVO_CONFIG_BLOCK_BASE_EE_ADDRESS + servo_index * MM_SERVO_CONFIG_BLOCK_SIZE;

        // Read servo configuration
        if (config_read_8(base_address + MM_SERVO_CONFIG_OFFSET, &servo_config->config) == false) return false;
        if (servo_config->config == 0xFF) {
            return false;
        }
        
        // Load servo information
        uint8_t servo_id = (servo_config->config & MM_SERVO_CONFIG_SERVO_TYPE_MASK) >> 4;
        if (servo_id == SERVO_DS3218MG_270_ID) {
            servo_config->logic_zero       = DS3218MG_LOGIC_ZERO;
            servo_config->min_pulse_width  = DS3218MG_MIN_PULSE_WIDTH;
            servo_config->max_pulse_width  = DS3218MG_MAX_PULSE_WIDTH;
            servo_config->max_physic_angle = DS3218MG_MAX_PHYSIC_ANGLE;
        }
        else {
            // Unknown servo type
            return false;
        }

        // Read servo zero trim
        if (config_read_16(base_address + MM_SERVO_ZERO_TRIM_OFFSET, (uint16_t*)&servo_config->zero_trim) == false) return false;
    }
    return true;
}

//  ***************************************************************************
/// @brief  Start move servo to new angle
/// @param  ch:    servo channel
/// @param  angle: new angle
/// @return physic angle
//  ***************************************************************************
static float calculate_physic_angle(float logic_angle, const servo_config_t* config) {

    // Convert logic angle to physic angle
    float physic_angle = 0;
    if (config->config & MM_SERVO_CONFIG_REVERSE_DIRECTION_MASK) {
        physic_angle = (float)config->logic_zero - (logic_angle + (float)config->zero_trim);
    }
    else {
        physic_angle = (float)config->logic_zero + (logic_angle + (float)config->zero_trim);
    }
    
    // Check physic angle value
    if (physic_angle < 0 || physic_angle > config->max_physic_angle) {
        sysmon_set_error(SYSMON_MATH_ERROR);
        sysmon_disable_module(SYSMON_MODULE_SERVO_DRIVER);
        return config->logic_zero;
    }

    return physic_angle;
}

//  ***************************************************************************
/// @brief  Convert servo angle to PWM pulse width
/// @param  physic_angle: servo angle
/// @param  config: servo configuration. @ref servo_config_t
/// @return PWM pulse width
//  ***************************************************************************
static uint32_t convert_angle_to_pulse_width(float physic_angle, const servo_config_t* config) {

    float step = (float)(config->max_pulse_width - config->min_pulse_width) / (float)config->max_physic_angle;
    float pulse_width = (float)config->min_pulse_width + physic_angle * step;
    
    // Check pulse width value
    if (pulse_width < config->min_pulse_width || pulse_width > config->max_pulse_width) {
        sysmon_set_error(SYSMON_MATH_ERROR);
        sysmon_disable_module(SYSMON_MODULE_SERVO_DRIVER);
        return config->min_pulse_width + (pulse_width > config->max_pulse_width - config->min_pulse_width) / 2;
    }
    
    return (uint32_t)pulse_width;
}
