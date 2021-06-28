#include <stdio.h>
#include <string.h>

#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"

#include "esp_attr.h"
#include "esp_sleep.h"

#include "driver/uart.h"

#include "at24c.h"
#include "ds3231.h"

#include "appEEPROM.h"
#include "menu.h"

//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

#include "esp_log.h"

FSM_Menu_t FSM_Menu;

SemaphoreHandle_t UART_Data_Semaphore;
SemaphoreHandle_t UART_Receive_Semapore;

char UART_Buffer[RD_BUF_SIZE];

void app_main()
{

    i2c_dev_t dev;
    EEPROM_t dev_EEPROM;
    //time_t now;
    struct tm timeinfo;

    /****************************/
    /****SET TIME****************/
    /****************************/
    const struct tm time = {
        .tm_year = 2021,
        .tm_mon = 5, // 0-based
        .tm_mday = 28,
        .tm_hour = 13,
        .tm_min = 14,
        .tm_sec = 0};
    /****************************/

    event_t new_event = {
        .event_TYPE = UNAUTHORIZED_ACCESS,
        .event_ID = 0};

    //Inicializacion de los Perifericos
    // Initialize RTC

    if (ds3231_init_desc(&dev, I2C_NUM_0, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO) != ESP_OK)
        ESP_LOGE(pcTaskGetTaskName(0), "Could not init device descriptor.\n");

    //Initialize EEPROM

    if (i2c_master_driver_initialize(&dev_EEPROM, EEPROM_SIZE, I2C_NUM_0, AT24C32_ADDR, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO) != ESP_OK)
        ESP_LOGI(pcTaskGetTaskName(0), "EEPROM I2C Init Failed. \n");

    ESP_LOGI(pcTaskGetTaskName(0), "I2C EEPROM communication successfully initialized.\n");

    UART_Init();
    EEPROM_Index_Init(&dev_EEPROM, &new_event);

    //Create Semaphores and initializations

    UART_Receive_Semapore = xSemaphoreCreateBinary();
    UART_Data_Semaphore = xSemaphoreCreateBinary();
    FSM_Menu = PRINT_MAIN_MENU;

    while (1) //FINITE STATE MACHINE
    {

        switch (FSM_Menu)
        {
        case PRINT_MAIN_MENU:

            Print_Main_Menu();

            xSemaphoreGive(UART_Receive_Semapore);
            xSemaphoreTake(UART_Data_Semaphore, (portTickType)portMAX_DELAY);

            FSM_Menu = getSelection(UART_Buffer);
            break;

        case PRINT_DATE:
            printf("\n \nImprimir fecha actual \n\n");

            if (ds3231_get_time(&dev, &timeinfo) != ESP_OK)
                ESP_LOGE(pcTaskGetTaskName(0), "Could not get time.");
            else
            {

                printf("%04d-%02d-%02d %02d:%02d:%02d\n\n",
                       timeinfo.tm_year, timeinfo.tm_mon + 1,
                       timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            }
            FSM_Menu = PRINT_MAIN_MENU;
            break;

        case SET_DATE:
            printf("Actualizar fecha y Hora. \n\n");

            if (ds3231_set_time(&dev, &time) != ESP_OK)
                ESP_LOGE(pcTaskGetTaskName(0), "No se pudo actualizar la fecha y hora.\n");
            else
                ESP_LOGI(pcTaskGetTaskName(0), "Se actualizo correctamenta la fecha y hora.\n\n");
            FSM_Menu = PRINT_MAIN_MENU;
            break;

        case ADD_NEW_USER:
            printf("Agregar nuevo usuario \n");
            FSM_Menu = PRINT_MAIN_MENU;
            break;
        case DELETE_USER:
            printf("Borrar usuario \n");
            FSM_Menu = PRINT_MAIN_MENU;
            break;
        case PRINT_USER_LIST:
            printf("Imprimir lista completa de usuarios \n");
            FSM_Menu = PRINT_MAIN_MENU;
            break;

        case ADD_NEW_EVENT:
            printf("Agregar nuevo evento \n");

            if (ds3231_get_time(&dev, &timeinfo) != ESP_OK)
                ESP_LOGE(pcTaskGetTaskName(0), "Could not get time.");

            new_event.time_stamp = timeinfo;
            new_event.event_ID++;

            if (EEPROM_New_Event(&dev_EEPROM, new_event) != ESP_OK)
                ESP_LOGE(pcTaskGetTaskName(0), "Could not create new event.\n");
            else
                ESP_LOGI(pcTaskGetTaskName(0), "El nuevo evento se creo correctamente.\n\n");

            FSM_Menu = PRINT_MAIN_MENU;
            break;

        case PRINT_EVENT_LIST:
            printf("Imprimir lista completa de eventos:\n\n");

            if (EEPROM_List_Events(&dev_EEPROM) != ESP_OK)
                ESP_LOGE(pcTaskGetTaskName(0), "Could not list events.\n");
            else
                ESP_LOGI(pcTaskGetTaskName(0), "Se pudo listar correctamente los eventos.\n\n");

            FSM_Menu = PRINT_MAIN_MENU;
            break;
        default:
            printf("Invalid data, please start again.\n\n");
            FSM_Menu = PRINT_MAIN_MENU;
            break;
        }
        //xSemaphoreGive(UART_Receive_Semapore);
    }
}
