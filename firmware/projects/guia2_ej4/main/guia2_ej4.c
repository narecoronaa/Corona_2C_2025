/*! @mainpage Osciloscopio digital con ADC y transmisión UART
 *
 * @section genDesc Descripción General
 *
 * Esta aplicación digitaliza una señal analógica del canal CH1 del conversor AD
 * y la transmite por UART a un graficador de puerto serie de la PC.
 *
 * PARTE 1: Lectura de potenciómetro a 500Hz para visualización en Serial Oscilloscope
 * PARTE 2: Generación de señal ECG por DAC a 250Hz (cada 4ms) 
 *
 * @section hardConn Conexión de Hardware
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	CH1 ADC	 	| 	GPIO_1		|
 * | 	UART_PC	 	| 	USB			|
 * | 	CH0 DAC	    | 	GPIO_25		|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 24/10/2025 | Implementación osciloscopio digital            |
 *
 * @author Corona Narella (narella.corona@ingenieria.uner.edu.ar)
 */

/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "timer_mcu.h"
#include "uart_mcu.h"
#include "analog_io_mcu.h"

/*==================[macros and definitions]=================================*/
// PARTE 1: Timer para lectura de potenciómetro (500Hz)
/** Período del timer A para muestreo ADC: 500Hz = 2000μs */
#define TIMER_ADC_PERIOD_US     2000

// PARTE 2: Timer para generación ECG (250Hz - cada 4ms) 
#define TIMER_ECG_PERIOD_US     4000
#define BUFFER_SIZE             231

/** Canal ADC a usar para potenciómetro */
#define ADC_CHANNEL             CH1

/** Velocidad UART para transmisión */
#define UART_BAUD_RATE          115200

/*==================[internal data definition]===============================*/
// PARTE 1: Variables para lectura de potenciómetro
/** Handle de la tarea de procesamiento ADC */
TaskHandle_t adc_task_handle = NULL;

/** Variable para almacenar la conversión ADC del potenciómetro (12 bits) */
uint16_t valor_adc = 0;

// PARTE 2: Variables para generación ECG 
unsigned char ecg[BUFFER_SIZE] = {
    17,17,17,17,17,17,17,17,17,17,17,18,18,18,17,17,17,17,17,17,17,18,18,18,18,18,18,18,17,17,16,16,16,16,17,17,18,18,18,17,17,17,17,
    18,18,19,21,22,24,25,26,27,28,29,31,32,33,34,34,35,37,38,37,34,29,24,19,15,14,15,16,17,17,17,16,15,14,13,13,13,13,13,13,13,12,12,
    10,6,2,3,15,43,88,145,199,237,252,242,211,167,117,70,35,16,14,22,32,38,37,32,27,24,24,26,27,28,28,27,28,28,30,31,31,31,32,33,34,36,
    38,39,40,41,42,43,45,47,49,51,53,55,57,60,62,65,68,71,75,79,83,87,92,97,101,106,111,116,121,125,129,133,136,138,139,140,140,139,137,
    133,129,123,117,109,101,92,84,77,70,64,58,52,47,42,39,36,34,31,30,28,27,26,25,25,25,25,25,25,25,25,24,24,24,24,25,25,25,25,25,25,25,
    24,24,24,24,24,24,24,24,23,23,22,22,21,21,21,20,20,20,20,20,19,19,18,18,18,19,19,19,19,18,17,17,18,18,18,18,18,18,18,18,17,17,17,17,
    17,17,17
};

int indice_ecg = 0;

/*==================[internal functions declaration]=========================*/
// PARTE 1: Funciones para lectura de potenciómetro
/**
 * @brief Callback del timer A - dispara conversión ADC cada 2ms (500Hz)
 * Usado para muestrear el potenciómetro conectado al ADC
 */
void TimerAdcCallback(void *param);

/**
 * @brief Tarea que procesa y transmite datos del ADC (potenciómetro)
 */
static void AdcTask(void *pvParameters);

// PARTE 2: Funciones para generación ECG
void TimerEcgCallback(void *param);
void GenerarSeñalECG(void);

/*==================[external functions definition]==========================*/

// PARTE 2: Implementación de generación ECG 
void GenerarSeñalECG(void) {
    uint8_t valor_dac = ecg[indice_ecg];
    AnalogOutputWrite(valor_dac);
    indice_ecg = (indice_ecg + 1) % BUFFER_SIZE;
}

void TimerEcgCallback(void *param) {
    GenerarSeñalECG();
}

// PARTE 1: Implementación de lectura ADC
void TimerAdcCallback(void *param) {
    // PARTE 1: Notifica a la tarea ADC que tome una muestra del potenciómetro
    // Se ejecuta cada 2ms (500Hz) para digitalizar la señal analógica
    vTaskNotifyGiveFromISR(adc_task_handle, NULL);
}

static void AdcTask(void *pvParameters) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        // Lee el conversor ADC conectado al potenciómetro (12 bits: 0-4095)
        AnalogInputReadSingle(ADC_CHANNEL, &valor_adc);
        
        // Convierte el valor digital a voltaje (0-3.3V)
        float voltage = (valor_adc * 3.3) / 4095.0;
        
        // FORMATO PARA SERIAL OSCILLOSCOPE: Solo valores numéricos
        char buffer[32];
        sprintf(buffer, "%.3f\r\n", voltage);  
        UartSendString(UART_PC, buffer);
    }
}

void app_main(void) {
    // PARTE 1: Inicialización del ADC para lectura de potenciómetro
    analog_input_config_t adc_config = {
        .input = ADC_CHANNEL,   // Canal CH1 para potenciómetro
        .mode = ADC_SINGLE,     // Conversión individual
        .func_p = NULL,         
        .param_p = NULL,
        .sample_frec = 0        
    };
    AnalogInputInit(&adc_config);
    
    // PARTE 2: Inicialización del DAC 
    AnalogOutputInit();

    // Inicialización UART 
    serial_config_t uart_config = {
        .port = UART_PC,
        .baud_rate = UART_BAUD_RATE,
        .func_p = NULL,         
        .param_p = NULL
    };
    UartInit(&uart_config);
    
    // PARTE 1: Configuración timer A para muestreo ADC (500Hz)
    timer_config_t timer_adc_config = {
        .timer = TIMER_A,
        .period = TIMER_ADC_PERIOD_US,  // Cada 2000μs = 500Hz
        .func_p = TimerAdcCallback,     // Callback para lectura ADC
        .param_p = NULL
    };
    TimerInit(&timer_adc_config);
    
    // PARTE 2: Configuración timer B para generación ECG (250Hz)
    timer_config_t timer_ecg_config = {
        .timer = TIMER_B,
        .period = TIMER_ECG_PERIOD_US,
        .func_p = TimerEcgCallback,
        .param_p = NULL
    };
    TimerInit(&timer_ecg_config);
    
    // PARTE 1: Crear tarea para procesamiento del ADC
    xTaskCreate(AdcTask, "AdcTask", 4096, NULL, 5, &adc_task_handle);
    
    TimerStart(TIMER_A);  // PARTE 1: Inicia muestreo de potenciómetro
    TimerStart(TIMER_B);  // PARTE 2: Generación ECG 
    
}

/*==================[end of file]============================================*/