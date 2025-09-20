/*! @mainpage Medición de distancia y visualización en display
 *
 * @section genDesc Descripción General
 *
 * Este programa mide la distancia utilizando el sensor ultrasónico HC-SR04 y muestra el valor en centímetros en un display LCD ITS-E0803.
 * Además, enciende LEDs según el rango de distancia detectado y permite pausar la medición o mantener el valor mostrado en el display mediante teclas.
 *
 * - Si la distancia es menor a 10 cm, todos los LEDs están apagados.
 * - Si la distancia está entre 10 y 20 cm, se enciende LED_1.
 * - Si la distancia está entre 20 y 30 cm, se encienden LED_1 y LED_2.
 * - Si la distancia es mayor a 30 cm, se encienden LED_1, LED_2 y LED_3.
 *
 * Tecla 1: Pausa o reanuda la medición.
 * Tecla 2: Mantiene el valor actual en el display (hold).
 *
 * @section hardConn Conexión de Hardware
 *
 * |    Peripheral   |   ESP32   |
 * |:--------------: |:---------:|
 * |   HC-SR04 TRIG  |  GPIO_2   |
 * |   HC-SR04 ECHO  |  GPIO_3   |
 * |   BCD0          |  GPIO_20  |
 * |   BCD1          |  GPIO_21  |
 * |   BCD2          |  GPIO_22  |
 * |   BCD3          |  GPIO_23  |
 * |   SEL1          |  GPIO_19  |
 * |   SEL2          |  GPIO_18  |
 * |   SEL3          |  GPIO_9   |
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 13/09/2025 | Documentación y código actualizado             |
 *
 * @author Corona Narella (narella.corona@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "switch.h" 

/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD_LED 100

/*==================[internal data definition]===============================*/
TaskHandle_t led_task_handle = NULL;
TaskHandle_t teclas_task_handle = NULL;

/** 
 * @brief Indica si el display está en modo hold (true: mantiene el valor, false: muestra la distancia actual).
 */
bool hold_lcd = false;

/** 
 * @brief Indica si la medición está activa (true) o pausada (false).
 */
bool medicion_activa = true;

/**
 * @brief Actualiza el estado de los LEDs según la distancia medida.
 * 
 * @param distancia Distancia en centímetros.
 */
void ActualizarLeds(uint16_t distancia);

/**
 * @brief Tarea que mide la distancia, controla los LEDs y muestra el valor en el display.
 */
static void LedTask(void *pvParameters) {
    while (1) {
        uint16_t distancia = 0;

        if (medicion_activa) {
            distancia = HcSr04ReadDistanceInCentimeters();
        } else {
                    // Si la medición está pausada, no actualiza la distancia
                }

                ActualizarLeds(distancia);

                // Solo se actualiza el display si no está en hold
                if (!hold_lcd) {
                    LcdItsE0803Write(distancia);
                }

        vTaskDelay(CONFIG_BLINK_PERIOD_LED / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Tarea que gestiona las teclas para pausar la medición y activar el hold en el display.
 */
static void TeclasTask(void *pvParameters) {
    
    uint8_t teclas;
    bool tecla1_anterior = false;
    bool tecla2_anterior = false;

    while (1) {
        teclas = SwitchesRead();

        // Tecla 1: Toggle medición 
        bool sw1 = teclas & SWITCH_1;
        if (sw1 && !tecla1_anterior) {
            medicion_activa = !medicion_activa;
        }
        tecla1_anterior = sw1;

        // Tecla 2: Toggle hold 
        bool sw2 = teclas & SWITCH_2;
        if (sw2 && !tecla2_anterior) {
            hold_lcd = !hold_lcd;
        }
        tecla2_anterior = sw2;

        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
} 

/*==================[external functions definition]==========================*/
static void ActualizarLeds(uint16_t distancia) {
    LedOff(LED_1);
    LedOff(LED_2);
    LedOff(LED_3);

    if (distancia < 10) {
        // Todos los LEDs apagados
    } else if (distancia < 20) {
        LedOn(LED_1);
    } else if (distancia < 30) {
        LedOn(LED_1);
        LedOn(LED_2);
    } else {
        LedOn(LED_1);
        LedOn(LED_2);
        LedOn(LED_3);
    }
}

void app_main(void){
    SwitchesInit();
    LedsInit();
    HcSr04Init(GPIO_3, GPIO_2); // Echo en GPIO_3, Trigger en GPIO_2
    LcdItsE0803Init();

    xTaskCreate(LedTask, "LedTask", 2048, NULL, 4, &led_task_handle);
    xTaskCreate(TeclasTask, "TeclasTask", 2048, NULL, 5, &teclas_task_handle);
}
/*==================[end of file]============================================*/