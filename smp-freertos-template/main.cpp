
//
// vim: ts=4 et
// Copyright (c) 2022 Petr Vanek, petr@fotoventus.cz
//
/// @file   main.cpp
/// @author Petr Vanek
//
// @brief - simple example

#include "pico/stdlib.h"
#include <stdio.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <timers.h>

SemaphoreHandle_t g_chars_mutex = NULL;     // synchronize printing chars
TimerHandle_t g_oneShotHandleTimer = NULL;  // timer handler - one shot
TimerHandle_t g_fiveShotHandleTimer = NULL; // timer handle multiple shot
TaskHandle_t g_taskHandleO = NULL;          // taks handle for printing 0 chars
TaskHandle_t g_taskHandleX = NULL;          // taks handle for printing X chars
QueueHandle_t g_readWriteQueue = NULL;      // read / write queue

void taskBlink(void *params)
{
    while (true)
    {
        gpio_put(PICO_DEFAULT_LED_PIN, 1);
        vTaskDelay(50);
        gpio_put(PICO_DEFAULT_LED_PIN, 0);
        vTaskDelay(50);
    }
}

void taskReader(void *params)
{
    while (true)
    {

        // number of items in the queue
        auto items = uxQueueMessagesWaiting(g_readWriteQueue);
        if (items != 0)
        {
            printf("Queue items count = %d\n", items);
        }

        int32_t received = 0;

        auto status = xQueueReceive(g_readWriteQueue, &received, pdMS_TO_TICKS(100UL));

        if (status == pdPASS)
        {
            printf("Received = %d\n", received);
        }
    }
}

void taskWriter(void *params)
{
    auto val = (int32_t)params;
    while (true)
    {
        auto status = xQueueSendToBack(g_readWriteQueue, &val, 0);
        val++;
        vTaskDelay(500);
    }
}

/**
 * @brief job to print 10 characters on serial line.
 * Printing is completed in a locked section using mutex to prevent another task from interfering with the printing
 *
 * @param pvParameters - pointer to char
 */
void taskPrintBunchOfChars(void *pvParameters)
{
    char ch = *(char *)pvParameters;

    while (true)
    {
        if (xSemaphoreTake(g_chars_mutex, 0) == pdTRUE)
        {
            for (auto i = 0; i < 10; i++)
            {
                putchar(ch);
            }
            puts("");
            xSemaphoreGive(g_chars_mutex);
        }
        vTaskDelay(100);
    }
}

/**
 * @brief callback function for timers
 *
 * @param xTimer
 */
void timerCallback(TimerHandle_t xTimer)
{
    TickType_t ticks;
    uint32_t count;

    // get internal ID as counter
    count = (uint32_t)pvTimerGetTimerID(xTimer);
    count++;
    vTimerSetTimerID(xTimer, (void *)count);

    /* Obtain the current tick count. */
    ticks = xTaskGetTickCount();

    if (g_oneShotHandleTimer == xTimer)
    {
        printf("One shot timer [%d]\n", ticks);
        vTaskDelete(g_taskHandleO);
        vTaskDelete(g_taskHandleX);
    }
    else if (g_fiveShotHandleTimer == xTimer)
    {
        printf("Five shot timer [%d] ticks=%d\n", ticks, count);

        if (count == 2)
        {
            vTaskSuspend(g_taskHandleO);
        }
        else if (count == 4)
        {
            vTaskSuspend(g_taskHandleX);
        }

        if (count == 5)
        {

            // resume task
            vTaskResume(g_taskHandleO);
            vTaskResume(g_taskHandleX);

            // stop timer
            xTimerStop(g_fiveShotHandleTimer, 0);

            // fire one shot
            xTimerStart(g_oneShotHandleTimer, 0);
        }
    }
    else
    { // other unknown timer ...
    }
}

/**
 * @brief main entry point
 *
 * @return int
 */
int main()
{

    // initialize serial line:  115200.8.N.1
    stdio_init_all();

    // wait for serial line ready
    sleep_ms(1000);
    printf("RTOS START\n");

    // buildin LED
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    do
    {

        // mutex example
        g_chars_mutex = xSemaphoreCreateMutex();
        if (g_chars_mutex == NULL)
            break; // failed

        const char charO = 'O';
        const char charX = 'X';
        if (pdPASS != xTaskCreate(taskPrintBunchOfChars, "Task O", configMINIMAL_STACK_SIZE, (void *)&charO, tskIDLE_PRIORITY + 1UL, &g_taskHandleO))
            break;
        if (pdPASS != xTaskCreate(taskPrintBunchOfChars, "Task X", configMINIMAL_STACK_SIZE, (void *)&charX, tskIDLE_PRIORITY + 1UL, &g_taskHandleX))
            break;

        // multiple shot timer, used same timer callback for multiple timer
        // multiple shots timer, tick = 500ms
        // identifier sets to zero
        g_fiveShotHandleTimer = xTimerCreate("RepeatedTimer", pdMS_TO_TICKS(500), pdTRUE, 0, timerCallback);
        if (g_fiveShotHandleTimer == NULL)
            break; // failed

        // one shot timer sets to 5000 ms,
        // identifier sets to zero
        g_oneShotHandleTimer = xTimerCreate("OneShot", pdMS_TO_TICKS(5000), pdFALSE, 0, timerCallback);
        if (g_oneShotHandleTimer == NULL)
            break; // failed

        // start multiple shot
        if (pdPASS != xTimerStart(g_fiveShotHandleTimer, 0))
            break;

        // queue - reader - writer
        g_readWriteQueue = xQueueCreate(5, sizeof(int32_t));
        if (g_readWriteQueue == NULL)
            break;

        if (pdPASS != xTaskCreate(taskReader, "reader", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1UL, NULL))
            break;
        if (pdPASS != xTaskCreate(taskWriter, "writer", configMINIMAL_STACK_SIZE, (void *)123, tskIDLE_PRIORITY + 1UL, NULL))
            break;

        if (pdPASS != xTaskCreate(taskBlink, "blink", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1UL, NULL))
            break;

        // start RTOS scheduler
        vTaskStartScheduler();

    } while (false);

    //  it should never end here, unless there is a memory deficit
    //  the LED stays on to indicate an error
    gpio_put(PICO_DEFAULT_LED_PIN, 1);
    while (true)
    {
    };

    return 0;
}
