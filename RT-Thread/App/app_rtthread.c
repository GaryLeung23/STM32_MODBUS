#include <rtthread.h>
#include <rthw.h>
#include "main.h"
#include <stdio.h>
#include "bsp.h"
#include "user_mb_app.h"

//====================操作系统各线程优先级==================================
#define THREAD_LED_PRIO              11
#define THREAD_MODBUSSLAVEPOLL_PRIO         10
#define THREAD_MODBUSMASTERPOLL_POLL         9
//====================操作系统各线程堆栈====================================
ALIGN(RT_ALIGN_SIZE)
static rt_uint8_t thread_LedPoll_stack[256];
static rt_uint8_t thread_ModbusSlavePoll_stack[512];
static rt_uint8_t thread_ModbusMasterPoll_stack[512];

//====================操作系统各线程id====================================
static struct rt_thread thread_LedPoll;
static struct rt_thread thread_ModbusSlavePoll;
static struct rt_thread thread_ModbusMasterPoll;
//====================操作系统各线程介绍====================================

USHORT  usModbusUserData[MB_PDU_SIZE_MAX];
UCHAR   ucModbusUserData[MB_PDU_SIZE_MAX];

void thread_entry_LedPoll(void *parameter){
    eMBMasterReqErrCode    errorCode = MB_MRE_NO_ERR;
    while(1){
        usModbusUserData[0] = (USHORT)(rt_tick_get()/10);
        usModbusUserData[1] = (USHORT)(rt_tick_get()%10);
        ucModbusUserData[0] = 0xFF;
        rt_thread_delay(500);
        errorCode = eMBMasterReqWriteMultipleCoils(1,8,8,ucModbusUserData,RT_WAITING_FOREVER);
//        errorCode = eMBMasterReqWriteHoldingRegister(1,3,usModbusUserData[0],RT_WAITING_FOREVER);
        if(errorCode != MB_MRE_NO_ERR){
            HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
        }
        else{
            HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
        }
        rt_thread_delay(500);
    }
}
void thread_entry_ModbusSlavePoll(void* parameter)
{
    eMBInit(MB_RTU, 0x01, 1, 115200,  MB_PAR_EVEN);
    eMBEnable();
    while (1)
    {
        eMBPoll();
    }
}
void thread_entry_ModbusMasterPoll(void* parameter)
{
    eMBMasterInit(MB_RTU, 1, 115200,  MB_PAR_EVEN); //没有slave addr
    eMBMasterEnable();
    while (1)
    {
        eMBMasterPoll();
    }
}

//**********************系统初始化函数********************************
//函数定义: int rt_application_init(void)
//入口参数：无
//出口参数：return 0
//********************************************************************
int rt_application_init(void)
{
    //初始化LED线程
    rt_thread_init(&thread_LedPoll,"led",thread_entry_LedPoll,RT_NULL,thread_LedPoll_stack,sizeof(thread_LedPoll_stack),THREAD_LED_PRIO,20);
    //开启线程调度
    rt_thread_startup(&thread_LedPoll);

    //这里因为没有第二个串口，所以之只能slave与master分开使用
    //Master
    rt_thread_init(&thread_ModbusMasterPoll, "MBMasterPoll",
                   thread_entry_ModbusMasterPoll, RT_NULL, thread_ModbusMasterPoll_stack,
                   sizeof(thread_ModbusMasterPoll_stack), THREAD_MODBUSMASTERPOLL_POLL,
                   5);
    rt_thread_startup(&thread_ModbusMasterPoll);
    //Slave
//    rt_thread_init(&thread_ModbusSlavePoll, "MBSlavePoll",
//                   thread_entry_ModbusSlavePoll, RT_NULL, thread_ModbusSlavePoll_stack,
//                   sizeof(thread_ModbusSlavePoll_stack), THREAD_MODBUSSLAVEPOLL_PRIO,
//                   20);
//    rt_thread_startup(&thread_ModbusSlavePoll);


    return 0;
}


//**************************初始化RT-Thread函数*************************************
//函数定义: void rtthread_startup(void)
//入口参数：无
//出口参数：return 0,但是不会reach here
//备    注：Editor：Armink 2011-04-04    Company: BXXJS
//**********************************************************************************
int rtthread_startup(void)
{
    rt_hw_interrupt_disable();

    /* board level initialization
     * NOTE: please initialize heap inside board initialization.
     */
    rt_hw_board_init();

    /* show RT-Thread version */
    rt_show_version();

    /* timer system initialization */
    rt_system_timer_init();

    /* scheduler system initialization */
    rt_system_scheduler_init();

    /* create init_thread */
    rt_application_init();

    /* timer thread initialization */
    rt_system_timer_thread_init();

    /* idle thread initialization */
    rt_thread_idle_init();

    /* start scheduler */
    rt_system_scheduler_start();

    /* never reach here */
    return 0;
}