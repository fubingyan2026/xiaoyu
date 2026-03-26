/**
 ***************************************(C) COPYRIGHT 2018 DJI***************************************
 * @file       bsp_imu.h
 * @brief      this file contains the common defines and functions prototypes for
 *             the bsp_imu.c driver
 * @note
 * @Version    V1.0.0
 * @Date       Jan-30-2018
 ***************************************(C) COPYRIGHT 2018 DJI***************************************
 */

#ifndef INS_TASK_H
#define INS_TASK_H
#include "stdint.h"

#define MPU_DELAY(x) HAL_Delay(x)
#define INS_TASK_INIT_TIME 5

// MPU6500加速度计灵敏度定义 - 补充完整
#define mpu_ACCEL_2G_SEN 0.0005985504f // 2G灵敏度: 16384 LSB/g
#define mpu_ACCEL_3G_SEN 0.0008974358974f
#define mpu_ACCEL_4G_SEN 0.0011971008f // 4G灵敏度: 8192 LSB/g
#define mpu_ACCEL_6G_SEN 0.00179443359375f
#define mpu_ACCEL_8G_SEN 0.0023942016f // 8G灵敏度: 4096 LSB/g
#define mpu_ACCEL_12G_SEN 0.0035888671875f
#define mpu_ACCEL_16G_SEN 0.0047884032f // 16G灵敏度: 2048 LSB/g
#define mpu_ACCEL_24G_SEN 0.007177734375f

// 陀螺仪灵敏度
#define mpu_GYRO_2000_SEN 0.00106526443603169529841533860381f
#define mpu_GYRO_1000_SEN 0.00053263221801584764920766930190693f
#define mpu_GYRO_500_SEN 0.00026631610900792382460383465095346f
#define mpu_GYRO_250_SEN 0.00013315805450396191230191732547673f
#define mpu_GYRO_125_SEN 0.000066579027251980956150958662738366f
// 修正后的陀螺仪低通滤波器配置
typedef enum
{
    // CONFIG寄存器: [7:5]=保留, [4:3]=EXT_SYNC_SET, [2:0]=DLPF_CFG
    MPU6500_DLPF_BW_250 = 0x00,  // 250Hz, 延迟0.97ms (实际: 0.97ms)
    MPU6500_DLPF_BW_184 = 0x01,  // 184Hz, 延迟2.9ms  (实际: 2.9ms)
    MPU6500_DLPF_BW_92 = 0x02,   // 92Hz,  延迟3.9ms  (实际: 3.9ms)
    MPU6500_DLPF_BW_41 = 0x03,   // 41Hz,  延迟5.9ms  (实际: 5.9ms)
    MPU6500_DLPF_BW_20 = 0x04,   // 20Hz,  延迟9.9ms  (实际: 8.5ms)
    MPU6500_DLPF_BW_10 = 0x05,   // 10Hz,  延迟17.85ms (实际: 13.8ms)
    MPU6500_DLPF_BW_5 = 0x06,    // 5Hz,   延迟33.48ms (实际: 19.0ms)
    MPU6500_DLPF_DISABLED = 0x07 // 禁用DLPF, 延迟0ms (256Hz有效带宽)
} MPU6500_DLPF_Config;

// 修正后的加速度计低通滤波器配置
typedef enum
{
    // ACCEL_CONFIG_2寄存器: [7:4]=保留, [3]=ACCEL_FCHOICE_B, [2:0]=A_DLPF_CFG
    // 注意: 要使能DLPF，需要设置ACCEL_FCHOICE_B=1 (位3)
    MPU6500_ACCEL_DLPF_BW_460 = 0x08,  // 460Hz, 延迟1.94ms (0x00 | 0x08)
    MPU6500_ACCEL_DLPF_BW_184 = 0x09,  // 184Hz, 延迟5.80ms (0x01 | 0x08)
    MPU6500_ACCEL_DLPF_BW_92 = 0x0A,   // 92Hz,  延迟7.80ms (0x02 | 0x08)
    MPU6500_ACCEL_DLPF_BW_41 = 0x0B,   // 41Hz,  延迟11.80ms (0x03 | 0x08)
    MPU6500_ACCEL_DLPF_BW_20 = 0x0C,   // 20Hz,  延迟19.80ms (0x04 | 0x08)
    MPU6500_ACCEL_DLPF_BW_10 = 0x0D,   // 10Hz,  延迟35.70ms (0x05 | 0x08)
    MPU6500_ACCEL_DLPF_BW_5 = 0x0E,    // 5Hz,   延迟66.96ms (0x06 | 0x08)
    MPU6500_ACCEL_DLPF_DISABLED = 0x00 // 禁用DLPF (1130Hz有效带宽)
} MPU6500_Accel_DLPF_Config;

// 量程配置
typedef enum
{
    MPU6500_GYRO_FS_250 = 0x00,  // ±250°/s
    MPU6500_GYRO_FS_500 = 0x08,  // ±500°/s
    MPU6500_GYRO_FS_1000 = 0x10, // ±1000°/s (轻量化推荐)
    MPU6500_GYRO_FS_2000 = 0x18  // ±2000°/s
} MPU6500_Gyro_Range;

typedef enum
{
    MPU6500_ACCEL_FS_2 = 0x00, // ±2g
    MPU6500_ACCEL_FS_4 = 0x08, // ±4g (轻量化推荐)
    MPU6500_ACCEL_FS_8 = 0x10, // ±8g
    MPU6500_ACCEL_FS_16 = 0x18 // ±16g
} MPU6500_Accel_Range;

/*********** MPU原始数据 结构体*****************/
typedef struct
{
    int16_t ax; // accels
    int16_t ay;
    int16_t az;

    int16_t gx; // gyro
    int16_t gy;
    int16_t gz;

    int16_t mx; // mag磁力计数据
    int16_t my;
    int16_t mz;

    int16_t temp;

    float accel_offset[3]; // 偏移量
    float gyro_offset[3];

    float gyro_cali_offset[3];
    float accel_cali_offset[3];
    float mag_cali_offset[3];
} mpu_data_t;

/*********** IMU数据 结构体*****************/
typedef struct
{
    float pit;
    float rol;
    float yaw;

    int16_t ax;
    int16_t ay;
    int16_t az;

    int16_t mx; // 磁力计数据
    int16_t my;
    int16_t mz;

    float temp;

    float wx; /*!< omiga, +- 2000dps => +-32768  so gx/16.384/57.3 =	rad/s */
    float wy;
    float wz;

    // float vx;
    // float vy;
    // float vz;

} imu_t;

extern mpu_data_t mpu_data;
extern imu_t imu;

/**
 * @brief 初始化imu mpu6500和磁性计ist3810
 * @param
 * @retval
 * @usage  call in main() function
 */
uint8_t mpu_device_init(void);
/**
 *@brief初始化四元数
 *@param
 *@retval
 *@usage在main（）函数中调用
 */
extern void init_quaternion(void);
/**
 *@     获取mpu  ist数据
 *@param
 *@retval
 *@usage在main（）函数中调用
 */
extern void get_mpu_data(void);
/**
 *@简要更新imu AHRS
 *@param
 *@retval
 *@usage在main（）函数中调用
 */
extern void imu_ahrs_update(void);
/**
 *@简要更新imu态度
 *@param
 *@retval
 *@usage在main（）函数中调用
 */
extern void imu_attitude_update(void);
/**
 *@brief获取MPU6500的偏移数据
 *@param
 *@retval
 *@usage在main（）函数中调用
 */
extern void mpu_offset_call(void);

extern void INS_task_Init(void);

extern void INS_task_Loop(void);

/**
 * @brief          IMU数据处理任务
 * @retval         none
 */
extern void INS_task(void const *pvParameters);
/**
 * @brief          校准陀螺仪设置，将从flash或者其他地方传入校准值
 * @param[in]      陀螺仪的比例因子，1.0f为默认值，不修改
 * @param[in]      陀螺仪的零漂
 * @retval         none
 */
extern void INS_set_cali_gyro(float cali_scale[3], const float cali_offset[3]);
/**
 * @brief          校准陀螺仪
 * @param[out]     陀螺仪的比例因子，1.0f为默认值，不修改
 * @param[out]     陀螺仪的零漂，采集陀螺仪的静止的输出作为offset
 * @param[out]     陀螺仪的时刻，每次在gyro_offset调用会加1,
 * @retval         none
 */
extern void INS_cali_gyro(float cali_scale[3], float cali_offset[3], uint16_t *time_count);
/**
 * @brief          计算陀螺仪零漂
 * @param[out]     gyro_offset:计算零漂
 * @param[in]      gyro:角速度数据
 * @param[out]     offset_time_count: 自动加1
 * @retval         none
 */
extern void gyro_offset_calc(float gyro_offset[3], const float gyro[3], uint16_t *offset_time_count);

/**
 * @brief          获取欧拉角, 0:yaw, 1:pitch, 2:roll 单位 rad
 * @param[in]      none
 * @retval         INS_angle的指针
 */
extern const imu_t *get_INS_angle_point(void);

#endif
