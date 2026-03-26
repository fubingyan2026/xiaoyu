#include "joint_pos.h"
#include "maths.h"
// 定义常量
#define PI 3.14159265358979323846f
#define KP 0.1f // 比例控制增益

// 函数声明
void inverse_kinematics(float x, float y, float l1, float l2, float *theta1, float *theta2, int elbow);

void joint_velocities(float x, float y, float x_dot, float y_dot, float l1, float l2, float theta1, float theta2,
                      float *w1, float *w2);

void forward_kinematics(float theta1, float theta2, float l1, float l2, float *x, float *y);


float x = 0.5f, y = 0.5f; // 末端目标位置
float l1 = 1, l2 = 1.2f; // 连杆长度
float target_theta1, target_theta2; // 关节角度
float x_dot, y_dot; // 末端速度
float w1, w2; // 关节电机速度
int elbow = 0; // 肘部上(1)或肘下(0)
float x_forward, y_forward; // 正解算得到的末端位置
float current_theta1 = PI, current_theta2 = -PI; // 关节角度

int Joint_Pos_Ioop(void)
{
    // 输入连杆长度
    // printf("请输入连杆长度l1和l2(用空分格隔): ");
    // scanf("%lf %lf", &l1, &l2);

    // 输入末端目标位置
    // printf("请输入末端目标位置x和y(用空格分隔): ");
    // scanf("%lf %lf", &x, &y);

    // 输入肘部上或肘下
    // printf("请输入肘部上(1)或肘下(0): ");
    // scanf("%d", &elbow);

    // 计算正解算得到的末端位置
    forward_kinematics(current_theta1, current_theta2, l1, l2, &x_forward, &y_forward);

    // 计算关节角度
    inverse_kinematics(x, y, l1, l2, &target_theta1, &target_theta2, elbow);

    // 输入末端速度
    // printf("请输入末端速度x_dot和y_dot(用空格分隔): ");
    // scanf("%lf %lf", &x_dot, &y_dot);
    x_dot = KP * (x - x_forward);
    y_dot = KP * (y - y_forward);
    // 计算关节电机速度
    joint_velocities(x, y, x_dot, y_dot, l1, l2, target_theta1, target_theta2, &w1, &w2);

    // 输出结果
    //    printf("\n逆解算结果:\n");
    //    printf("关节1的角度: %lf 弧度 (%lf 度)\n", target_theta1, target_theta1 * 180 / PI);
    //    printf("关节2的角度: %lf 弧度 (%lf 度)\n", target_theta2, target_theta2 * 180 / PI);
    //    printf("关节1的电机速度: %lf 弧度/秒\n", w1);
    //    printf("关节2的电机速度: %lf 弧度/秒\n", w2);

    //    printf("\n正解算结果:\n");
    //    printf("末端位置x: %lf\n", x_forward);
    //    printf("末端位置y: %lf\n", y_forward);

    return 0;
}

// 逆解算函数
void inverse_kinematics(float x, float y, //
                        float l1, float l2, //
                        float *theta1, float *theta2, //
                        int elbow)
{
    // 计算关节2的角度
    const float cos_theta2 = (x * x + y * y - l1 * l1 - l2 * l2) / (2 * l1 * l2);
    *theta2 = acos_approx(cos_theta2);

    // 根据肘部选择确定theta2的符号
    if (elbow == 0) {
        // 肘下
        *theta2 = -acos_approx(cos_theta2);
    }

    // 计算关节1的角度
    const float k1 = l1 + l2 * cos_approx(*theta2);
    const float k2 = l2 * sin_approx(*theta2);
    *theta1 = atan2_approx(y, x) - atan2_approx(k2, k1);
}

// 关节电机速度计算函数
void joint_velocities(float x, float y, //
                      float x_dot, float y_dot, //
                      float l1, float l2, //
                      float theta1, float theta2, //
                      float *w1, float *w2)
{
    float J[2][2]; // 雅可比矩阵

    // 计算雅可比矩阵
    J[0][0] = -l1 * sin_approx(theta1) - l2 * sin_approx(theta1 + theta2);
    J[0][1] = -l2 * sin_approx(theta1 + theta2);
    J[1][0] = l1 * cos_approx(theta1) + l2 * cos_approx(theta1 + theta2);
    J[1][1] = l2 * cos_approx(theta1 + theta2);

    // 计算雅可比矩阵的行列式
    float det_J = J[0][0] * J[1][1] - J[0][1] * J[1][0];

    // 计算雅可比矩阵的逆
    float J_inv[2][2];
    J_inv[0][0] = J[1][1] / det_J;
    J_inv[0][1] = -J[0][1] / det_J;
    J_inv[1][0] = -J[1][0] / det_J;
    J_inv[1][1] = J[0][0] / det_J;

    // 计算关节速度
    *w1 = J_inv[0][0] * x_dot + J_inv[0][1] * y_dot;
    *w2 = J_inv[1][0] * x_dot + J_inv[1][1] * y_dot;
}

// 正解算函数
void forward_kinematics(float theta1, float theta2, //
                        float l1, float l2, //
                        float *x, float *y)
{
    // 计算末端位置
    *x = l1 * cos_approx(theta1) + l2 * cos_approx(theta1 + theta2);
    *y = l1 * sin_approx(theta1) + l2 * sin_approx(theta1 + theta2);
}
