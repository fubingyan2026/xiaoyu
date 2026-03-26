
#ifndef PS4_CONTROLLER_H
#define PS4_CONTROLLER_H

#include "stdint.h"
/********************************************************************************/
/*                                  T Y P E S */
/********************************************************************************/

/********************/
/*    A N A L O G   */
/********************/

typedef struct
{
    int8_t lx;
    int8_t ly;
    int8_t rx;
    int8_t ry;
} ps4_analog_stick_t;

typedef struct
{
    uint8_t l2;
    uint8_t r2;
} ps4_analog_button_t;

typedef struct
{
    ps4_analog_stick_t stick;
    ps4_analog_button_t button;
} ps4_analog_t;

/*********************/
/*   B U T T O N S   */
/*********************/

typedef struct
{
    uint8_t right : 1;
    uint8_t down : 1;
    uint8_t up : 1;
    uint8_t left : 1;

    uint8_t square : 1;
    uint8_t cross : 1;
    uint8_t circle : 1;
    uint8_t triangle : 1;

    uint8_t upright : 1;
    uint8_t downright : 1;
    uint8_t upleft : 1;
    uint8_t downleft : 1;

    uint8_t l1 : 1;
    uint8_t r1 : 1;
    uint8_t l2 : 1;
    uint8_t r2 : 1;

    uint8_t share : 1;
    uint8_t options : 1;
    uint8_t l3 : 1;
    uint8_t r3 : 1;

    uint8_t ps : 1;
    uint8_t touchpad : 1;
} ps4_button_t;

/*******************************/
/*   S T A T U S   F L A G S   */
/*******************************/

typedef struct
{
    uint8_t battery;
    uint8_t charging : 1;
    uint8_t audio : 1;
    uint8_t mic : 1;
} ps4_status_t;

/********************/
/*   S E N S O R S  */
/********************/

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} ps4_sensor_gyroscope_t;

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
} ps4_sensor_accelerometer_t;

typedef struct
{
    ps4_sensor_accelerometer_t accelerometer;
    ps4_sensor_gyroscope_t gyroscope;
} ps4_sensor_t;

/*******************/
/*    O T H E R    */
/*******************/

typedef struct
{
    uint8_t smallRumble;
    uint8_t largeRumble;
    uint8_t r, g, b;
    uint8_t flashOn;
    uint8_t flashOff; // Time to flash bright/dark (255 = 2.5 seconds)
} ps4_cmd_t;

typedef struct
{
    ps4_button_t button_down;
    ps4_button_t button_up;
    ps4_analog_t analog_move;
} ps4_event_t;

typedef struct
{
    ps4_analog_t analog;
    ps4_button_t button;
    ps4_status_t status;
    ps4_sensor_t sensor;
    uint32_t check_CRC;
    uint8_t *latestPacket;
} ps4_t;

extern ps4_t *ps4_rx_pointer;

extern void Get_PS4_Controller_data(uint8_t *data_buff, ps4_t *_ps4, uint16_t data_lenth);

#endif
