/*
 * FreeModbus Libary: RT-Thread Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
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
 * File: $Id: portserial.c,v 1.60 2013/08/13 15:07:05 Armink $
 */

#include "port.h"
#include "usart.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "bsp.h"

/* ----------------------- Static variables ---------------------------------*/
ALIGN(RT_ALIGN_SIZE)
/* software simulation serial transmit IRQ handler thread stack */
static rt_uint8_t serial_soft_trans_irq_stack[512];
/* software simulation serial transmit IRQ handler thread */
static struct rt_thread thread_serial_soft_trans_irq;
/* serial event */
static struct rt_event event_serial;

static void serial_soft_trans_irq(void* parameter);
/* ----------------------- Defines ------------------------------------------*/
/* serial transmit event */
#define EVENT_SERIAL_TRANS_START    (1<<0)

/* ----------------------- static functions ---------------------------------*/
static void prvvUARTTxReadyISR(void);
static void prvvUARTRxISR(void);


/* ----------------------- Start implementation -----------------------------*/
BOOL xMBPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits,
        eMBParity eParity)
{
    /**
     * set 485 mode receive and transmit control IO
     * @note MODBUS_SLAVE_RT_CONTROL_PIN_INDEX need be defined by user
     */
//    rt_pin_mode(MODBUS_SLAVE_RT_CONTROL_PIN_INDEX, PIN_MODE_OUTPUT);

    /**
     * 已经设置串口参数
     */
    //重新设置

    huart1.Instance = USART1;
    huart1.Init.BaudRate = ulBaudRate;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    switch(eParity)
    {
        // 奇校验
        case MB_PAR_ODD:
            huart1.Init.Parity = UART_PARITY_ODD;
            huart1.Init.WordLength = UART_WORDLENGTH_9B;			// 带奇偶校验数据位为9bits
            break;

            // 偶校验
        case MB_PAR_EVEN:
            huart1.Init.Parity = UART_PARITY_EVEN;
            huart1.Init.WordLength = UART_WORDLENGTH_9B;			// 带奇偶校验数据位为9bits
            break;

            // 无校验
        default:
            huart1.Init.Parity = UART_PARITY_NONE;
            huart1.Init.WordLength = UART_WORDLENGTH_8B;			// 无奇偶校验数据位为8bits
            break;
    }


    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        return FALSE;
    }

    /* software initialize */
    rt_event_init(&event_serial, "slave event", RT_IPC_FLAG_PRIO);
    rt_thread_init(&thread_serial_soft_trans_irq,
                   "slave trans",
                   serial_soft_trans_irq,
                   RT_NULL,
                   serial_soft_trans_irq_stack,
                   sizeof(serial_soft_trans_irq_stack),
                   10, 5);
    rt_thread_startup(&thread_serial_soft_trans_irq);

    return TRUE;
}

void vMBPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
    rt_uint32_t recved_event;
    if (xRxEnable)
    {
        /* enable RX interrupt */
        __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);		// 使能接收非空中断
        /* switch 485 to receive mode */
//        rt_pin_write(MODBUS_SLAVE_RT_CONTROL_PIN_INDEX, PIN_LOW);
    }
    else
    {
        /* switch 485 to transmit mode */
//        rt_pin_write(MODBUS_SLAVE_RT_CONTROL_PIN_INDEX, PIN_HIGH);
        /* disable RX interrupt */
        __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);		// 禁能接收非空中断
    }
    if (xTxEnable)
    {
        /* start serial transmit */
        rt_event_send(&event_serial, EVENT_SERIAL_TRANS_START);
//        __HAL_UART_ENABLE_IT(&huart1, UART_IT_TC);			// 使能发送为空中断
    }
    else
    {
        /* stop serial transmit */
        // clear flag
        rt_event_recv(&event_serial, EVENT_SERIAL_TRANS_START,
                RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 0,
                &recved_event);
//        __HAL_UART_DISABLE_IT(&huart1, UART_IT_TC);		// 禁能发送为空中断
    }
}

void vMBPortClose(void)
{

}

BOOL xMBPortSerialPutByte(CHAR ucByte)
{
    HAL_UART_Transmit(&huart1, (uint8_t *) &ucByte, 1, HAL_MAX_DELAY);
    return TRUE;
}

BOOL xMBPortSerialGetByte(CHAR * pucByte)
{
    HAL_UART_Receive(&huart1,(uint8_t *)pucByte, 1, HAL_MAX_DELAY);
    return TRUE;
}

/*
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call
 * xMBPortSerialPutByte( ) to send the character.
 */
void prvvUARTTxReadyISR(void)
{
    pxMBFrameCBTransmitterEmpty();
}

/*
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
void prvvUARTRxISR(void)
{
    pxMBFrameCBByteReceived();
}

/**
 * Software simulation serial transmit IRQ handler.
 *
 * @param parameter parameter
 */
static void serial_soft_trans_irq(void* parameter) {
    rt_uint32_t recved_event;
    while (1)
    {
        /* waiting for serial transmit start */
        rt_event_recv(&event_serial, EVENT_SERIAL_TRANS_START, RT_EVENT_FLAG_OR,
                RT_WAITING_FOREVER, &recved_event);
        /* execute modbus callback */
        prvvUARTTxReadyISR();
    }
}

//参考HAL_UART_IRQHandler
//void USART1_IRQHandler(void)
//{
//    if(__HAL_UART_GET_IT_SOURCE(&huart1, UART_IT_RXNE)!=RESET)			// 接收非空中断标记被置位
//    {
//
//        prvvUARTRxISR();//接受中断								// 通知modbus有数据到达
//    }
//}