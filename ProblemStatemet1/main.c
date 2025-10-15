/*HERE IS THE CODE FOR THE PROBLEM STATEMENT 1 WITH ITS COMMENTS*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t dataID;
    int32_t DataValue;
} Data_t;

QueueHandle_t Queue1;

/* Task prototypes & handles (as requested) */
void ExampleTask1(void *pV);
TaskHandle_t TaskHandle_1;
void ExampleTask2(void *pV);
TaskHandle_t TaskHandle_2;


volatile uint8_t GG__DDaattaaIIDD = 1;
volatile int32_t GG__DDaattaaVVaalluuee = 0;

/* Configuration: initial priorities (tunable) */
#define EXAMPLE_TASK1_PRIORITY   (tskIDLE_PRIORITY + 1)
#define EXAMPLE_TASK2_PRIORITY   (tskIDLE_PRIORITY + 2)
#define EXAMPLE_TASK2_PRIORITY_BOOST 2

/* Helper to print; expects that printf is retargeted to your UART/SWO */
static void print_data(const Data_t *d) {
    printf("ExampleTask2 Eval -> dataID=%u, DataValue=%ld\r\n",
           (unsigned)d->dataID, (long)d->DataValue);
}

/* ---------------- ExampleTask1 ----------------
   Sends Data_t every exactly 500ms using vTaskDelayUntil
*/
void ExampleTask1(void *pV)
{
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(500); /* 500 ms */
    Data_t txData;

    /* Initialize xLastWakeTime for precise periodic delay */
    xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        /* populate structure from globals (assume updated elsewhere) */
        txData.dataID = (uint8_t)GG__DDaattaaIIDD;
        txData.DataValue = (int32_t)GG__DDaattaaVVaalluuee;

        /* send to Queue1; use a short block time to avoid blocking forever */
        if (Queue1 != NULL) {
            BaseType_t ok = xQueueSend(Queue1, &txData, pdMS_TO_TICKS(10));
            if (ok != pdPASS) {
                /* queue full or error - optional: handle error/log */
                printf("ExampleTask1: Queue send failed\n");
            }
        }

        /* wait until next exact period */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
    }
}

/* ---------------- ExampleTask2 ----------------
   Waits on Queue1 and applies the decision logic described in the assignment.
*/
void ExampleTask2(void *pV)
{
    Data_t rxData;
    UBaseType_t initialPriority;
    BaseType_t priorityBoosted = pdFALSE;

    /* Store the creation priority (initial) to allow relative changes */
    initialPriority = EXAMPLE_TASK2_PRIORITY;

    for (;;) {
        /* block until data available */
        if (Queue1 != NULL) {
            if (xQueueReceive(Queue1, &rxData, portMAX_DELAY) == pdPASS) {
                /* Print at every evaluation */
                print_data(&rxData);

                /* Apply logic */
                if (rxData.dataID == 0) {
                    printf("ExampleTask2: dataID==0 -> deleting self\n");
                    /* delete ExampleTask2 (self) */
                    vTaskDelete(NULL);
                    /* vTaskDelete does not return */
                } else if (rxData.dataID == 1) {
                    /* allow processing of DataValue member */
                    if (rxData.DataValue == 0) {
                        /* Increase priority by 2 from initial creation value */
                        if (!priorityBoosted) {
                            UBaseType_t newPrio = initialPriority + EXAMPLE_TASK2_PRIORITY_BOOST;
                            vTaskPrioritySet(TaskHandle_2, newPrio);
                            priorityBoosted = pdTRUE;
                            printf("ExampleTask2: priority increased to %lu\n", (unsigned long)newPrio);
                        } else {
                            printf("ExampleTask2: priority already boosted\n");
                        }
                    } else if (rxData.DataValue == 1) {
                        /* Decrease the priority if previously increased */
                        if (priorityBoosted) {
                            vTaskPrioritySet(TaskHandle_2, initialPriority);
                            priorityBoosted = pdFALSE;
                            printf("ExampleTask2: priority restored to %lu\n", (unsigned long)initialPriority);
                        } else {
                            printf("ExampleTask2: priority not boosted; nothing to restore\n");
                        }
                    } else if (rxData.DataValue == 2) {
                        printf("ExampleTask2: DataValue==2 -> deleting self\n");
                        vTaskDelete(NULL);
                    } else {
                        /* Other DataValue handling - currently no-op */
                        printf("ExampleTask2: DataValue (%ld) no-op\n", (long)rxData.DataValue);
                    }
                } else {
                    /* dataID other values, no specified action; just print */
                    printf("ExampleTask2: received unknown dataID %u\n", (unsigned)rxData.dataID);
                }
            }
        }
    } /* for */
}

/* ---------------- main / system init ----------------
   Create queue, tasks, and start scheduler.
   Integrate with your board init (HAL init, clock config) as required.
*/
int main(void)
{
    /* Perform any hardware initialization here (HAL_Init, SystemClock_Config etc.) */
    /* For the assignment provide only RTOS logic; assume board init is done elsewhere */

    /* Create Queue1: length 5, item size sizeof(Data_t) */
    Queue1 = xQueueCreate(5, sizeof(Data_t));
    if (Queue1 == NULL) {
        /* queue creation failed */
        printf("Failed to create Queue1\n");
        for (;;); /* halt or handle error */
    }

    /* Create ExampleTask2 first (so we have a handle for priority operations) */
    BaseType_t ret;

    ret = xTaskCreate(ExampleTask2, "ExampleTask2", configMINIMAL_STACK_SIZE + 128,
                      NULL, EXAMPLE_TASK2_PRIORITY, &TaskHandle_2);
    if (ret != pdPASS) {
        printf("Failed to create ExampleTask2\n");
        for (;;);
    }

    /* Create ExampleTask1 */
    ret = xTaskCreate(ExampleTask1, "ExampleTask1", configMINIMAL_STACK_SIZE + 128,
                      NULL, EXAMPLE_TASK1_PRIORITY, &TaskHandle_1);
    if (ret != pdPASS) {
        printf("Failed to create ExampleTask1\n");
        for (;;);
    }

    /* Start the scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
    for (;;);
    return 0;
}
