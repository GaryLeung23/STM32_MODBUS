/*
 * FreeModbus Libary: BARE Port
 * Copyright (C) 2006 Christian Walter <wolti@sil.at>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id$
 */

#include "port.h"
#include "usart.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR( void );
static void prvvUARTRxISR( void );

/* ----------------------- Start implementation -----------------------------*/
void
vMBPortSerialEnable( BOOL xRxEnable, BOOL xTxEnable )
{
    /* If xRXEnable enable serial receive interrupts. If xTxENable enable
     * transmitter empty interrupts.
     */
    if(xRxEnable)
    {
        //
        // 如果使用了485控制芯片,那么在此处将485设置为接收模式
        //

        // MAX485_SetMode(MODE_RECV);

        __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);		// 使能接收非空中断
    }
    else
    {
        __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);		// 禁能接收非空中断
    }

    if(xTxEnable)
    {
        //
        // 如果使用了485控制芯片,那么在此处将485设置为发送模式
        //

        // MAX485_SetMode(MODE_SENT);

        __HAL_UART_ENABLE_IT(&huart1, UART_IT_TXE);			// 使能发送为空中断
    }
    else
    {
        __HAL_UART_DISABLE_IT(&huart1, UART_IT_TXE);		// 禁能发送为空中断
    }
}

BOOL
xMBPortSerialInit( UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits, eMBParity eParity )
{
    /**
     * 已经设置串口参数
     */

    return TRUE;
}

BOOL
xMBPortSerialPutByte( CHAR ucByte )
{
      /* Put a byte in the UARTs transmit buffer. This function is called
       * by the protocol stack if pxMBFrameCBTransmitterEmpty( ) has been
       * called. */
    HAL_UART_Transmit(&huart1, (uint8_t *) &ucByte, 1, HAL_MAX_DELAY);

    return TRUE;
}

BOOL
xMBPortSerialGetByte( CHAR * pucByte )
{
    /* Return the byte in the UARTs receive buffer. This function is called
     * by the protocol stack after pxMBFrameCBByteReceived( ) has been called.
     */
    HAL_UART_Receive(&huart1,(uint8_t *)pucByte, 1, HAL_MAX_DELAY);
    return TRUE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */

static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}

//参考HAL_UART_IRQHandler
void USART1_IRQHandler(void)
{
    if(__HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_RXNE)!=RESET)			// 接收非空中断标记被置位
    {

        prvvUARTRxISR();//接受中断								// 通知modbus有数据到达
    }

    if(__HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_TXE)!=RESET)				// 发送为空中断标记被置位
    {

        prvvUARTTxReadyISR();//发送中断									// 通知modbus数据可以发松
    }

    HAL_NVIC_ClearPendingIRQ(USART1_IRQn);
    HAL_UART_IRQHandler(&huart1);
}