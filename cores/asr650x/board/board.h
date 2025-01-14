/*!
 * \file      board.h
 *
 * \brief     Target board general functions implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#ifndef __BOARD_H__
#define __BOARD_H__

#include <stdint.h>

#ifdef BOOTLOADER
#define STDIO_UART 0
#define STDIO_UART_BAUDRATE 115200
#else
#define STDIO_UART 0
#define STDIO_UART_BAUDRATE 115200
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/*!
 * Battery thresholds
 */
#warning "Change the Battery type before going to PRODUCTION
#define BATTERY_TYPE 0 // 1= SAFT 3.6v or 0=Lithium 4.2V
//#define BATERRY_LEVEL_EMPTY 0x01
//#define BATERRT_LEVEL_FULL 0xEF
#if (BATTERY_TYPE)
#define BATTERY_MAX_LEVEL 3600      // mV 3.6 for saft LS17500 and 4200 for lithium
#define BATTERY_MIN_LEVEL 2300      // mV 2.2 minimum for saft LS17500 and 3.4 minimum for lithium
#define BATTERY_SHUTDOWN_LEVEL 2200 // mV 2.1 shutdown for saft LS17500 and 3.3 minimum for lithium
#else
#define BATTERY_MAX_LEVEL 4100      // 4.1       // mV 3.6 for saft LS17500 and 4100 for lithium
#define BATTERY_MIN_LEVEL 3400      // 3.4       // mV 2.4 minimum for saft LS17500 and 3.4 minimum for lithium
#define BATTERY_SHUTDOWN_LEVEL 3300 // 3.31 // mV 2.3 minimum for saft LS17500 and 3.3 minimum for lithium
#endif

#define BATTERY_LORAWAN_UNKNOWN_LEVEL 255
#define BATTERY_LORAWAN_MAX_LEVEL 254
#define BATTERY_LORAWAN_MIN_LEVEL 1
#define BATTERY_LORAWAN_EXT_PWR 0

    static uint16_t BatteryVoltage = BATTERY_MAX_LEVEL;

    /*!
     * Battery thresholds
     */
    // #warning "Change the Battery type before going to PRODUCTION
    // #define BATTERY_TYPE 0       // 1= SAFT 3.6v or 0=Lithium 4.2V
    // #define BAT_LEVEL_EMPTY 0x01 // 254
    // #define BAT_LEVEL_FULL 0xFE  // 1
    // #if (BATTERY_TYPE)
    // #define BATTERY_MAX_LEVEL 3600      // mV 3.6 for saft LS17500 and 4200 for lithium
    // #define BATTERY_MIN_LEVEL 2200      // mV 2.2 minimum for saft LS17500 and 3.4 minimum for lithium
    // #define BATTERY_SHUTDOWN_LEVEL 2100 // mV 2.1 shutdown for saft LS17500 and 3.3 minimum for lithium
    // #else
    // #define BATTERY_MAX_LEVEL 4100      // mV 3.6 for saft LS17500 and 4100 for lithium
    // #define BATTERY_MIN_LEVEL 3400      // mV 2.4 minimum for saft LS17500 and 3.4 minimum for lithium
    // #define BATTERY_SHUTDOWN_LEVEL 3300 // mV 2.3 minimum for saft LS17500 and 3.3 minimum for lithium
    // #endif

    /*!
     * Possible power sources
     */
    enum BoardPowerSources
    {
        USB_POWER = 0,
        BATTERY_POWER,
    };

    /*!
     * Board Version
     */
    typedef union BoardVersion_u
    {
        struct BoardVersion_s
        {
            uint8_t Rfu;
            uint8_t Revision;
            uint8_t Minor;
            uint8_t Major;
        } Fields;
        uint32_t Value;
    } BoardVersion_t;

    /*!
     * \brief Disable interrupts
     *
     * \remark IRQ nesting is managed
     */
    void BoardDisableIrq(void);

    /*!
     * \brief Enable interrupts
     *
     * \remark IRQ nesting is managed
     */
    void BoardEnableIrq(void);

    /*!
     * \brief Initializes the mcu.
     */
    void boardInitMcu(void);

    /*!
     * \brief Resets the mcu.
     */
    void BoardResetMcu(void);

    /*!
     * \brief Initializes the boards peripherals.
     */
    void BoardInitPeriph(void);

    /*!
     * \brief De-initializes the target board peripherals to decrease power
     *        consumption.
     */
    void BoardDeInitMcu(void);

    /*!
     * \brief Gets the current potentiometer level value
     *
     * \retval value  Potentiometer level ( value in percent )
     */
    uint8_t BoardGetPotiLevel(void);

    /*!
     * \brief Measure the Battery voltage
     *
     * \retval value  battery voltage in volts
     */
    uint16_t BoardGetBatteryVoltage(void);

    /*!
     * \brief Get the current battery level
     *
     * \retval value  battery level [  0: USB,
     *                                 1: Min level,
     *                                 x: level
     *                               254: fully charged,
     *                               255: Error]
     */
    uint8_t BoardGetBatteryLevel(void);

    /*!
     * Returns a pseudo random seed generated using the MCU Unique ID
     *
     * \retval seed Generated pseudo random seed
     */
    uint32_t BoardGetRandomSeed(void);

    /*!
     * \brief Gets the board 64 bits unique ID
     *
     * \param [IN] id Pointer to an array that will contain the Unique ID
     */
    void BoardGetUniqueId(uint8_t *id);

    /*!
     * \brief Get the board power source
     *
     * \retval value  power source [0: USB_POWER, 1: BATTERY_POWER]
     */
    uint8_t GetBoardPowerSource(void);

    /*!
     * \brief Get the board version
     *
     * \retval value  Version
     */
    BoardVersion_t BoardGetVersion(void);

    extern int FLASH_update(uint32_t dst_addr, const void *data, uint32_t size);
    extern int FLASH_read_at(uint32_t address, uint8_t *pData, uint32_t len_bytes);

#ifdef __cplusplus
}
#endif

#endif // __BOARD_H__
