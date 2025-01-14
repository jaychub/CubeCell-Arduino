/*******************************************************************************
 * @file    low_power.c
 * @author  MCD Application Team
 * @version V1.1.1
 * @date    01-June-2017
 * @brief   driver for low power
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics International N.V.
 * All rights reserved.</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted, provided that the following conditions are met:
 *
 * 1. Redistribution of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of other
 *    contributors to this software may be used to endorse or promote products
 *    derived from this software without specific written permission.
 * 4. This software, including modifications and/or derivative works of this
 *    software, must execute solely and exclusively on microcontroller or
 *    microprocessor devices manufactured by or for STMicroelectronics.
 * 5. Redistribution and use of this software other than as permitted under
 *    this license is void and will automatically terminate your rights under
 *    this license.
 *
 * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
 * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT
 * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "hw.h"
#include "low_power.h"

/* Private function prototypes -----------------------------------------------*/
extern bool wakeByUart;
extern uint32_t systime;
extern uint8 UART_1_initVar;
uint8_t HasLoopedThroughMain = 0;

void TimerLowPowerHandler(bool *wokeUp)
{
    if (HasLoopedThroughMain < 5)
    {
        HasLoopedThroughMain++;
        *wokeUp = false;
    }
    else
    {
        *wokeUp = true;
        HasLoopedThroughMain = 0;
        lowPowerHandler();
    }
}
void lowPowerHandler(void)
{
    bool wdtState; // keep track of whether the watchdog was enabled.
    if (wakeByUart == false)
    {
        pinMode(P4_1, ANALOG); // SPI0  MISO;
        if (UART_1_initVar)
            pinMode(P3_1, ANALOG);             // UART_TX
        wdtState = CySysWdtGetEnabledStatus(); // get the status of the wdt whether it is enabled by the user or not
        if (wdtState)
            CySysWdtDisable(); // if it was enabled then disable it
        // TODO: test _Sleep() https://manualzz.com/doc/o/epbds/psoc-creator-system-reference-guide-apis
        CySysPmDeepSleep(); // go to deep sleep use CySysPmSleep() for sleep mode if

        if (wdtState)
            CySysWdtEnable(); // restore the status of the wdt to enabled only if was enabled by the user

        systime = (uint32_t)RtcGetTimerValue();
        pinMode(P4_1, INPUT);
        if (UART_1_initVar)
            pinMode(P3_1, OUTPUT_PULLUP);
    }
}
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
