/*!
 * \file      sx1261dvk1bas-board.c
 *
 * \brief     Target board SX1261DVK1BAS shield driver implementation
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
#include <project.h>
//#include "asr_project.h"
#include <stdlib.h>
#include "utilities.h"
#include "board-config.h"
#include "board.h"
#include "delay.h"
#include "timer.h"
#include "sx126x-board.h"
#include "debug.h"

SX126x_t SX126x;

#define ID1 (0x1FF80050)
#define ID2 (0x1FF80054)
#define ID3 (0x1FF80064)

/*!
 * Battery thresholds
 */
//#warning "Change the Battery type before going to PRODUCTION
// #define BATTERY_TYPE 0 // 1= SAFT 3.6v or 0=Lithium 4.2V
// //#define BAT_LEVEL_EMPTY 0x01 // 254
// //#define BAT_LEVEL_FULL 0xFE  // 1
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
 * Antenna switch GPIO pins objects
 */
Gpio_t AntPow;
Gpio_t DeviceSel;
LOG_LEVEL g_log_level = LL_DEBUG;

#ifdef CONFIG_LORA_USE_TCXO
bool UseTCXO = true;
#else
bool UseTCXO = false;
#endif
uint8_t gPaOptSetting = 0;
static uint32_t gBaudRate = STDIO_UART_BAUDRATE;
char gChipId[17];

/*!
 * Flag used to indicate if board is powered from the USB
 */
static bool UsbIsConnected = false;

void SX126xIoInit(void)
{
    GpioInit(&SX126x.Spi.Nss, RADIO_NSS, OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1);
    GpioInit(&SX126x.BUSY, RADIO_BUSY, INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
    GpioInit(&SX126x.DIO1, RADIO_DIO_1, INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
}

void SX126xIoIrqInit(DioIrqHandler dioIrq)
{
    GpioSetInterrupt(&SX126x.DIO1, RISING, IRQ_HIGH_PRIORITY, dioIrq);
}

void SX126xIoDeInit(void)
{
    GpioInit(&SX126x.Spi.Nss, RADIO_NSS, ANALOG, PIN_PUSH_PULL, PIN_PULL_UP, 1);
    GpioInit(&SX126x.BUSY, RADIO_BUSY, ANALOG, PIN_PUSH_PULL, PIN_NO_PULL, 0);
    GpioInit(&SX126x.DIO1, RADIO_DIO_1, ANALOG, PIN_PUSH_PULL, PIN_NO_PULL, 0);
}

uint32_t SX126xGetBoardTcxoWakeupTime(void)
{
    return BOARD_TCXO_WAKEUP_TIME;
}

void SX126xReset(void)
{
    pinMode(SX126x.Reset.pin, OUTPUT);
    DelayMs(20);
    GpioInit(&SX126x.Reset, RADIO_RESET, OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
    DelayMs(40);
    GpioInit(&SX126x.Reset, RADIO_RESET, OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1); // internal pull-up
    DelayMs(20);
    pinMode(SX126x.Reset.pin, ANALOG);
}

void SX126xWaitOnBusy(void)
{
    while (GpioRead(&SX126x.BUSY) == 1)
        ;
}

void SX126xWakeup(void)
{
    BoardDisableIrq();

    GpioWrite(&SX126x.Spi.Nss, 0);
    SpiInOut(&SX126x.Spi, RADIO_GET_STATUS);
    SpiInOut(&SX126x.Spi, 0x00);

    GpioWrite(&SX126x.Spi.Nss, 1);

    // Wait for chip to be ready.
    SX126xWaitOnBusy();

    BoardEnableIrq();
}

void SX126xWriteCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size)
{

    SX126xCheckDeviceReady();
    GpioWrite(&SX126x.Spi.Nss, 0);

    SpiInOut(&SX126x.Spi, (uint8_t)command);

    for (uint16_t i = 0; i < size; i++)
    {
        SpiInOut(&SX126x.Spi, buffer[i]);
    }

    GpioWrite(&SX126x.Spi.Nss, 1);

    if (command != RADIO_SET_SLEEP)
    {
        SX126xWaitOnBusy();
    }
}

void SX126xReadCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size)
{

    SX126xCheckDeviceReady();
    GpioWrite(&SX126x.Spi.Nss, 0);

    SpiInOut(&SX126x.Spi, (uint8_t)command);
    SpiInOut(&SX126x.Spi, 0x00);
    for (uint16_t i = 0; i < size; i++)
    {
        buffer[i] = SpiInOut(&SX126x.Spi, 0);
    }

    GpioWrite(&SX126x.Spi.Nss, 1);

    SX126xWaitOnBusy();
}

void SX126xWriteRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
    SX126xCheckDeviceReady();

    GpioWrite(&SX126x.Spi.Nss, 0);

    SpiInOut(&SX126x.Spi, RADIO_WRITE_REGISTER);
    SpiInOut(&SX126x.Spi, (address & 0xFF00) >> 8);
    SpiInOut(&SX126x.Spi, address & 0x00FF);

    for (uint16_t i = 0; i < size; i++)
    {
        SpiInOut(&SX126x.Spi, buffer[i]);
    }

    GpioWrite(&SX126x.Spi.Nss, 1);

    SX126xWaitOnBusy();
}

void SX126xWriteRegister(uint16_t address, uint8_t value)
{
    SX126xWriteRegisters(address, &value, 1);
}

void SX126xReadRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
    SX126xCheckDeviceReady();

    GpioWrite(&SX126x.Spi.Nss, 0);

    SpiInOut(&SX126x.Spi, RADIO_READ_REGISTER);
    SpiInOut(&SX126x.Spi, (address & 0xFF00) >> 8);
    SpiInOut(&SX126x.Spi, address & 0x00FF);
    SpiInOut(&SX126x.Spi, 0);
    for (uint16_t i = 0; i < size; i++)
    {
        buffer[i] = SpiInOut(&SX126x.Spi, 0);
    }
    GpioWrite(&SX126x.Spi.Nss, 1);

    SX126xWaitOnBusy();
}

uint8_t SX126xReadRegister(uint16_t address)
{
    uint8_t data;
    SX126xReadRegisters(address, &data, 1);
    return data;
}

void SX126xWriteBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{

    SX126xCheckDeviceReady();

    GpioWrite(&SX126x.Spi.Nss, 0);

    SpiInOut(&SX126x.Spi, RADIO_WRITE_BUFFER);
    SpiInOut(&SX126x.Spi, offset);
    for (uint16_t i = 0; i < size; i++)
    {
        SpiInOut(&SX126x.Spi, buffer[i]);
    }
    GpioWrite(&SX126x.Spi.Nss, 1);

    SX126xWaitOnBusy();
}

void SX126xReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
    SX126xCheckDeviceReady();

    GpioWrite(&SX126x.Spi.Nss, 0);

    SpiInOut(&SX126x.Spi, RADIO_READ_BUFFER);
    SpiInOut(&SX126x.Spi, offset);
    SpiInOut(&SX126x.Spi, 0);
    for (uint16_t i = 0; i < size; i++)
    {
        buffer[i] = SpiInOut(&SX126x.Spi, 0);
    }
    GpioWrite(&SX126x.Spi.Nss, 1);

    SX126xWaitOnBusy();
}

void SX126xSetRfTxPower(int8_t power)
{
    SX126xSetTxParams(power, RADIO_RAMP_200_US);
}

uint8_t SX126xGetPaSelect(uint32_t channel)
{
    (void)channel;
    return SX1262;
}

uint8_t SX126xGetPaOpt()
{
    return gPaOptSetting;
}

void SX126xSetPaOpt(uint8_t opt)
{
    if (opt > 3)
        return;

    gPaOptSetting = opt;
}

void SX126xAntSwOn(void)
{
    // GpioInit( &AntPow, RADIO_ANT_SWITCH_POWER, OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
}

void SX126xAntSwOff(void)
{
    // GpioInit( &AntPow, RADIO_ANT_SWITCH_POWER, ANALOG, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
}

bool SX126xCheckRfFrequency(uint32_t frequency)
{
    (void)frequency;
    // Implement check. Currently all frequencies are supported
    return true;
}

void BoardDisableIrq(void)
{
    CyGlobalIntDisable;
}

void BoardEnableIrq(void)
{
    CyGlobalIntEnable;
}

void DelayMsMcu(uint32_t ms)
{
    CyDelay(ms);
}

static char *olds = NULL;
extern void *rawmemchr(__const void *__s, int __c);
char *strtok_l(char *s, const char *delim)
{
    char *token;

    if (s == NULL)
        s = olds;

    s += strspn(s, delim);
    if (*s == '\0')
    {
        olds = s;
        return NULL;
    }

    token = s;
    s = strpbrk(token, delim);
    if (s == NULL)
        olds = rawmemchr(token, '\0');
    else
    {
        *s = '\0';
        olds = s + 1;
    }
    return token;
}

static const double huge = 1.0e300;
#define __HI(x) *(1 + (int *)&x)
#define __LO(x) *(int *)&x

static const double TWO52[2] = {
    4.50359962737049600000e+15,  /* 0x43300000, 0x00000000 */
    -4.50359962737049600000e+15, /* 0xC3300000, 0x00000000 */
};

extern uint32_t systime;

void boardInitMcu(void)
{
    SpiInit();
    Asr_Timer_Init();
    RtcInit();
    systime = millis();
#if defined(CubeCell_Board) || defined(CubeCell_Capsule) || defined(CubeCell_BoardPlus) || defined(CubeCell_GPS) || defined(CubeCell_HalfAA)
    pinMode(Vext, OUTPUT);
    digitalWrite(Vext, HIGH);

    /*
     * Board, BoardPlus, Capsule, GPS and HalfAA variants
     * have external 10K VDD pullup resistor
     * connected to GPIO7 (USER_KEY / VBAT_ADC_CTL) pin
     */
    pinMode(VBAT_ADC_CTL, INPUT);
#endif
    SX126xIoInit();
    delay(10);
    SX126xReset();
    delay(10);
    sx126xSleep();
}

void DBG_LogLevelSet(int level)
{
    g_log_level = level;
}

int DBG_LogLevelGet()
{
    return g_log_level;
}

char *HW_Get_MFT_ID(void)
{
    return CONFIG_MANUFACTURER;
}

char *HW_Get_MFT_Model(void)
{
    return CONFIG_DEVICE_MODEL;
}

char *HW_Get_MFT_Rev(void)
{
    return CONFIG_VERSION;
}

char *HW_Get_MFT_SN(void)
{
    uint32_t id[2];
    CyGetUniqueId(id);
    sprintf(gChipId, "%08X%08X", (unsigned int)id[0], (unsigned int)id[1]);
    return gChipId;
}

bool HW_Set_MFT_Baud(uint32_t baud)
{
    uint32_t div = (float)CYDEV_BCLK__HFCLK__HZ / baud / UART_1_UART_OVS_FACTOR + 0.5 - 1;
    UART_1_SCBCLK_DIV_REG = div << 8;
    UART_1_SCBCLK_CMD_REG = 0x8000FF41u;

    gBaudRate = baud;
    return true;
}

uint32_t HW_Get_MFT_Baud(void)
{
    return gBaudRate;
}

void HW_Reset(int mode)
{
    if (mode == 0)
    {
        CySoftwareReset();
    }
    else if (mode == 1)
    {
        Bootloadable_1_Load();
    }
}

uint32_t BoardGetRandomSeed(void)
{
    return ((*(uint32_t *)ID1) ^ (*(uint32_t *)ID2) ^ (*(uint32_t *)ID3));
}

uint8_t GetBoardPowerSource(void)
{
    if (UsbIsConnected == false)
    {
        return BATTERY_POWER;
    }
    else
    {
        return USB_POWER;
    }
}

/*  get the BatteryVoltage in mV. */
uint16_t BoardGetBatteryVoltage(void)
{
    float temp = 0;
    uint16_t volt;
    uint8_t pin;
    pin = ADC;

#if defined(CubeCell_Board) || defined(CubeCell_Capsule) || defined(CubeCell_BoardPlus) || defined(CubeCell_BoardPRO) || defined(CubeCell_GPS) || defined(CubeCell_HalfAA)
    /*
     * have external 10K VDD pullup resistor
     * connected to VBAT_ADC_CTL pin
     */

    pinMode(VBAT_ADC_CTL, OUTPUT);
    digitalWrite(VBAT_ADC_CTL, LOW);
#endif
    for (int i = 0; i < 50; i++) // read 50 times and get average
        temp += analogReadmV(pin);
    volt = temp / 50;

#if defined(CubeCell_Board) || defined(CubeCell_Capsule) || defined(CubeCell_BoardPlus) || defined(CubeCell_BoardPRO) || defined(CubeCell_GPS) || defined(CubeCell_HalfAA)
    pinMode(VBAT_ADC_CTL, INPUT);
#endif

    volt = volt * 2;
    return volt;
}

uint8_t BoardGetBatteryLevel(void)
{
    uint8_t batteryLevel = 0;

    BatteryVoltage = BoardGetBatteryVoltage();

    if (GetBoardPowerSource() == USB_POWER)
    {
        batteryLevel = BATTERY_LORAWAN_EXT_PWR;
    }
    else
    {
        if (BatteryVoltage >= BATTERY_MAX_LEVEL)
        {
            batteryLevel = BATTERY_LORAWAN_MAX_LEVEL;
        }
        else if ((BatteryVoltage > BATTERY_MIN_LEVEL) && (BatteryVoltage < BATTERY_MAX_LEVEL))
        {
            batteryLevel =
                ((253 * (BatteryVoltage - BATTERY_MIN_LEVEL)) / (BATTERY_MAX_LEVEL - BATTERY_MIN_LEVEL)) + 1;
        }
        else if ((BatteryVoltage >= BATTERY_SHUTDOWN_LEVEL) && (BatteryVoltage <= BATTERY_MIN_LEVEL))
        {
            batteryLevel = 1;
        }
        else // if( BatteryVoltage <= BATTERY_SHUTDOWN_LEVEL )
        {
            batteryLevel = BATTERY_LORAWAN_UNKNOWN_LEVEL;
        }
    }
    return batteryLevel;
}

// uint8_t BoardGetBatteryLevel(void)
// {
//     // 5.5 End-Device Status (DevStatusReq, DevStatusAns)
//     // 0      The end-device is connected to an external power source.
//     // 1..254 The battery level, 1 being at minimum and 254 being at maximum
//     // 255    The end-device was not able to measure the battery level.
//     const uint32_t batteryVoltage = BoardGetBatteryVoltage();
//     //printf("\nThe end-device BoardGetBatteryVoltage():%d \r\n", batteryVoltage);
//     uint8_t batteryLevel = 0;
//     //#if (BATTERY_TYPE)
//     //   const double maxBattery = 3.6; // 3.6 for saft LS17500 ;//4.2 for lithium; //must update to match SAFT 3.6 max voltage
//     //   const double minBattery = 2.2; // 2.4 minimum for saft LS17500 and 3.4 minimum for lithium
//     //#else
//     const double maxBattery = BATTERY_LEVEL_MAX; // / 1000; // 4.1; // 4.2 for lithium; //must update to match SAFT 3.6 max voltage
//     const double minBattery = BATTERY_LEVEL_MIN; // / 1000; //3.3; // 2.4 minimum for saft LS17500 and 3.4 minimum for lithium
//                                                  //#endif
//     const double batVoltage = fmax(minBattery, fmin(maxBattery, batteryVoltage / 1000.0));

//     //printf("\nThe end-device BoardGetBatteryVoltage():%d batVoltage:%f \r\n", batteryVoltage, batVoltage);

//     // power plugged in
//     if (batteryVoltage > (uint32_t)(BATTERY_LEVEL_MAX * 1000))
//     {
//         batteryLevel = 0;
//     } // measure power level
//     else if ((batteryVoltage > (uint32_t)(BATTERY_LEVEL_MIN * 1000)) && (batteryVoltage < (uint32_t)(BATTERY_LEVEL_MAX * 1000)))
//     {
//         batteryLevel = BATERRY_LEVEL_EMPTY + ((batVoltage - minBattery) / (maxBattery - minBattery)) * (BATERRT_LEVEL_FULL - BATERRY_LEVEL_EMPTY);
//     }
//     else if ((batteryVoltage <= (uint32_t)(BATTERY_LEVEL_SHUTDOWN * 1000))) // || (batteryVoltage <= BATTERY_LEVEL_SHUTDOWN))
//     {
//         batteryLevel = 1;
//     }
//     else
//     {
//         batteryLevel = 255; // invalid input
//     }
//     // printf("battery level (1-254): %u\n", batlevel);
//     return batteryLevel;
// }
