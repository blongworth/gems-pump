/**
 * @file test_valve_manager.cpp
 * @brief Unit tests for ValveManager library
 */

#include <unity.h>

// Configure ValveManager before including
#define VALVE_SERVO_PIN 1
#define RED_LED_PIN 39
#define GREEN_LED_PIN 36
#define VALVE_CHANGE_INTERVAL 30
#define LOG_INTERVAL 10
#define MIN_MOVE_INTERVAL 2000
#define VALVE_EXTERNAL_SERIAL Serial2

#include <ValveManager.h>

// Test valve manager creation
void test_valve_manager_creation() {
    ValveManager valveManager;
    
    // Should not be initialized yet
    TEST_ASSERT_FALSE(valveManager.isInitialized());
    
    // Should return 0 for sensor readings when not initialized
    TEST_ASSERT_EQUAL(0, valveManager.getCurrentPosition());
    TEST_ASSERT_EQUAL(0, valveManager.getVoltage());
    TEST_ASSERT_EQUAL(0, valveManager.getCurrent());
}

// Test valve position constants
void test_valve_position_constants() {
    TEST_ASSERT_EQUAL(1205, VALVE_BOTTOM_POS);
    TEST_ASSERT_EQUAL(1795, VALVE_TOP_POS);
    TEST_ASSERT_EQUAL(1500, VALVE_HOME_POS);
}

// Test configuration defines
void test_configuration_defines() {
    TEST_ASSERT_EQUAL(1, VALVE_SERVO_PIN);
    TEST_ASSERT_EQUAL(39, RED_LED_PIN);
    TEST_ASSERT_EQUAL(36, GREEN_LED_PIN);
    TEST_ASSERT_EQUAL(30, VALVE_CHANGE_INTERVAL);
    TEST_ASSERT_EQUAL(10, LOG_INTERVAL);
    TEST_ASSERT_EQUAL(2000, MIN_MOVE_INTERVAL);
}

// Test serial control define
void test_serial_control_define() {
#ifdef SERIAL_CONTROL
    TEST_ASSERT_TRUE(VALVE_SERIAL_CONTROL_ENABLED);
#else
    TEST_ASSERT_FALSE(VALVE_SERIAL_CONTROL_ENABLED);
#endif
}

void setup() {
    // NOTE: This is run on the MCU; Unity output will be on Serial
    delay(2000);
    
    UNITY_BEGIN();
    
    RUN_TEST(test_valve_manager_creation);
    RUN_TEST(test_valve_position_constants);
    RUN_TEST(test_configuration_defines);
    RUN_TEST(test_serial_control_define);
    
    UNITY_END();
}

void loop() {
    // Empty loop
}
