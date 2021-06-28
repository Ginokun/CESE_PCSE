#ifndef APPEEPROM_H_
#define APPEEPROM_H_

#include "at24c.h"
#include <time.h>

#define BYTES_PER_ADDRESS 2 //Address numbers of bytes (address size)

#define INDEX_FIRST_EVENT_ADDRESS 0x0000 //En esta direccion se guarda la direccion del primer evento
#define INDEX_NEXT_EVENT_ADDRESS 0x0002  //En esta direccion se guarda la direccion del proximo evento a guardar
#define INDEX_FIRST_USER_ADDRESS 0x0004  //En esta direccion se guarda la direccion del primer usuario
#define INDEX_LAST_USER_ADDRESS 0x0006   //En esta direccion se guarda la direccion del ultimo usuario
#define INDEX_NEW_USER_ADDRESS 0x0008

#define FIRST_EVENT_ADDRESS 0x000A
#define LAST_EVENT_ADDRESS 0x0F00     //Direccion de la ultimo espacio de memoria disponible para guardar eventos.
#define FIRST_USER_ADDRESS 0x0F02     //!
#define LAST_USER_ADDRESS 0x0FE1      //Direccion de la ultimo espacio de memoria disponible para guardar usuarios.
#define INDEX_NEW_USER_ADDRESS 0x0008 //!

#define LAST_MEMORY_ADDRESS 0x0FE1 //Ultima direccion de memoria disponible.

#define MAX_USER_NAME 50     //Maxima cantidad de caracteres del nombre de usuario.
#define MAX_USER_PASSWORD 50 //Maxima cantidad de caracteres del nombre de usuario.

enum
{
    LOW_BATTERY = 0,
    AUTHORIZED_ACCESS,
    UNAUTHORIZED_ACCESS,
    FORCED_ENTRY,
    DOOR_OPENED,
    DOOR_CLOSED
};

typedef uint8_t EVENT_TYPE_t;

typedef struct
{
    uint8_t user_ID;
    char user[MAX_USER_NAME];
    char password[MAX_USER_NAME];
} user_t;

// typedef struct
// {
//     uint32_t event_ID;
//     EVENT_TYPE_t event;
//     uint8_t user_ID;
//     struct tm time_stamp;
// } event_t;

typedef struct
{
    uint8_t event_ID;
    EVENT_TYPE_t event_TYPE;
    struct tm time_stamp;
} event_t;

esp_err_t EEPROM_Index_Init(EEPROM_t *dev);

//static esp_err_t EEPROM_Get_Next_Event_Address(EEPROM_t *dev, uint16_t *get_address);
//static esp_err_t EEPROM_Read_Data(EEPROM_t *dev, uint32_t Data, ssize_t Data_Size, uint16_t read_address);
//static esp_err_t EEPROM_Save_Data(EEPROM_t *dev, uint32_t Data, ssize_t Data_Size, uint16_t save_address);
//static esp_err_t EEPROM_Save_Next_Event_Address(EEPROM_t *dev, uint16_t save_address);
//static void Print_Event(event_t event)

esp_err_t EEPROM_New_Event(EEPROM_t *dev, event_t new_event);
esp_err_t EEPROM_List_Events(EEPROM_t *dev);

#endif /* APPEEPROM_H_ */