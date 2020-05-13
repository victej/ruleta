//Integrantes del equipo ruleta 
//Hernan Salgado Suarez
//Victor Manuel Tejeda Meza
//Javier Gonzales Ramirez

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define GPIO_COLUMNA_1    15
#define GPIO_COLUMNA_2    2
#define GPIO_COLUMNA_3    0
#define GPIO_COLUMNA_4    4
#define GPIO_RENGLON_1    16
#define GPIO_RENGLON_2    17
#define GPIO_RENGLON_3    5
#define GPIO_RENGLON_4    18
#define GPIO_LED_1	  12 
#define GPIO_LED_2	  14
#define GPIO_LED_3	  27
#define GPIO_LED_4	  26
#define GPIO_LED_5	  25
#define GPIO_LED_6	  33
#define GPIO_LED_7	  32
#define GPIO_LED_8	  35
#define GPIO_BOTON_INICIO 34


#define GPIO_RENGLONES_SEL  ((1ULL<<GPIO_RENGLON_1) | (1ULL<<GPIO_RENGLON_2) | (1ULL<<GPIO_RENGLON_3) | (1ULL<<GPIO_RENGLON_4))
#define GPIO_COLUMNAS_SEL  ((1ULL<<GPIO_COLUMNA_1) | (1ULL<<GPIO_COLUMNA_2) | (1ULL<<GPIO_COLUMNA_3) | (1ULL<<GPIO_COLUMNA_4))

//Define las colas para la comunicaciÃ³n de eventos del teclado 
//static QueueHandle_t xFIFOTeclado;
static xQueueHandle xFIFOTeclado = NULL;

// define la tarea del teclado 
static void TaskTeclado( void *pvParameters );

// define la tarea de la ruleta
static void TaskRuleta( void *pvParameters1 );


//#define ESP_INTR_FLAG_DEFAULT 0


void app_main(void)
{

    gpio_config_t io_conf;
    
    //Configura los renglones como salida
    
    //deshabilita las interrupciones
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //Configura como salida
    io_conf.mode = GPIO_MODE_OUTPUT;
    //Mascara de bits de las terminales que se desea configurar
    io_conf.pin_bit_mask = GPIO_RENGLONES_SEL;
    //deshabilita el modo pull-down
    io_conf.pull_down_en = 0;
    //habilita el pull-up 
    io_conf.pull_up_en = 0;
    //configura las GPIO con los valores indicados
    gpio_config(&io_conf);
    
    
    //Configura las columnas como entrada con pull-down
    
    //habilita las interrupciones con flanco de bajada
    io_conf.intr_type = GPIO_PIN_INTR_NEGEDGE;
    //Configura como entrada
    io_conf.mode = GPIO_MODE_INPUT;
    //Mascara de bits de las terminales que se desea configurar
    io_conf.pin_bit_mask = GPIO_COLUMNAS_SEL;
    //habilita el modo pull-down
    io_conf.pull_down_en = 1;
    //deshabilita el pull-up 
    io_conf.pull_up_en = 0;
    //configura las GPIO con los valores indicados
    gpio_config(&io_conf);
    
    //Configura LEDS como salida y boton de inicio como entrada
    gpio_set_direction(GPIO_LED_1, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LED_2, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LED_3, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LED_4, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LED_5, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LED_6, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LED_7, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_LED_8, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_BOTON_INICIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_BOTON_INICIO,GPIO_PULLUP_ONLY);

    //crea el FIFO del teclado
    xFIFOTeclado = xQueueCreate(10, sizeof(int));
    
    //Crea tarea para lectura de teclas
    xTaskCreate(TaskTeclado, "TareaTeclado", 2048, NULL, 1, NULL);

    //Crea tarea para ruleta
    xTaskCreate(TaskRuleta, "TareaRuleta", 2048, NULL, 1, NULL);


    }


//arreglo con los numeros de GPIO de las diferentes columnas y renglones del
//Teclado multiplexado
int columnas[]={GPIO_COLUMNA_1,GPIO_COLUMNA_2,GPIO_COLUMNA_3,GPIO_COLUMNA_4};
int renglones[]={GPIO_RENGLON_1,GPIO_RENGLON_2,GPIO_RENGLON_3,GPIO_RENGLON_4};

//Tarea de exploracion del teclado
static void TaskTeclado(void *pvParameters)  
{
  int renglon,columna,teclaPresionada;
  int codigo,codigoAnterior;
  //(void) pvParameters;
  
  codigoAnterior=-1;
  
  for(renglon=0;renglon<4;renglon++)
    gpio_set_level(renglones[renglon],0);
  
  for (;;) // una tarea nunca debe retornar o salir
  {
    teclaPresionada=0;//supone que no hay una tecla presionada
    codigo=0;
    //exploracion del teclado multiplexado
    for(renglon=0;renglon<4;renglon++){
      gpio_set_level(renglones[renglon],1);
      for(columna=0;columna<4;columna++){
          //printf("r:%d c: %d in:%d\n",renglon,columna,gpio_get_level(columnas[columna]));
          if(gpio_get_level(columnas[columna])==1) {
             teclaPresionada=1;
             codigo=(columna)|(renglon<<2);
          }
      }
      gpio_set_level(renglones[renglon],0);
    }
    
    //eliminacion de rebotes
    if(teclaPresionada==1){
      if (codigo==codigoAnterior){
        //meter a la cola el codigo de la tecla
        xQueueSendToBack(xFIFOTeclado,&codigo,portMAX_DELAY);
        vTaskDelay( 150 / portTICK_RATE_MS ); // espera durante 150mst
      }
      codigoAnterior=codigo;
      vTaskDelay( 50 / portTICK_RATE_MS ); // espera durante 50 ms
    }else{
      codigoAnterior=-1;
      vTaskDelay( 20 / portTICK_RATE_MS ); // espera durante 20ms 
    }
  }
}


//Tarea de ruleta
static void TaskRuleta(void *pvParameters1)
{

uint32_t teclaR;

	if (gpio_get_level(GPIO_BOTON_INICIO)==0)
	{

		xQueueReceive(xFIFOTeclado, &teclaR, 1000);
		int ganador = esp_random()&0x7;
		int vueltas;
		int tRuleta = 500;

		for (vueltas=0; vueltas<4; vueltas++ )
		{
			gpio_set_level(GPIO_LED_1, 1);
			vTaskDelay(tRuleta / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_1, 0);
			gpio_set_level(GPIO_LED_2, 1);
			vTaskDelay(tRuleta / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_2, 0);
			gpio_set_level(GPIO_LED_3, 1);
			vTaskDelay(tRuleta / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_3, 0);
			gpio_set_level(GPIO_LED_4, 1);
			vTaskDelay(tRuleta / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_4, 0);
			gpio_set_level(GPIO_LED_5, 1);
			vTaskDelay(tRuleta / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_5, 0);
			gpio_set_level(GPIO_LED_6, 1);
			vTaskDelay(tRuleta / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_6, 0);
			gpio_set_level(GPIO_LED_7, 1);
			vTaskDelay(tRuleta / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_7, 0);
			gpio_set_level(GPIO_LED_8, 1);
			vTaskDelay(tRuleta / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_8, 0);
			tRuleta = tRuleta/2;
		}

		switch(ganador)
		{
			case 1:
			gpio_set_level(GPIO_LED_1, 1);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_1, 0);
			break;
	
			case 2:
			gpio_set_level(GPIO_LED_2, 1);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_1, 0);
			break;

			case 3:
			gpio_set_level(GPIO_LED_3, 1);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_1, 0);
			break;
			
			case 4:
			gpio_set_level(GPIO_LED_4, 1);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_1, 0);
			break;

			case 5:
			gpio_set_level(GPIO_LED_5, 1);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_1, 0);
			break;

			case 6:
			gpio_set_level(GPIO_LED_6, 1);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_1, 0);
			break;

			case 7:
			gpio_set_level(GPIO_LED_7, 1);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_1, 0);
			break;

			case 8:
			gpio_set_level(GPIO_LED_8, 1);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			gpio_set_level(GPIO_LED_1, 0);
			break;
		}
			
		if (ganador == teclaR)
		{
		printf("Ganaste");
		}
		else{printf("Perdiste");}
	}
}

