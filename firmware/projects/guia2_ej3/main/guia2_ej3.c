/*! @mainpage Medidor de distancia por ultrasonido con interrupciones y puerto serie
 *
 * @section genDesc Descripción General
 *
 * Este programa mide la distancia utilizando el sensor ultrasónico HC-SR04 y muestra el valor en centímetros en un display LCD ITS-E0803.
 * Además, enciende LEDs según el rango de distancia detectado y permite pausar la medición o mantener el valor mostrado en el display mediante teclas.
 * El control de las teclas se realiza mediante **interrupciones**, y la actualización periódica de la medición y visualización se gestiona con un **timer**.
 * 
 * Los datos de medición se envían por **puerto serie** en formato: "XXX cm\r\n" (3 dígitos + espacio + unidad + cambio de línea).
 * También se puede controlar la EDU-ESP desde la PC mediante comandos serie:
 * - Tecla "O": Pausa/reanuda la medición (replica TEC1)
 * - Tecla "H": Activa/desactiva hold en display (replica TEC2)
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
 * |   UART          |  USB      |
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 26/09/2025 | Creación código guía 2 ejercicio 3            |
 *
 * @author Corona Narella (narella.corona@ingenieria.uner.edu.ar)
 *
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"
#include "hc_sr04.h"
#include "lcditse0803.h"
#include "switch.h" 
#include "timer_mcu.h"
#include "uart_mcu.h"

/*==================[macros and definitions]=================================*/
#define PERIODO_TIMER_US 1000000 // 1 segundo en microsegundos

/*==================[internal data definition]===============================*/
TaskHandle_t led_task_handle = NULL;

/** 
 * @brief Indica si el display está en modo hold.
 */
bool hold_lcd = false;

/** 
 * @brief Indica si la medición está activa (true) o pausada (false).
 */
bool medicion_activa = true;

/*==================[internal functions declaration]=========================*/

/**
 * @brief Actualiza el estado de los LEDs según la distancia medida.
 */
void ActualizarLeds(uint16_t distancia);

/**
 * @brief Callback de interrupción para TEC1.
 * 
 * Alterna el estado de la medición (activa/pausada) cada vez que se presiona la tecla 1.
 */
void AtiendoTecla1(void* args);

/**
 * @brief Callback de interrupción para TEC2.
 * 
 * Alterna el modo hold del display, permitiendo mantener o actualizar el valor mostrado, cada vez que se presiona la tecla 2.
 */
void AtiendoTecla2(void* args);

/**
 * @brief Función de callback del timer que notifica a la tarea de medición.
 */
void TimerCallback(void *param) {
    vTaskNotifyGiveFromISR(led_task_handle, NULL);
}


/**
 * @brief Tarea que mide la distancia, controla los LEDs y muestra el valor en el display.
 */
static void LedTask(void *pvParameters) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        uint16_t distancia = 0;

        if (medicion_activa) {
            distancia = HcSr04ReadDistanceInCentimeters();
        }
        UartSendString(UART_PC, (const char *)UartItoa(distancia, 10));
		UartSendString(UART_PC, " cm\r\n");
        ActualizarLeds(distancia);

        // Solo se actualiza el display si no está en hold
        if (!hold_lcd) {
            LcdItsE0803Write(distancia);
        }
    }
}

/*==================[external functions definition]==========================*/

void ActualizarLeds(uint16_t distancia) {
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

void AtiendoTecla1(void* args) {
    medicion_activa = !medicion_activa;
}

void AtiendoTecla2(void* args) {
    hold_lcd = !hold_lcd;
}

void funcTeclas(void* args) {
    uint8_t tecla;
    UartReadByte(UART_PC, &tecla);
    switch (tecla) {
        case 'O':
        case 'o':
            AtiendoTecla1(NULL);
            break;
        case 'H':
        case 'h':
            AtiendoTecla2(NULL);
            break;
        default:
            // Tecla no reconocida
            break;
    }
}

void app_main(void){
    SwitchesInit();
    LedsInit();
    HcSr04Init(GPIO_3, GPIO_2); // Echo en GPIO_3, Trigger en GPIO_2
    LcdItsE0803Init();

    // Configuración del UART
    serial_config_t uart_config = {
        .port = UART_PC,
        .baud_rate = 9600,
        .func_p = funcTeclas,
        .param_p = NULL
    };
    UartInit(&uart_config);

    // Activar interrupciones para las teclas
    SwitchActivInt(SWITCH_1, AtiendoTecla1, NULL);
    SwitchActivInt(SWITCH_2, AtiendoTecla2, NULL);

    xTaskCreate(LedTask, "LedTask", 2048, NULL, 4, &led_task_handle);

    // Configuración del timer
    timer_config_t config = {
        .timer = TIMER_A,
        .period = PERIODO_TIMER_US,
        .func_p = TimerCallback,
        .param_p = NULL
    };

    TimerInit(&config);
    TimerStart(TIMER_A);
}
/*==================[end of file]============================================*/