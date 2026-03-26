/**
 * @file pid_test.cpp
 * @brief Unit tests for PID controller
 */

#include <gtest/gtest.h>

// Wrap C headers for C++
extern "C" {
#include "pid.h"
}

// Test fixture for PID controller tests
class PIDTest : public ::testing::Test {
protected:
    pid_type_def pid;

    void SetUp() override {
        // Initialize PID with test parameters
        float pid_params[4] = {1.0f, 0.1f, 0.05f, 0.0f};  // Kp, Ki, Kd, Kf
        PID_init(&pid, PID_POSITION, pid_params, 100.0f, 50.0f);
    }
};

// Test basic PID initialization
TEST_F(PIDTest, InitTest) {
    EXPECT_EQ(pid.mode, PID_POSITION);
    EXPECT_FLOAT_EQ(pid.Kp, 1.0f);
    EXPECT_FLOAT_EQ(pid.Ki, 0.1f);
    EXPECT_FLOAT_EQ(pid.Kd, 0.05f);
    EXPECT_FLOAT_EQ(pid.max_out, 100.0f);
    EXPECT_FLOAT_EQ(pid.max_iout, 50.0f);
}

// Test PID calculation with no error
TEST_F(PIDTest, NoErrorTest) {
    float output = PID_calc(&pid, 50.0f, 50.0f);  // ref = 50, set = 50, error = 0
    EXPECT_FLOAT_EQ(output, 0.0f);
}

// Test PID calculation with positive error
TEST_F(PIDTest, PositiveErrorTest) {
    float output = PID_calc(&pid, 40.0f, 60.0f);  // ref = 40, set = 60, error = 20
    // Pout = Kp * error = 1.0 * 20 = 20
    // Iout = Ki * error = 0.1 * 20 = 2
    // Dout = Kd * (error - prev_error) = 0.05 * 20 = 1 (first time, prev_error = 0)
    // Expected output = 20 + 2 + 1 = 23
    EXPECT_GT(output, 0.0f);
}

// Test PID calculation with negative error
TEST_F(PIDTest, NegativeErrorTest) {
    float output = PID_calc(&pid, 60.0f, 40.0f);  // ref = 60, set = 40, error = -20
    EXPECT_LT(output, 0.0f);
}

// Test output limiting
TEST_F(PIDTest, OutputLimitTest) {
    // Reset PID with very high gain to trigger output limit
    float pid_params[4] = {100.0f, 0.0f, 0.0f, 0.0f};
    PID_init(&pid, PID_POSITION, pid_params, 100.0f, 50.0f);

    float output = PID_calc(&pid, 0.0f, 10.0f);  // error = 10, Pout = 1000
    EXPECT_LE(output, 100.0f);
    EXPECT_GE(output, -100.0f);
}

// Test integral limiting
TEST_F(PIDTest, IntegralLimitTest) {
    // Reset PID with high integral gain
    float pid_params[4] = {0.0f, 10.0f, 0.0f, 0.0f};
    PID_init(&pid, PID_POSITION, pid_params, 100.0f, 20.0f);

    // First call: Iout = 10 * 10 = 100, but limited to 20
    float output1 = PID_calc(&pid, 0.0f, 10.0f);
    EXPECT_LE(output1, 20.0f);

    // Second call: Iout = previous Iout + 10 * 10 = 20 + 100 = 120, but limited to 20
    float output2 = PID_calc(&pid, 0.0f, 10.0f);
    EXPECT_LE(output2, 20.0f);
}

// Test PID clear
TEST_F(PIDTest, ClearTest) {
    // First calculate some output
    PID_calc(&pid, 40.0f, 60.0f);

    // Clear the PID
    PID_clear(&pid);

    // Check that all values are reset
    EXPECT_FLOAT_EQ(pid.out, 0.0f);
    EXPECT_FLOAT_EQ(pid.Pout, 0.0f);
    EXPECT_FLOAT_EQ(pid.Iout, 0.0f);
    EXPECT_FLOAT_EQ(pid.Dout, 0.0f);
    EXPECT_FLOAT_EQ(pid.error[0], 0.0f);
    EXPECT_FLOAT_EQ(pid.error[1], 0.0f);
    EXPECT_FLOAT_EQ(pid.error[2], 0.0f);
}

// Test NULL pointer handling
TEST(PIDNullTest, NullPointerTest) {
    float output = PID_calc(nullptr, 50.0f, 50.0f);
    EXPECT_FLOAT_EQ(output, 0.0f);

    PID_init(nullptr, PID_POSITION, nullptr, 0.0f, 0.0f);
    PID_clear(nullptr);
}

// Test delta PID mode
TEST(PIDDeltaTest, DeltaModeTest) {
    pid_type_def delta_pid;
    float pid_params[4] = {1.0f, 0.1f, 0.05f, 0.0f};
    PID_init(&delta_pid, PID_DELTA, pid_params, 100.0f, 50.0f);

    EXPECT_EQ(delta_pid.mode, PID_DELTA);

    // First call with delta PID
    float output1 = PID_calc(&delta_pid, 50.0f, 55.0f);  // error = 5
    // For delta PID: Pout = Kp * (error - prev_error) = 1.0 * 5 = 5
    EXPECT_GT(output1, 0.0f);

    // Second call
    float output2 = PID_calc(&delta_pid, 52.0f, 55.0f);  // error = 3
    // Pout = Kp * (error - prev_error) = 1.0 * (3 - 5) = -2
    EXPECT_LT(output2, output1);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
