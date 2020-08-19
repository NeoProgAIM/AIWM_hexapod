//  ***************************************************************************
/// @file    swlp.c
/// @author  NeoProg
//  ***************************************************************************
#include "swlp.h"
#include "stm32f373xc.h"
#include "swlp_protocol.h"
#include "sequences_engine.h"
#include "indication.h"
#include "system_monitor.h"
#include <stdint.h>


static bool check_frame(const uint8_t* rx_buffer, uint32_t frame_size);
static uint16_t calculate_crc16(const uint8_t* frame, uint32_t size);


//  ***************************************************************************
/// @brief  SWLP driver initialization
/// @param  none
/// @return none
//  ***************************************************************************
void swlp_init(void) {

}

//  ***************************************************************************
/// @brief  Process received frame
/// @param  rx_buffer: received data
/// @param  frame_size: frame size
/// @param  tx_buffer: transmit data
/// @retval tx_buffer
/// @return bytes for transmit from tx_buffer
//  ***************************************************************************
uint32_t swlp_process_frame(const uint8_t* rx_buffer, uint32_t frame_size, uint8_t* tx_buffer) {

    // Check frame
    if (check_frame(rx_buffer, frame_size) == false) {
        return 0;
    }

    // Preparing
    const swlp_frame_t* swlp_rx_frame = (const swlp_frame_t*)rx_buffer;
    const swlp_command_payload_t* request = (const swlp_command_payload_t*)swlp_rx_frame->payload;

    swlp_frame_t* swlp_tx_frame = (swlp_frame_t*)tx_buffer;
    swlp_status_payload_t* response = (swlp_status_payload_t*)swlp_tx_frame->payload;

    // Process command
    response->command_status = SWLP_CMD_STATUS_OK;
    switch (request->command) {

        case SWLP_CMD_NONE:
            break;
        case SWLP_CMD_SELECT_SEQUENCE_UP:
            sequences_engine_select_sequence(SEQUENCE_UP, 0, 0);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_DOWN:
            sequences_engine_select_sequence(SEQUENCE_DOWN, 0, 0);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_DIRECT:
            sequences_engine_select_sequence(SEQUENCE_DIRECT, request->curvature, request->distance);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_REVERSE:
            sequences_engine_select_sequence(SEQUENCE_REVERSE, request->curvature, request->distance);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_UP_DOWN:
            sequences_engine_select_sequence(SEQUENCE_UP_DOWN, 0, 0);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_PUSH_PULL:
            sequences_engine_select_sequence(SEQUENCE_PUSH_PULL, 0, 0);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_ATTACK_LEFT:
            sequences_engine_select_sequence(SEQUENCE_ATTACK_LEFT, 0, 0);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_ATTACK_RIGHT:
            sequences_engine_select_sequence(SEQUENCE_ATTACK_RIGHT, 0, 0);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_DANCE:
            sequences_engine_select_sequence(SEQUENCE_DANCE, 0, 0);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_ROTATE_X:
            sequences_engine_select_sequence(SEQUENCE_ROTATE_X, 0, 0);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_ROTATE_Z:
            sequences_engine_select_sequence(SEQUENCE_ROTATE_Z, 0, 0);
            break;
        case SWLP_CMD_SELECT_SEQUENCE_NONE:
            sequences_engine_select_sequence(SEQUENCE_NONE, 0, 0);
            break;
            
        default:
            response->command_status = SWLP_CMD_STATUS_ERROR;
    }

    // Prepare status payload
    response->command = request->command;
    response->module_status = sysmon_module_status;
    response->system_status = sysmon_system_status;
    response->battery_voltage = sysmon_battery_voltage;
    response->battery_charge = sysmon_battery_charge;

    // Prepare response
    swlp_tx_frame->start_mark = SWLP_START_MARK_VALUE;
    swlp_tx_frame->crc16 = calculate_crc16((uint8_t*)swlp_tx_frame, sizeof(swlp_frame_t) - 2);

    return sizeof(swlp_frame_t);
}





//  ***************************************************************************
/// @brief  Check SWP frame
/// @param  rx_buffer: frame
/// @param  frame_size: frame size
/// @return true - frame valid, false - frame invalid
//  ***************************************************************************
static bool check_frame(const uint8_t* rx_buffer, uint32_t frame_size) {

    // Check frame size
    if (frame_size != sizeof(swlp_frame_t)) {
        return false;
    }

    // Check frame CRC16
    uint16_t crc = calculate_crc16(rx_buffer, frame_size - 2);
    if (calculate_crc16(rx_buffer, frame_size) != 0) {
        return false;
    }

    // Check start mark
    const swlp_frame_t* swlp_frame = (const swlp_frame_t*)rx_buffer;
    if (swlp_frame->start_mark != SWLP_START_MARK_VALUE) {
        return false;
    }

    return true;
}

//  ***************************************************************************
/// @brief  Calculate frame CRC16
/// @param  frame: frame
/// @param  size: frame size
/// @return CRC16 value
//  ***************************************************************************
static uint16_t calculate_crc16(const uint8_t* frame, uint32_t size) {

    uint16_t crc16 = 0xFFFF;
    uint16_t data = 0;
    uint16_t k = 0;

    while (size--) {
        crc16 ^= *frame++;
        k = 8;
        while (k--) {
            data = crc16;
            crc16 >>= 1;
            if (data & 0x0001) {
                crc16 ^= SWLP_CRC16_POLYNOM;
            }
        }
    }
    return crc16;
}