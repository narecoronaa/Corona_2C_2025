/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * This section describes how the program works.
 *
 * <a href="https://drive.google.com/...">Operation Example</a>
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	PIN_X	 	| 	GPIO_X		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 12/09/2023 | Document creation		                         |
 *
 * @author Albano Peñalva (albano.penalva@uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "switch.h"
#include "esp_mac.h"
/*==================[macros and definitions]=================================*/

#define OFF 0
#define ON 1
#define TOGGLE 2 
#define CONFIG_BLINK_PERIOD 100 // Periodo de parpadeo en milisegundos

/*==================[internal data definition]===============================*/
struct leds
{
    uint8_t mode;       //ON, OFF, TOGGLE
	uint8_t n_led;        //indica el número de led a controlar
	uint8_t n_ciclos;   //indica la cantidad de ciclos de ncendido/apagado
	uint16_t periodo;    //indica el tiempo de cada ciclo
} my_leds;

/*==================[internal functions declaration]=========================*/

void ControlLeds(struct leds* puntero_led);

/*==================[external functions definition]==========================*/

void ControlLeds(struct leds* puntero_led)
{
	switch (puntero_led->mode)
	{
		case ON:
			switch (puntero_led->n_led) {
                case LED_1:
                    LedOn(LED_1);
                break;
                case LED_2:
                    LedOn(LED_2);
                break;
                case LED_3:
                    LedOn(LED_3);
                break;
            }
		break;

		case OFF:
			switch (puntero_led->n_led) {
                case LED_1:
                    LedOff(LED_1);
                break;
                case LED_2:
                    LedOff(LED_2);
                break;
                case LED_3:
                    LedOff(LED_3);
                break;
            }
		break;

		case TOGGLE:
			for (int i = 0; i < puntero_led->n_ciclos; i++) // Repetir el parpadeo según la cantidad de ciclos
			{
				switch (puntero_led->n_led) {
                    case LED_1:
                        LedToggle(LED_1);
                    break;
                    case LED_2:
                        LedToggle(LED_2);
                    break;
                    case LED_3:
                        LedToggle(LED_3);
                    break;
                }

				size_t j;
				for (j = 0; j < puntero_led->periodo ; j++) // Repetir el parpadeo según el periodo
				{	
					vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS); //portTICK_PERIOD_MS:  constante que indica cuántos milisegundos dura un tick en la configuración de FreeRTOS
				}
					
			}
			
	}
}

void app_main(void){
	LedsInit(); // Inicializar los LEDs de la placa
	
	struct leds led1, led2;

    led1.mode = ON;
    led1.n_led = LED_1;
    led1.n_ciclos = 5;
    led1.periodo = 10;

    led2.mode = TOGGLE;
    led2.n_led = LED_2;
    led2.n_ciclos = 20;
    led2.periodo = 5;  //5 * 100 ms = 500ms

    //ControlLeds(&led1);
    ControlLeds(&led2);

}
/*==================[end of file]============================================*/