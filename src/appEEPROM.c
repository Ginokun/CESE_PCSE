

#include "at24c.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"
#include "esp_log.h"

#include "appEEPROM.h"

#define tag "appEEPROM"



static esp_err_t EEPROM_Get_Next_Event_Address(EEPROM_t *dev, uint16_t *get_address);
static esp_err_t EEPROM_Read_Data(EEPROM_t *dev, uint32_t * Data, ssize_t Data_Size, uint16_t read_address);
static esp_err_t EEPROM_Save_Data(EEPROM_t *dev, uint32_t Data, ssize_t Data_Size, uint16_t save_address);
static esp_err_t EEPROM_Save_Next_Event_Address(EEPROM_t *dev, uint16_t save_address);
static void Print_Event(event_t event);





static esp_err_t EEPROM_Read_Data(EEPROM_t *dev, uint32_t *Data, ssize_t Data_Size, uint16_t read_address)
{
    esp_err_t ret = 0;

    uint8_t aux_data = 0;
    int8_t i = 0;

    for (i = Data_Size - 1; i >= 0; i--)
    {
        ret = ReadRom(dev, read_address + i, &aux_data);
        if (ret != ESP_OK)
        {
            ESP_LOGI(tag, "ReadRom fail %d", ret);
            return ret;
        }
        *Data += aux_data;
        if (i > 0)
            *Data <<= 8;
    }
    //ESP_LOGI(tag, "Se leyeron correctamente 0x%04x\n \n", (uint8_t)*Data);

    return ret;
}

static esp_err_t EEPROM_Save_Data(EEPROM_t *dev, uint32_t Data, ssize_t Data_Size, uint16_t save_address)
{
    esp_err_t ret = 0;
    uint8_t aux_data = 0;

    uint8_t i = 0;

    for (i = 0; i < Data_Size; i++)
    {
        aux_data = (uint8_t)(Data & 0xFF);

        ret = WriteRom(dev, save_address, aux_data);
        if (ret != ESP_OK)
        {
            ESP_LOGI(tag, "WriteRom fail %d", ret);
            return ret;
        }
        save_address++;

        if (i < (Data_Size - 1))
            Data = (Data >> 8);
    }
    //ESP_LOGI(tag, "Se grabaron correctamente %d datos \n", i);

    return ret;
}

esp_err_t EEPROM_Index_Init(EEPROM_t *dev, event_t *event)
{
    esp_err_t ret;
    uint16_t aux_address = 0;

    //Conseguir la direccion del primer evento para ver si se inicializo la memoria.
    ret = EEPROM_Read_Data(dev, (uint32_t *)&aux_address, sizeof(aux_address), INDEX_FIRST_EVENT_ADDRESS);
    if (ret != ESP_OK)
    {
        ESP_LOGI(tag, "Read Data fail: %d", ret);
        return ret;
    }
    //En caso de no estar inicializado el indice, se inicializa.
    if (aux_address != FIRST_EVENT_ADDRESS)
    {
        //Se inicializa el indice que apunta a la primera direccion de los eventos.
        ret = EEPROM_Save_Data(dev, FIRST_EVENT_ADDRESS, sizeof((uint16_t)FIRST_EVENT_ADDRESS), INDEX_FIRST_EVENT_ADDRESS);
        if (ret != ESP_OK)
        {
            ESP_LOGI(tag, "EEPROM Init, write first Event Failed %d", ret);
            return ret;
        }
        //Se inicializa el indice que apunta a la proxima direccion donde guardar un evento. Al no haber eventos, es la misma del primer evento.
        ret = EEPROM_Save_Data(dev, FIRST_EVENT_ADDRESS, sizeof((uint16_t)FIRST_EVENT_ADDRESS), INDEX_NEXT_EVENT_ADDRESS);
        if (ret != ESP_OK)
        {
            ESP_LOGI(tag, "EEPROM Init, write first Event Failed %d", ret);
            return ret;
        }
    }
    //Se obtiene la direccion en memoria del proximo evento para leer el ID del ultimo evento guardado y poder continuar con la numeracion.
    aux_address = 0;
    ret = EEPROM_Get_Next_Event_Address(dev, &aux_address);
    if (ret != ESP_OK)
    {
        ESP_LOGI(tag, "Error getting next event address \n");
        return ret;
    }
    if (aux_address > FIRST_EVENT_ADDRESS)
    {
        aux_address = aux_address - 24;
        aux_address = aux_address - sizeof(event->event_TYPE);
        aux_address = aux_address - sizeof(event->event_ID);

        ret = EEPROM_Read_Data(dev, (uint32_t *)&(event->event_ID), sizeof(event->event_ID), aux_address);
        if (ret != ESP_OK)
        {
            ESP_LOGI(tag, "Read Data fail: %d", ret);
            return ret;
        }
    }
    return ret;
}

static esp_err_t EEPROM_Get_Next_Event_Address(EEPROM_t *dev, uint16_t *get_address) //Se obtiene la direccion en memoria del proximo evento.
{

    esp_err_t ret;
    uint16_t aux_address = 0;

    ret = EEPROM_Read_Data(dev, (uint32_t *)&aux_address, sizeof(aux_address), INDEX_NEXT_EVENT_ADDRESS);
    if (ret != ESP_OK)
    {
        ESP_LOGI(tag, "Read Data fail: %d", ret);
        return ret;
    }
    *get_address = aux_address;
    //ESP_LOGI(tag, "Se obtuvo correctamente la direccion del proximo evento \n");

    return ret;
}

/**
 * @brief 
 * 
 * @param dev 
 * @param Data 
 * @param Data_Size 
 * @param read_address 
 * @return esp_err_t 
 */

static esp_err_t EEPROM_Save_Next_Event_Address(EEPROM_t *dev, uint16_t save_address) //Se obtiene la direccion en memoria del ultimo evento.
{
    esp_err_t ret;

    ret = EEPROM_Save_Data(dev, (uint32_t)save_address, sizeof(save_address), INDEX_NEXT_EVENT_ADDRESS);

    return ret;
}

esp_err_t EEPROM_New_Event(EEPROM_t *dev, event_t new_event)
{
    esp_err_t ret;
    uint16_t address = 0;

    //Se obtiene la direccion en memoria del proximo evento.
    ret = EEPROM_Get_Next_Event_Address(dev, &address);

    if (ret != ESP_OK)
    {
        ESP_LOGI(tag, "Error getting last address \n");
        return ret;
    }

    if (address >= LAST_EVENT_ADDRESS) //Si se llego al final de la memoria para eventos, se vuelve a pisar el principio de la memoria.
    {
        address = FIRST_EVENT_ADDRESS;
    }

    //Se guarda el nuevo evento en memoria
    ret = EEPROM_Save_Data(dev, (uint32_t)new_event.event_ID, sizeof(new_event.event_ID), address);
    if (ret != ESP_OK)
    {
        ESP_LOGI(tag, "Error saving data \n");
        return ret;
    }
    address = address + sizeof(new_event.event_ID);
    ret = EEPROM_Save_Data(dev, (uint32_t)new_event.event_TYPE, sizeof(new_event.event_TYPE), address);
    if (ret != ESP_OK)
    {
        ESP_LOGI(tag, "Error saving data \n");
        return ret;
    }
    address = address + sizeof(new_event.event_TYPE);

    //Se realiza el guardado de la fecha y hora en memoria.

    ret = EEPROM_Save_Data(dev, (uint32_t)new_event.time_stamp.tm_year, sizeof(new_event.time_stamp.tm_year), address);
    address = address + sizeof(new_event.time_stamp.tm_year);
    ret = EEPROM_Save_Data(dev, (uint32_t)new_event.time_stamp.tm_mon, sizeof(new_event.time_stamp.tm_mon), address);
    address = address + sizeof(new_event.time_stamp.tm_mon);
    ret = EEPROM_Save_Data(dev, (uint32_t)new_event.time_stamp.tm_mday, sizeof(new_event.time_stamp.tm_mday), address);
    address = address + sizeof(new_event.time_stamp.tm_mday);
    ret = EEPROM_Save_Data(dev, (uint32_t)new_event.time_stamp.tm_hour, sizeof(new_event.time_stamp.tm_hour), address);
    address = address + sizeof(new_event.time_stamp.tm_hour);
    ret = EEPROM_Save_Data(dev, (uint32_t)new_event.time_stamp.tm_min, sizeof(new_event.time_stamp.tm_min), address);
    address = address + sizeof(new_event.time_stamp.tm_min);
    ret = EEPROM_Save_Data(dev, (uint32_t)new_event.time_stamp.tm_sec, sizeof(new_event.time_stamp.tm_sec), address);
    address = address + sizeof(new_event.time_stamp.tm_sec);

    //ESP_LOGI(tag, "Se grabo correctamente el nuevo evento \n");

    //Se actualiza la direccion del proximo evento en el indice de la memoria

    ret = EEPROM_Save_Next_Event_Address(dev, address);
    if (ret != ESP_OK)
    {
        ESP_LOGI(tag, "Error actualizando la ultima direccion del evento \n");
        return ret;
    }
    //ESP_LOGI(tag, "Se actualizo correctamente la direccion del nuevo evento \n");

    return ret;
}

static void Print_Event(event_t event)
{

    printf("Event ID: %d\n", event.event_ID);

    printf("%04d-%02d-%02d %02d:%02d:%02d\n",
           event.time_stamp.tm_year, event.time_stamp.tm_mon + 1,
           event.time_stamp.tm_mday, event.time_stamp.tm_hour, event.time_stamp.tm_min, event.time_stamp.tm_sec);

    switch (event.event_TYPE)
    {
    case LOW_BATTERY:

        printf("Meaning: LOW_BATTERY\n\n");
        break;

    case AUTHORIZED_ACCESS:

        printf("Meaning: AUTHORIZED_ACCESS\n\n");
        break;

    case UNAUTHORIZED_ACCESS:

        printf("Meaning: UNAUTHORIZED_ACCESS\n\n");
        break;

    case FORCED_ENTRY:

        printf("Meaning: FORCED_ENTRY\n\n");
        break;

    case DOOR_OPENED:

        printf("Meaning: DOOR_OPENED\n\n");
        break;

    case DOOR_CLOSED:

        printf("Meaning: DOOR_CLOSED\n\n");
        break;

    default:
        printf("Meaning: Not a valid case.\n\n");
        break;
    }
    //TODO: Agregar para imprimir timestamp.
}

esp_err_t EEPROM_List_Events(EEPROM_t *dev)
{
    esp_err_t ret;

    uint16_t address = 0;
    event_t event = {.event_TYPE = 0,
                     .event_ID = 0,
                     .time_stamp = {
                         .tm_year = 0,
                         .tm_mon = 0, // 0-based
                         .tm_mday = 0,
                         .tm_hour = 0,
                         .tm_min = 0,
                         .tm_sec = 0}};

    //Se obtiene la direccion en memoria del proximo evento.
    ret = EEPROM_Get_Next_Event_Address(dev, &address);
    if (ret != ESP_OK)
    {
        ESP_LOGI(tag, "Error getting next event address \n");
        return ret;
    }
    if (LAST_EVENT_ADDRESS > address)
    {
        while (address > FIRST_EVENT_ADDRESS)
        {

            //Se lee la fecha y hora del evento de la memoria.

            address = address - sizeof(event.time_stamp.tm_sec);
            ret = EEPROM_Read_Data(dev, (uint32_t *)&event.time_stamp.tm_sec, sizeof(event.time_stamp.tm_sec), address);
            address = address - sizeof(event.time_stamp.tm_min);
            ret = EEPROM_Read_Data(dev, (uint32_t *)&event.time_stamp.tm_min, sizeof(event.time_stamp.tm_min), address);
            address = address - sizeof(event.time_stamp.tm_hour);
            ret = EEPROM_Read_Data(dev, (uint32_t *)&event.time_stamp.tm_hour, sizeof(event.time_stamp.tm_hour), address);
            address = address - sizeof(event.time_stamp.tm_mday);
            ret = EEPROM_Read_Data(dev, (uint32_t *)&event.time_stamp.tm_mday, sizeof(event.time_stamp.tm_mday), address);
            address = address - sizeof(event.time_stamp.tm_mon);
            ret = EEPROM_Read_Data(dev, (uint32_t *)&event.time_stamp.tm_mon, sizeof(event.time_stamp.tm_mon), address);
            address = address - sizeof(event.time_stamp.tm_year);
            ret = EEPROM_Read_Data(dev, (uint32_t *)&event.time_stamp.tm_year, sizeof(event.time_stamp.tm_year), address);

            address = address - sizeof(event.event_TYPE);
            ret = EEPROM_Read_Data(dev, (uint32_t *)&(event.event_TYPE), sizeof(event.event_TYPE), address);
            if (ret != ESP_OK)
            {
                ESP_LOGI(tag, "Read Data fail: %d", ret);
                return ret;
            }
            address = address - sizeof(event.event_ID);
            ret = EEPROM_Read_Data(dev, (uint32_t *)&(event.event_ID), sizeof(event.event_ID), address);
            if (ret != ESP_OK)
            {
                ESP_LOGI(tag, "Read Data fail: %d", ret);
                return ret;
            }

            Print_Event(event);

            event.event_TYPE = 0;
            event.event_ID = 0;

            event.time_stamp.tm_year = 0;
            event.time_stamp.tm_mon = 0; // 0-based
            event.time_stamp.tm_mday = 0;
            event.time_stamp.tm_hour = 0;
            event.time_stamp.tm_min = 0;
            event.time_stamp.tm_sec = 0;
        }
    }
    return ret;
}
