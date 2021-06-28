#define EX_UART_NUM UART_NUM_0
#define PATTERN_CHR_NUM (3) /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)



#include <time.h>



typedef enum
{
    PRINT_MAIN_MENU,  //0
    PRINT_DATE,       //1        Ver fecha y hora
    SET_DATE,         //2        Modificar fecha y Hora
    ADD_NEW_USER,     //3        Agregar nuevo usuario
    DELETE_USER,      //4        Eliminar usuario
    PRINT_USER_LIST,  //5        Listar todos los usuarios
    ADD_NEW_EVENT,    //6        Crear nuevo evento
    PRINT_EVENT_LIST, //7        Ver todos los eventos
    INVALID_DATA      //8        Datos ingresados invalidos.
} FSM_Menu_t;

void uart_event_task(void *pvParameters);

void UART_Init(void);

void Print_Main_Menu();

FSM_Menu_t getSelection(char *UART_temp_data);