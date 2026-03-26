/**
 ***************************************(C) COPYRIGHT 2018 DJI***************************************
 * @file       bsp_imu.c
 * @brief      mpu6500 module driver, configurate MPU6500 and Read the Accelerator
 *             and Gyrometer data using SPI interface
 * @note
 * @Version    V1.0.0
 * @Date       Jan-30-2018
 ***************************************(C) COPYRIGHT 2018 DJI***************************************
 */

#include "ins_task.h"

#include <stdlib.h>

#include "bsp_delay.h"
#include "spi.h"
#include "main.h"
#include "string.h"
#include "bsp_imu_pwm.h"
#include "algorithm/maths.h"
#include "algorithm/user_lib.h"
#include "memory_pool/memory_pool.h"

#define BOARD_DOWN (1)
// #define IST8310
#define MPU_HSPI hspi2
#define MPU_NSS_LOW HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_RESET)
#define MPU_NSS_HIGH HAL_GPIO_WritePin(IMU_CS_GPIO_Port, IMU_CS_Pin, GPIO_PIN_SET)

#define Kp (2.50f)
#define Ki (0.1f)
				
float mpu_accel_SEN = mpu_ACCEL_4G_SEN;
float mpu_gyro_SEN = mpu_GYRO_1000_SEN;

static volatile float q0 = 1.0f;
static volatile float q1 = 0.0f;
static volatile float q2 = 0.0f;
static volatile float q3 = 0.0f;
volatile float exInt, eyInt, ezInt; /* error integral */
static volatile float gx, gy, gz, ax, ay, az, mx, my, mz;
volatile uint32_t last_update, now_update; /* Sampling cycle count, ubit ms */
static uint8_t tx, rx; 
static uint8_t tx_buff[14] = {0xff};
uint8_t mpu_buff[14]; /* buffer to save imu raw data */
uint8_t ist_buff[6]; /* buffer to save IST8310 raw data */
mpu_data_t mpu_data;
imu_t imu = {0};

// static float accel_fliter_1[3] = { 0.0f, 0.0f, 0.0f };
// static float accel_fliter_2[3] = { 0.0f, 0.0f, 0.0f };
// static float accel_fliter_3[3] = { 0.0f, 0.0f, 0.0f };
// static const float fliter_num[3] = { 1.929454039488895f, -0.93178349823448126f, 0.002329458745586203f };
// INS
float INS_gyro[3] = {0.0f, 0.0f, 0.0f};
float INS_accel[3] = {0.0f, 0.0f, 0.0f};
float INS_mag[3] = {0.0f, 0.0f, 0.0f};
float INS_quat[4] = {0.0f, 0.0f, 0.0f, 0.0f};
float INS_angle[3] = {0.0f, 0.0f, 0.0f}; // rad

/**
 * @brief  write a byte of data to specified register
 * @param  reg:  the address of register to be written
 *         data: data to be written
 * @retval
 * @usage  call in ist_reg_write_by_mpu(),
 *                 ist_reg_read_by_mpu(),
 *                 mpu_master_i2c_auto_read_config(),
 *                 ist8310_init(),
 *                 mpu_set_gyro_fsr(),
 *                 mpu_set_accel_fsr(),
 *                 mpu_device_init() function
 */
uint8_t mpu_write_byte(uint8_t const reg, uint8_t const data)
{
    MPU_NSS_LOW;
    tx = reg & 0x7F;
    HAL_SPI_TransmitReceive(&MPU_HSPI, &tx, &rx, 1, 55);
    tx = data;
    HAL_SPI_TransmitReceive(&MPU_HSPI, &tx, &rx, 1, 55);
    MPU_NSS_HIGH;
    return 0;
}

/**
 * @brief  Reads a single byte from the specified MPU register.
 * @param  reg: The register address to read from.
 * @retval The value of the byte read from the specified register.
 */
uint8_t mpu_read_byte(uint8_t const reg)
{
    MPU_NSS_LOW;
    tx = reg | 0x80;
    HAL_SPI_TransmitReceive(&MPU_HSPI, &tx, &rx, 1, 55);
    HAL_SPI_TransmitReceive(&MPU_HSPI, &tx, &rx, 1, 55);
    MPU_NSS_HIGH;
    return rx;
}

/**
 * @brief  Reads a specified number of bytes from the MPU device starting at the given register address.
 * @param  regAddr: The starting register address to read from.
 * @param  pData: Pointer to the buffer where the read data will be stored.
 * @param  len: Number of bytes to read.
 * @retval Returns 0 on success.
 */
uint8_t mpu_read_bytes(uint8_t const regAddr, uint8_t* pData, uint8_t len)
{
    MPU_NSS_LOW;
    tx = regAddr | 0x80;
    tx_buff[0] = tx;
    HAL_SPI_TransmitReceive(&MPU_HSPI, &tx, &rx, 1, 55);
    HAL_SPI_TransmitReceive(&MPU_HSPI, tx_buff, pData, len, 55);
    MPU_NSS_HIGH;
    return 0;
}

/**
 * @brief  write IST8310 register through MPU6500's I2C master
 * @param  addr: the address to be written of IST8310's register
 *         data: data to be written
 * @retval
 * @usage  call in ist8310_init() function
 */
static void ist_reg_write_by_mpu(uint8_t addr, uint8_t data)
{
    /* turn off slave 1 at first */
    mpu_write_byte(MPU6500_I2C_SLV1_CTRL, 0x00);
    MPU_DELAY(2);
    mpu_write_byte(MPU6500_I2C_SLV1_REG, addr);
    MPU_DELAY(2);
    mpu_write_byte(MPU6500_I2C_SLV1_DO, data);
    MPU_DELAY(2);
    /* turn on slave 1 with one byte transmitting */
    mpu_write_byte(MPU6500_I2C_SLV1_CTRL, 0x80 | 0x01);
    /* wait longer to ensure the data is transmitted from slave 1 */
    MPU_DELAY(10);
}

/**
 * @brief  write IST8310 register through MPU6500's I2C Master
 * @param  addr: the address to be read of IST8310's register
 * @retval
 * @usage  call in ist8310_init() function
 */
static uint8_t ist_reg_read_by_mpu(uint8_t addr)
{
    mpu_write_byte(MPU6500_I2C_SLV4_REG, addr);
    MPU_DELAY(10);
    mpu_write_byte(MPU6500_I2C_SLV4_CTRL, 0x80);
    MPU_DELAY(10);
    uint8_t retval = mpu_read_byte(MPU6500_I2C_SLV4_DI);
    /* turn off slave4 after read */
    mpu_write_byte(MPU6500_I2C_SLV4_CTRL, 0x00);
    MPU_DELAY(10);
    return retval;
}

/**
 * @brief    initialize the MPU6500 I2C Slave 0 for I2C reading.
 * @param    device_address: slave device address, Address[6:0]
 * @retval   void
 * @note
 */
static void mpu_master_i2c_auto_read_config(uint8_t device_address, uint8_t reg_base_addr, uint8_t data_num)
{
    /*
     * configure the device address of the IST8310
     * use slave1, auto transmit single measure mode
     */
    mpu_write_byte(MPU6500_I2C_SLV1_ADDR, device_address);
    MPU_DELAY(2);
    mpu_write_byte(MPU6500_I2C_SLV1_REG, IST8310_R_CONFA);
    MPU_DELAY(2);
    mpu_write_byte(MPU6500_I2C_SLV1_DO, IST8310_ODR_MODE);
    MPU_DELAY(2);

    /* use slave0,auto read data */
    mpu_write_byte(MPU6500_I2C_SLV0_ADDR, 0x80 | device_address);
    MPU_DELAY(2);
    mpu_write_byte(MPU6500_I2C_SLV0_REG, reg_base_addr);
    MPU_DELAY(2);

    /* every eight mpu6500 internal samples one i2c master read */
    mpu_write_byte(MPU6500_I2C_SLV4_CTRL, 0x03);
    MPU_DELAY(2);
    /* enable slave 0 and 1 access delay */
    mpu_write_byte(MPU6500_I2C_MST_DELAY_CTRL, 0x01 | 0x02);
    MPU_DELAY(2);
    /* enable slave 1 auto transmit */
    mpu_write_byte(MPU6500_I2C_SLV1_CTRL, 0x80 | 0x01);
    /* Wait 6ms (minimum waiting time for 16 times internal average setup) */
    MPU_DELAY(6);
    /* enable slave 0 with data_num bytes reading */
    mpu_write_byte(MPU6500_I2C_SLV0_CTRL, 0x80 | data_num);
    MPU_DELAY(2);
}

/**
 * @brief  Initializes the IST8310 device
 * @param
 * @retval
 * @usage  call in mpu_device_init() function
 */
uint8_t ist8310_init()
{
    /* enable iic master mode */
    mpu_write_byte(MPU6500_USER_CTRL, 0x30);
    MPU_DELAY(10);
    /* enable iic 400khz */
    mpu_write_byte(MPU6500_I2C_MST_CTRL, 0x0d);
    MPU_DELAY(10);

    /* turn on slave 1 for ist write and slave 4 to ist read */
    mpu_write_byte(MPU6500_I2C_SLV1_ADDR, IST8310_ADDRESS);
    MPU_DELAY(10);
    mpu_write_byte(MPU6500_I2C_SLV4_ADDR, 0x80 | IST8310_ADDRESS);
    MPU_DELAY(10);

    /* IST8310_R_CONFB 0x01 = device rst */
    ist_reg_write_by_mpu(IST8310_R_CONFB, 0x01);
    MPU_DELAY(10);
    if (IST8310_DEVICE_ID_A != ist_reg_read_by_mpu(IST8310_WHO_AM_I))
        return 1;

    /* soft reset */
    ist_reg_write_by_mpu(IST8310_R_CONFB, 0x01);
    MPU_DELAY(10);

    /* config as ready mode to access register */
    ist_reg_write_by_mpu(IST8310_R_CONFA, 0x00);
    if (ist_reg_read_by_mpu(IST8310_R_CONFA) != 0x00)
        return 2;
    MPU_DELAY(10);

    /* normal state, no int */
    ist_reg_write_by_mpu(IST8310_R_CONFB, 0x00);
    if (ist_reg_read_by_mpu(IST8310_R_CONFB) != 0x00)
        return 3;
    MPU_DELAY(10);

    /* config low noise mode, x,y,z axis 16 time 1 avg */
    ist_reg_write_by_mpu(IST8310_AVGCNTL, 0x24); // 100100
    if (ist_reg_read_by_mpu(IST8310_AVGCNTL) != 0x24)
        return 4;
    MPU_DELAY(10);

    /* Set/Reset pulse duration setup,normal mode */
    ist_reg_write_by_mpu(IST8310_PDCNTL, 0xc0);
    if (ist_reg_read_by_mpu(IST8310_PDCNTL) != 0xc0)
        return 5;
    MPU_DELAY(10);

    /* turn off slave1 & slave 4 */
    mpu_write_byte(MPU6500_I2C_SLV1_CTRL, 0x00);
    MPU_DELAY(10);
    mpu_write_byte(MPU6500_I2C_SLV4_CTRL, 0x00);
    MPU_DELAY(10);

    /* configure and turn on slave 0 */
    mpu_master_i2c_auto_read_config(IST8310_ADDRESS, IST8310_R_XL, 0x06);
    MPU_DELAY(100);
    return 0;
}

/**
 * @brief  get the data of IST8310
 * @param  buff: the buffer to save the data of IST8310
 * @retval
 * @usage  call in mpu_get_data() function
 */
void ist8310_get_data(uint8_t* buff)
{
    mpu_read_bytes(MPU6500_EXT_SENS_DATA_00, buff, 6);
}

/**
 * @brief  Set the full-scale range of the gyroscope
 * @param  fsr: The full-scale range value to be set. This value is shifted left by 3 bits before being written to the register.
 * @retval Returns the result of the mpu_write_byte function, which writes the specified data to the MPU6500_GYRO_CONFIG register.
 */
uint8_t mpu_set_gyro_fsr(const uint8_t fsr)
{
    return mpu_write_byte(MPU6500_GYRO_CONFIG, fsr << 3);
}

/**
 * @brief  Set the full scale range of the accelerometer
 * @param  fsr: Full scale range setting value. The actual value written to the register is fsr shifted left by 3 bits.
 * @retval Result of the mpu_write_byte function, which writes the fsr value to the MPU6500_ACCEL_CONFIG register.
 */
uint8_t mpu_set_accel_fsr(const uint8_t fsr)
{
    return mpu_write_byte(MPU6500_ACCEL_CONFIG, fsr << 3);
}

/**
 * @brief  Initializes the MPU6500 device, setting up its configuration and starting auxiliary components.
 * @details This function resets the device, configures the power management, sets the clock source, enables
 *          accelerometer and gyroscope, configures the low pass filter, sets the full scale range for both
 *          accelerometer and gyroscope, initializes the IST8310 magnetometer, and calls a function to set
 *          offsets. It also introduces delays between initialization steps.
 * @retval 0 if the initialization is successful, otherwise non-zero value (currently always returns 0).
 * @note   The function assumes that mpu_write_byte, mpu_read_byte, ist8310_init, mpu_set_gyro_fsr,
 *         mpu_set_accel_fsr, and mpu_offset_call are defined elsewhere.
 */
uint8_t mpu_device_init(void)
{
    MPU_DELAY(100);

    uint8_t id = mpu_read_byte(MPU6500_WHO_AM_I);
    uint8_t i = 0;
    //考虑到轻量化云台对振动敏感，但同时也需要快速响应，我建议如下设置：
    //陀螺仪量程：±1000dps（0x10）
    //加速度计量程：±4G（0x08）
    //陀螺仪低通滤波器：20Hz（0x05）或10Hz（0x06）
    //加速度计低通滤波器：20Hz（0x05）或10Hz（0x06)
    //MPU6500_CONFIG寄存器用于设置陀螺仪的低通滤波器
    //而MPU6500_ACCEL_CONFIG_2用于设置加速度计的低通滤波器。
    uint8_t MPU6500_Init_Data[10][2] = {
        {MPU6500_INT_ENABLE, 0x01},
        {MPU6500_PWR_MGMT_1, 0x80}, /* Reset Device */
        {MPU6500_PWR_MGMT_1, 0x03}, /* Clock Source - Gyro-Z */
        {MPU6500_PWR_MGMT_2, 0x00}, /* Enable Acc & Gyro */
        {MPU6500_CONFIG, MPU6500_DLPF_BW_250}, /* LPF 41Hz(0x04) 20Hz(0x05) 10Hz(0x06) */
        {MPU6500_GYRO_CONFIG, MPU6500_GYRO_FS_1000}, /* +-2000dps(0x18) ±500dps(0x08) ±1000dps(0x10) */
        {MPU6500_ACCEL_CONFIG, MPU6500_ACCEL_FS_4}, /* +-8G(0x10) ±4G(0x08) ±2G(0x00) */
        {MPU6500_ACCEL_CONFIG_2, MPU6500_DLPF_BW_250}, /* enable LowPassFilter  Set Acc LPF */
        {MPU6500_USER_CTRL, 0x20},     /* 使能I2C主模式 */
    }; /* Enable AUX */
    for (i = 0; i < 10; i++)
    {
        mpu_write_byte(MPU6500_Init_Data[i][0], MPU6500_Init_Data[i][1]);
        MPU_DELAY(1);
    }

    mpu_set_gyro_fsr(3);
    mpu_set_accel_fsr(2);

    ist8310_init();
    mpu_offset_call();
    return 0;
}

/**
 * @brief  Initializes and sets the offset values for the accelerometer and gyroscope
 *
 * This function is intended to calculate and set the average offset values for the
 * accelerometer and gyroscope. It reads a series of raw data samples from the MPU6500
 * sensor, calculates the average offset for each axis, and stores these offsets in
 * the `mpu_data` structure. The current implementation sets fixed offset values.
 *
 * @note In the provided code, the actual calculation of the offsets is commented out,
 * and fixed values are assigned instead. For a real-world application, the comments
 * should be removed, and the fixed values should be replaced with the calculated
 * averages.
 *
 * @retval None
 *
 * @usage  This function is called during the initialization process of the MPU6500
 *         device, typically within the `mpu_device_init()` function.
 */
void mpu_offset_call(void)
{
    // static const uint16_t Offest_count = 100;
    // for (int i = 0; i < Offest_count; i++)
    // {
    //     mpu_read_bytes(MPU6500_ACCEL_XOUT_H, mpu_buff, 14);
    //     mpu_data.accel_offset[0] += (float)(mpu_buff[0] << 8 | mpu_buff[1]);
    //     mpu_data.accel_offset[1] += (float)(mpu_buff[2] << 8 | mpu_buff[3]);
    //     mpu_data.accel_offset[2] += (float)(mpu_buff[4] << 8 | mpu_buff[5]);
    //     mpu_data.gyro_offset[0] += (float)(mpu_buff[8] << 8 | mpu_buff[9]);
    //     mpu_data.gyro_offset[1] += (float)(mpu_buff[10] << 8 | mpu_buff[11]);
    //     mpu_data.gyro_offset[2] += (float)(mpu_buff[12] << 8 | mpu_buff[13]);
    //     MPU_DELAY(5);
    // }
    // mpu_data.accel_offset[0] = mpu_data.accel_offset[0] / (float)Offest_count;
    // mpu_data.accel_offset[1] = mpu_data.accel_offset[1] / (float)Offest_count;
    // mpu_data.accel_offset[2] = mpu_data.accel_offset[2] / (float)Offest_count;
    // mpu_data.gyro_offset[0] = mpu_data.gyro_offset[0] / (float)Offest_count;
    // mpu_data.gyro_offset[1] = mpu_data.gyro_offset[1] / (float)Offest_count;
    // mpu_data.gyro_offset[2] = mpu_data.gyro_offset[2] / (float)Offest_count;

    mpu_data.accel_offset[0] = 65465.96f;
    mpu_data.accel_offset[1] = 13112.28f;
    mpu_data.accel_offset[2] = 4366.87f;
    mpu_data.gyro_offset[0] = 24.36f;
    mpu_data.gyro_offset[1] = 65522.25f;
    mpu_data.gyro_offset[2] = 7.84f;
}

/**
 * @brief  Initializes the quaternion values based on the magnetic field data
 * @details This function initializes the quaternion values (q0, q1, q2, q3) which are used to represent the orientation of an object in 3D space. The initialization is based on the magnetic field data (hx, hy) from the IMU sensor.
 *          The function considers the signs and relative magnitudes of hx and hy to determine the appropriate initial quaternion values. These values are set differently depending on whether the BOARD_DOWN macro is defined or not.
 * @note   The function assumes that the global variables `imu.mx` and `imu.my` are already populated with valid magnetic field data.
 */
void init_quaternion(void)
{
    // hz;
    int16_t hx = imu.mx;
    int16_t hy = imu.my;
    // hz = imu.mz;

#if BOARD_DOWN==1
    if (hx < 0 && hy < 0)
    {
        if (abs(hx / hy) >= 1)
        {
            q0 = -0.005f;
            q1 = -0.199f;
            q2 = 0.979f;
            q3 = -0.0089f;
        }
        else
        {
            q0 = -0.008f;
            q1 = -0.555f;
            q2 = 0.83f;
            q3 = -0.002f;
        }
    }
    else if (hx < 0 && hy > 0)
    {
        if (abs(hx / hy) >= 1)
        {
            q0 = 0.005f;
            q1 = -0.199f;
            q2 = -0.978f;
            q3 = 0.012f;
        }
        else
        {
            q0 = 0.005f;
            q1 = -0.553f;
            q2 = -0.83f;
            q3 = -0.0023f;
        }
    }
    else if (hx > 0 && hy > 0)
    {
        if (abs(hx / hy) >= 1)
        {
            q0 = 0.0012f;
            q1 = -0.978f;
            q2 = -0.199f;
            q3 = -0.005f;
        }
        else
        {
            q0 = 0.0023f;
            q1 = -0.83f;
            q2 = -0.553f;
            q3 = 0.0023f;
        }
    }
    else if (hx > 0 && hy < 0)
    {
        if (abs(hx / hy) >= 1)
        {
            q0 = 0.0025f;
            q1 = 0.978f;
            q2 = -0.199f;
            q3 = 0.008f;
        }
        else
        {
            q0 = 0.0025f;
            q1 = 0.83f;
            q2 = -0.56f;
            q3 = 0.0045f;
        }
    }
#else
    if (hx < 0 && hy < 0)
    {
        if (abs(hx / hy) >= 1)
        {
            q0 = 0.195f;
            q1 = -0.015f;
            q2 = 0.0043f;
            q3 = 0.979f;
        }
        else
        {
            q0 = 0.555f;
            q1 = -0.015f;
            q2 = 0.006f;
            q3 = 0.829f;
        }
    }
    else if (hx < 0 && hy > 0)
    {
        if (abs(hx / hy) >= 1)
        {
            q0 = -0.193f;
            q1 = -0.009f;
            q2 = -0.006f;
            q3 = 0.979f;
        }
        else
        {
            q0 = -0.552f;
            q1 = -0.0048f;
            q2 = -0.0115f;
            q3 = 0.8313f;
        }
    }
    else if (hx > 0 && hy > 0)
    {
        if (abs(hx / hy) >= 1)
        {
            q0 = -0.9785f;
            q1 = 0.008f;
            q2 = -0.02f;
            q3 = 0.195f;
        }
        else
        {
            q0 = -0.9828f;
            q1 = 0.002f;
            q2 = -0.0167f;
            q3 = 0.5557f;
        }
    }
    else if (hx > 0 && hy < 0)
    {
        if (abs(hx / hy) >= 1)
        {
            q0 = -0.979f;
            q1 = 0.0116f;
            q2 = -0.0167f;
            q3 = -0.195f;
        }
        else
        {
            q0 = -0.83f;
            q1 = 0.014f;
            q2 = -0.012f;
            q3 = -0.556f;
        }
    }
#endif
}

/**
 * @brief  Updates the Attitude and Heading Reference System (AHRS) using IMU data
 * @details This function integrates gyroscope, accelerometer, and magnetometer data to update the orientation of the system.
 *          It uses a complementary filter with a proportional-integral (PI) controller to correct for drift.
 * @note   This function should be called periodically with new IMU data to maintain an accurate orientation estimate.
 */
void imu_ahrs_update(void)
{
    float hx, hy, hz, bx, bz;
    float vx, vy, vz, wx, wy, wz;
    float ex, ey, ez;
    float tempq0, tempq1, tempq2, tempq3;

    float q0q0 = q0 * q0;
    float q0q1 = q0 * q1;
    float q0q2 = q0 * q2;
    float q0q3 = q0 * q3;
    float q1q1 = q1 * q1;
    float q1q2 = q1 * q2;
    float q1q3 = q1 * q3;
    float q2q2 = q2 * q2;
    float q2q3 = q2 * q3;
    float q3q3 = q3 * q3;

    gx = imu.wx;
    gy = imu.wy;
    gz = imu.wz;
    ax = imu.ax;
    ay = imu.ay;
    az = imu.az;
    mx = imu.mx;
    my = imu.my;
    mz = imu.mz;

    now_update = millis(); // ms
    const float halfT = (float)(now_update - last_update) / 2000.0f;
    last_update = now_update;

    float norm = invSqrt(ax * ax + ay * ay + az * az);
    ax = ax * norm;
    ay = ay * norm;
    az = az * norm;

#ifdef IST8310
	norm = invSqrt(mx * mx + my * my + mz * mz);
	mx = mx * norm;
	my = my * norm;
	mz = mz * norm;
#else
    mx = 0;
    my = 0;
    mz = 0;
#endif
    hx = 2.0f * mx * (0.5f - q2q2 - q3q3) + 2.0f * my * (q1q2 - q0q3) + 2.0f * mz * (q1q3 + q0q2);
    hy = 2.0f * mx * (q1q2 + q0q3) + 2.0f * my * (0.5f - q1q1 - q3q3) + 2.0f * mz * (q2q3 - q0q1);
    hz = 2.0f * mx * (q1q3 - q0q2) + 2.0f * my * (q2q3 + q0q1) + 2.0f * mz * (0.5f - q1q1 - q2q2);
    bx = 1.0f / invSqrt((hx * hx) + (hy * hy));
    bz = hz;

    vx = 2.0f * (q1q3 - q0q2);
    vy = 2.0f * (q0q1 + q2q3);
    vz = q0q0 - q1q1 - q2q2 + q3q3;
    wx = 2.0f * bx * (0.5f - q2q2 - q3q3) + 2.0f * bz * (q1q3 - q0q2);
    wy = 2.0f * bx * (q1q2 - q0q3) + 2.0f * bz * (q0q1 + q2q3);
    wz = 2.0f * bx * (q0q2 + q1q3) + 2.0f * bz * (0.5f - q1q1 - q2q2);

    ex = (ay * vz - az * vy) + (my * wz - mz * wy);
    ey = (az * vx - ax * vz) + (mz * wx - mx * wz);
    ez = (ax * vy - ay * vx) + (mx * wy - my * wx);

    /* PI */
    if (ex != 0.0f && ey != 0.0f && ez != 0.0f)
    {
        exInt = exInt + ex * Ki * halfT;
        eyInt = eyInt + ey * Ki * halfT;
        ezInt = ezInt + ez * Ki * halfT;

        gx = gx + Kp * ex + exInt;
        gy = gy + Kp * ey + eyInt;
        gz = gz + Kp * ez + ezInt;
    }

    tempq0 = q0 + (-q1 * gx - q2 * gy - q3 * gz) * halfT;
    tempq1 = q1 + (q0 * gx + q2 * gz - q3 * gy) * halfT;
    tempq2 = q2 + (q0 * gy - q1 * gz + q3 * gx) * halfT;
    tempq3 = q3 + (q0 * gz + q1 * gy - q2 * gx) * halfT;

    norm = invSqrt(tempq0 * tempq0 + tempq1 * tempq1 + tempq2 * tempq2 + tempq3 * tempq3);
    q0 = tempq0 * norm;
    q1 = tempq1 * norm;
    q2 = tempq2 * norm;
    q3 = tempq3 * norm;
}

/**
 * @brief  Retrieves and processes data from the MPU6500 sensor, including acceleration, temperature, and gyroscope values.
 * @details This function reads raw data from the MPU6500 sensor, converts it into meaningful acceleration, temperature, and gyroscope measurements,
 *          and stores these in the `mpu_data` and `imu` structures. It also applies a gyro offset to the gyroscope readings and converts
 *          the temperature and gyroscope values into more usable units (degrees Celsius and radians per second, respectively).
 * @note   The function assumes that `mpu_buff` is large enough to hold at least 14 bytes of data.
 * @note   The conversion for temperature is based on a linear approximation: 21 + (temp / 333.87) degrees Celsius.
 * @note   Gyroscope values are converted from digital levels to radians per second, assuming a full scale range of 2000dps.
 */
void get_mpu_data(void)
{
    // uint16_t mpu_raw;
    mpu_read_bytes(MPU6500_ACCEL_XOUT_H, mpu_buff, sizeof(mpu_buff));

    mpu_data.ax = (int16_t)(mpu_buff[0] << 8 | mpu_buff[1]);
    mpu_data.ay = (int16_t)(mpu_buff[2] << 8 | mpu_buff[3]);
    mpu_data.az = (int16_t)(mpu_buff[4] << 8 | mpu_buff[5]);
    mpu_data.temp = (int16_t)(mpu_buff[6] << 8 | mpu_buff[7]);

    mpu_data.gx = (int16_t)((int16_t)(mpu_buff[8] << 8 | mpu_buff[9]) - (int16_t)mpu_data.gyro_offset[0]);
    mpu_data.gy = (int16_t)((int16_t)(mpu_buff[10] << 8 | mpu_buff[11]) - (int16_t)mpu_data.gyro_offset[1]);
    mpu_data.gz = (int16_t)(((int16_t)mpu_buff[12] << 8 | mpu_buff[13]) - (int16_t)mpu_data.gyro_offset[2]);

    // ist8310_get_data(ist_buff);
    // __memcpy(&mpu_data.mx, ist_buff, 3 * sizeof(int16_t));

    __memcpy(&imu.ax, &mpu_data.ax, 3 * sizeof(int16_t));

    imu.temp = 21 + (float)mpu_data.temp / 333.87f;
    /* 2000dps -> rad/s */
    imu.wx = (float)mpu_data.gx / 16.384f / 57.3f;
    imu.wy = (float)mpu_data.gy / 16.384f / 57.3f;
    imu.wz = (float)mpu_data.gz / 16.384f / 57.3f;
}

/**
 * @brief  Update the attitude (yaw, pitch, roll) of the IMU based on the current quaternion values.
 * @details This function calculates the yaw, pitch, and roll angles from the current quaternion
 *          values (q0, q1, q2, q3) and updates the imu structure with these new values. The angles
 *          are converted from radians to degrees.
 * @note   The angles are calculated using approximations for the atan2 and asin functions.
 * @retval None
 * @usage  Call in INS_task() or INS_task_Loop() after updating the quaternion values.
 */
void imu_attitude_update(void)
{
    /* yaw    -pi----pi */
    imu.yaw = -atan2_approx(2 * q1 * q2 + 2 * q0 * q3, -2 * q2 * q2 - 2 * q3 * q3 + 1) * 57.3f;
    /* pitch  -pi/2----pi/2 */
    imu.pit = -asin_approx(-2 * q1 * q3 + 2 * q0 * q2) * 57.3f;
    /* roll   -pi----pi  */
    imu.rol = atan2_approx(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2 * q2 + 1) * 57.3f;
}

void INS_task(void const* pvParameters)
{
    // wait a time
    delay_ms(20);

    while (mpu_device_init())
    {
        delay_ms(50);
    }

    init_quaternion();

    for (;;)
    {
        get_mpu_data();
        imu_ahrs_update();
        imu_attitude_update();
        // vTaskDelay(INS_TASK_INIT_TIME);
    }
}

void INS_task_Init(void)
{
    // wait a time
    delay_ms(20);

    while (mpu_device_init())
    {
        delay_ms(50);
    }

    init_quaternion();
}

//efficiency 170M 37us
void INS_task_Loop(void)
{
    get_mpu_data();
    imu_ahrs_update();
    imu_attitude_update();
}

/**
 * @brief  Set the calibration values for the gyroscope
 * @param  cali_scale: Array of scale factors for each axis (x, y, z)
 * @param  cali_offset: Array of offset values for each axis (x, y, z)
 */
void INS_set_cali_gyro(float cali_scale[3], const float cali_offset[3])
{
    mpu_data.gyro_cali_offset[0] = cali_offset[0];
    mpu_data.gyro_cali_offset[1] = cali_offset[1];
    mpu_data.gyro_cali_offset[2] = cali_offset[2];
    mpu_data.gyro_offset[0] = mpu_data.gyro_cali_offset[0];
    mpu_data.gyro_offset[1] = mpu_data.gyro_cali_offset[1];
    mpu_data.gyro_offset[2] = mpu_data.gyro_cali_offset[2];
}

/**
 * @brief  Calibrates the gyroscope by updating its offset and setting scale factors
 * @param  cali_scale Array to store the calibration scale factors for each axis
 * @param  cali_offset Array to store the calculated calibration offsets for each axis
 * @param  time_count Pointer to a variable that keeps track of the calibration time
 */
void INS_cali_gyro(float cali_scale[3], float cali_offset[3], uint16_t* time_count)
{
    if (*time_count == 0)
    {
        mpu_data.gyro_offset[0] = mpu_data.gyro_cali_offset[0];
        mpu_data.gyro_offset[1] = mpu_data.gyro_cali_offset[1];
        mpu_data.gyro_offset[2] = mpu_data.gyro_cali_offset[2];
    }
    gyro_offset_calc(mpu_data.gyro_offset, INS_gyro, time_count);

    cali_offset[0] = mpu_data.gyro_offset[0];
    cali_offset[1] = mpu_data.gyro_offset[1];
    cali_offset[2] = mpu_data.gyro_offset[2];
    cali_scale[0] = 1.0f;
    cali_scale[1] = 1.0f;
    cali_scale[2] = 1.0f;
}

/**
 * @brief  Calculate and update the gyroscope offset over time
 * @param  gyro_offset: Array to store the updated gyroscope offsets
 * @param  gyro: Current gyroscope readings
 * @param  offset_time_count: Pointer to a counter for the number of times the offset has been updated
 */
void gyro_offset_calc(float gyro_offset[3], const float gyro[3], uint16_t* offset_time_count)
{
    if (gyro_offset == NULL || gyro == NULL || offset_time_count == NULL)
    {
        return;
    }

    gyro_offset[0] = gyro_offset[0] - 0.0003f * gyro[0];
    gyro_offset[1] = gyro_offset[1] - 0.0003f * gyro[1];
    gyro_offset[2] = gyro_offset[2] - 0.0003f * gyro[2];
    (*offset_time_count)++;
}

/**
 * @brief  Returns a pointer to the IMU (Inertial Measurement Unit) data structure.
 * @retval A constant pointer to the imu_t structure containing the current IMU data.
 */
extern const imu_t* get_INS_angle_point(void)
{
    return &imu;
}
