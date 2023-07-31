#include <rtthread.h>
#include <rthw.h>
#include "main.h"
#include <stdio.h>
#include "bsp.h"


//====================操作系统各线程优先级==================================
#define THREAD_LED_PRIO              11

//====================操作系统各线程堆栈====================================
static rt_uint8_t rt_led_thread_stack[128];

//====================操作系统各线程id====================================
static struct rt_thread led_thread;

//====================操作系统各线程介绍====================================

void led_task_entry(void *parameter){
    while(1){
        HAL_GPIO_TogglePin(LED0_GPIO_Port,LED0_Pin);
        rt_thread_delay(500);
        HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
        rt_thread_delay(500);
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
    rt_thread_init(&led_thread,"led",led_task_entry,RT_NULL,rt_led_thread_stack,sizeof(rt_led_thread_stack),THREAD_LED_PRIO,20);
    //开启线程调度
    rt_thread_startup(&led_thread);

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