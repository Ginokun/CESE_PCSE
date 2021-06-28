
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "menu.h"
#include "ctype.h"

static const char *TAG = "uart_events";

/**
 *
 * - Port: UART0
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: on
 * - Pin assignment: TxD (default), RxD (default)
 */

static QueueHandle_t uart0_queue;

extern QueueHandle_t UART_Data;

extern SemaphoreHandle_t UART_Data_Semaphore;
extern SemaphoreHandle_t UART_Receive_Semapore;

extern char UART_Buffer[RD_BUF_SIZE];

void uart_event_task(void *pvParameters)
{
    uart_event_t event;

    for (;;)
    {

        //Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY))
        {
            //bzero(UART_Buffer, RD_BUF_SIZE);
            //ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
            switch (event.type)
            {
            //Event of UART receving data
            /*We'd better handler data event fast, there would be much more data events than
                other types of events. If we take too much time on data event, the queue might
                be full.*/
            case UART_DATA:
                //ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                if (xSemaphoreTake(UART_Receive_Semapore, 0))
                {
                    uart_read_bytes(EX_UART_NUM, (uint8_t *)UART_Buffer, event.size, portMAX_DELAY);
                    xSemaphoreGive(UART_Data_Semaphore); //Se habilita el semaforo avisando que se recibio información par la UART.
                }
                else
                {
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                }

                //ESP_LOGI(TAG, "[DATA EVT]:");
                //uart_write_bytes(EX_UART_NUM, (const char *)UART_Buffer, event.size);

                break;
            //Event of HW FIFO overflow detected
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                // If fifo overflow happened, you should consider adding flow control for your application.
                // The ISR has already reset the rx FIFO,
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart0_queue);
                break;
            //Event of UART ring buffer full
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                // If buffer full happened, you should consider encreasing your buffer size
                // As an example, we directly flush the rx buffer here in order to read more data.
                uart_flush_input(EX_UART_NUM);
                xQueueReset(uart0_queue);
                break;
            //Event of UART RX break detected
            case UART_BREAK:
                ESP_LOGI(TAG, "uart rx break");
                break;
            //Event of UART parity check error
            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;
            //Event of UART frame error
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            //UART_PATTERN_DET
            case UART_PATTERN_DET:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            //Others
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

void UART_Init(void)
{
    BaseType_t xReturned;

    esp_log_level_set(TAG, ESP_LOG_INFO);

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(EX_UART_NUM, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Create a task to handler UART event from ISR
    xReturned = xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
    if (xReturned == pdPASS)
    {
        ESP_LOGI(pcTaskGetTaskName(0), "UART Initialization successfull\n");
    }
    else
    {
        ESP_LOGE(pcTaskGetTaskName(0), "Could not init UART task.\n");
    }
}

void Print_Main_Menu()
{
    printf("*****************************\n");
    printf("*************MENU************\n");
    printf("*****************************\n\n");

    printf("1. Ver fecha y hora actual\n");
    printf("2. Mofidicar fecha y hora actual\n");
    printf("3. Agregar nuevo usuario\n");
    printf("4. Eliminar usuario\n");
    printf("5. Listar todos los usuarios\n");
    printf("6. Crear nuevo evento\n");
    printf("7. Ver todos los eventos\n\n");
    
    printf("*****************************\n\n\n");
}

FSM_Menu_t getSelection(char *UART_temp_data)
{
    uint8_t aux = (uint8_t)(UART_temp_data[0] - '0');

    switch (aux)
    {
    case PRINT_DATE:
        return PRINT_DATE;
        break;
    case SET_DATE:
        return SET_DATE;
        break;
    case ADD_NEW_USER:
        return ADD_NEW_USER;
        break;
    case DELETE_USER:
        return DELETE_USER;
        break;
    case PRINT_USER_LIST:
        return PRINT_USER_LIST;
        break;
    case ADD_NEW_EVENT:
        return ADD_NEW_EVENT;
        break;
    case PRINT_EVENT_LIST:
        return PRINT_EVENT_LIST;
        break;
    default:
        return INVALID_DATA;
        break;
    }
}