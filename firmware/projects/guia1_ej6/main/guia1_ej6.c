/*! @mainpage Template
 *
 * @section genDesc General Description
 *
 * Este programa permite mostrar un número de hasta 3 dígitos en un display multiplexado utilizando conversores BCD-7 segmentos (CD4543).
 * Convierte un número decimal a BCD, selecciona el dígito a mostrar y envía el valor BCD a los pines GPIO correspondientes.
 * Reutiliza funciones de ejercicios anteriores para la conversión y el manejo de los pines.
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
#include <gpio_mcu.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
/*==================[macros and definitions]=================================*/
#define N_BITS 4
/*==================[internal data definition]===============================*/

/**
 * @brief Estructura para configuración de pines GPIO.
 */
typedef struct
{
    gpio_t pin;			/*!< GPIO pin number */
    io_t dir;			/*!< GPIO direction '0' IN;  '1' OUT*/
} gpioConf_t;

/*==================[internal functions declaration]=========================*/

/**
 * @brief Convierte un número decimal a un arreglo de dígitos BCD.
 * 
 * @param data Número a convertir (uint32_t).
 * @param digits Cantidad de dígitos de salida.
 * @param bcd_number Puntero al arreglo donde se almacenarán los dígitos BCD.
 * @return int8_t 0 si la conversión fue exitosa, -1 si el número no cabe en la cantidad de dígitos.
 */
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number);

/**
 * @brief Envía un dígito BCD a los pines GPIO correspondientes.
 * 
 * @param digit Dígito BCD a mostrar (0-9).
 * @param gpio_config Vector de estructuras gpioConf_t que mappa los bits BCD a los pines GPIO.
 */
void BCDtoGPIO(uint8_t digit,  gpioConf_t *gpio_config);

/**
 * @brief Muestra un número en un display multiplexado usando BCD y selección de dígito.
 * 
 * @param data Número a mostrar (máximo 'digits' dígitos).
 * @param digits Cantidad de dígitos a mostrar.
 * @param bcd_gpio Vector de estructuras gpioConf_t para los pines BCD (4 elementos).
 * @param sel_gpio Vector de estructuras gpioConf_t para los pines de selección de dígito (tantos como dígitos).
 * 
 * Esta función convierte el número a BCD, selecciona cada dígito del display y envía el valor BCD a los pines correspondientes.
 * Incluye un pulso de latch para asegurar el registro de datos en el CD4543.
 */
void DisplayNumberOnLcd(uint32_t data, uint8_t digits, gpioConf_t *bcd_gpio, gpioConf_t *sel_gpio);

/*==================[external functions definition]==========================*/

//Función ejercicio 4
int8_t convertToBcdArray(uint32_t data, uint8_t digits, uint8_t *bcd_number) {
    int max_value = 1;
    for (int i = 0; i < digits; i++) {
        max_value *= 10;
    }
    
    if (data >= max_value) {
        printf("Error: El número %lu no cabe en %d dígitos\n", (unsigned long)data, digits);
        return -1; // Error
    }

    // Convertimos el número a BCD y almacenamos cada dígito en el arreglo
    for (int i = 0; i < digits; i++) {
        bcd_number[digits - 1 - i] = data % 10;
        printf("Guardando %d en bcd_number[%d]\n", bcd_number[digits - 1 - i], digits - 1 - i);
        data /= 10;
    }

    printf("Arreglo final BCD: ");
    for (int i = 0; i < digits; i++) {
        printf("%d ", bcd_number[i]);
    }
    printf("\n");

    return 0; // Éxito
}

//Función ejercicio 5
void BCDtoGPIO(uint8_t digit,  gpioConf_t *gpio_config)
{

    for (int i = 0; i < N_BITS; i++)
    {
        int bit = (digit & (1 << i)) ? 1 : 0;
        printf("  Bit %d: %d -> Pin %d\n", i, bit, gpio_config[i].pin);
        if(bit == 0)
        {
            GPIOOff(gpio_config[i].pin);			
        }
        else
        {
            GPIOOn(gpio_config[i].pin);
        }
    }
}

void DisplayNumberOnLcd(uint32_t data, uint8_t digits, gpioConf_t *bcd_gpio, gpioConf_t *sel_gpio)
{
    uint8_t bcd_array[digits];
    if (convertToBcdArray(data, digits, bcd_array) != 0) {
        printf("DisplayNumberOnLcd: Error en la conversión a BCD\n");
        return;
    }

    for (int i = 0; i < digits; i++) {
        printf("Mostrando dígito %d (valor %d) en el display, seleccionando pin %d\n", i, bcd_array[i], sel_gpio[i].pin);
        // Selecciona el dígito a mostrar (activar el pin correspondiente)
        for (int j = 0; j < digits; j++) {
            if (j == i) {
                GPIOOn(sel_gpio[j].pin);
                printf("  Activando selección de dígito en pin %d\n", sel_gpio[j].pin);
            } else {
                GPIOOff(sel_gpio[j].pin);
            }
        }

        // Envía el valor BCD al display usando la función reutilizada
        BCDtoGPIO(bcd_array[i], bcd_gpio);
    }
}

void app_main(void){
    uint8_t digits = 3;
    uint32_t numero = 682;

    gpioConf_t bcd_gpio[N_BITS] = {
        {GPIO_20, 1},
        {GPIO_21, 1},
        {GPIO_22, 1},
        {GPIO_23, 1}
    };

    gpioConf_t sel_gpio[3] = {
        {GPIO_19, 1},
        {GPIO_18, 1},
        {GPIO_9,  1}
    };
	
	// Inicialización de los pines GPIO
    for (int i = 0; i < N_BITS; i++)
    {
        GPIOInit(bcd_gpio[i].pin, bcd_gpio[i].dir);
    }


	for (int i = 0; i < 3; i++)
	{
    	GPIOInit(sel_gpio[i].pin, sel_gpio[i].dir);
	}
	
    DisplayNumberOnLcd(numero, digits, bcd_gpio, sel_gpio);
    
}
/*==================[end of file]============================================*/