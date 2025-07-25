#pragma once
#include <stdint.h>

/**
 * @file timing.h
 * @brief Timing and delay functions for the kernel
 * 
 * This module provides accurate timing functionality using the
 * Time Stamp Counter (TSC) for precise delays and timing measurements.
 */

/**
 * @brief Read the Time Stamp Counter (TSC)
 * @return Current TSC value as a 64-bit unsigned integer
 */
uint64_t timing_read_tsc(void);

/**
 * @brief Calibrate the TSC frequency
 * @return Estimated TSC ticks per second
 */
uint64_t timing_calibrate_tsc_frequency(void);

/**
 * @brief Delay execution for a specified number of seconds
 * @param seconds Number of seconds to delay
 */
void timing_delay_seconds(uint32_t seconds);

/**
 * @brief Delay execution for a specified number of milliseconds
 * @param milliseconds Number of milliseconds to delay
 */
void timing_delay_milliseconds(uint32_t milliseconds);

/**
 * @brief Get elapsed time in TSC ticks since a given start point
 * @param start_tsc Starting TSC value
 * @return Number of TSC ticks elapsed
 */
uint64_t timing_get_elapsed_ticks(uint64_t start_tsc);

/**
 * @brief Initialize the timing subsystem
 * Performs initial TSC frequency calibration
 */
void timing_init(void);
