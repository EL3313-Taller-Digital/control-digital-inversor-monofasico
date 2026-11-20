
#include <stdint.h>
#include "xil_printf.h"

/*
 * PROYECTO 4 - PRUEBA DIGITAL DEL REGULADOR PR
 *
 * FPGA: Basys 3
 * Procesador: MicroBlaze V
 * Entorno: Vitis 2026.1
 *
 * Prueba numerica independiente.
 * No utiliza XADC, AXI ni PWM.
 *
 * Frecuencia de referencia: 50 Hz
 * Frecuencia de muestreo: 10 kHz
 * Numero de muestras: 1000
 */

/* CONFIGURACION */

#define NUM_MUESTRAS      1000U
#define MUESTRAS_PERIODO  200U

#define CORRIENTE_BASE_MA 1500LL
#define REFERENCIA_MA     200LL
#define SENO_PICO         8192LL

/* COEFICIENTES ORIGINALES Q2.30 */

#define PR_A1_Q30  (-2143730768LL)
#define PR_A2_Q30  ( 1071047268LL)

#define PR_B0_Q30  (     673639LL)
#define PR_B2_Q30  (    -673639LL)

#define PR_KP_Q30  (  268435456LL)
#define Q30_UNIDAD ( 1073741824LL)

/* TABLA SENOIDAL ORIGINAL */

static const int16_t seno_cuarto[51] = {
       0,  257,  514,  771, 1027,
    1282, 1535, 1787, 2037, 2285,
    2531, 2775, 3016, 3253, 3488,
    3719, 3947, 4170, 4389, 4605,
    4815, 5021, 5222, 5417, 5608,
    5793, 5972, 6145, 6312, 6473,
    6627, 6775, 6917, 7051, 7179,
    7299, 7412, 7518, 7617, 7708,
    7791, 7867, 7935, 7995, 8047,
    8091, 8127, 8156, 8176, 8188,
    8192
};

/* VARIABLES VISIBLES EN DEBUG */

volatile uint32_t prueba_finalizada = 0U;
volatile uint32_t prueba_ok = 0U;
volatile uint32_t error_codigo = 0U;

volatile uint32_t muestras_procesadas = 0U;
volatile uint32_t saturaciones = 0U;

volatile int32_t referencia_actual = 0;
volatile int32_t corriente_medida = 0;
volatile int32_t error_actual = 0;

volatile int32_t salida_resonante = 0;
volatile int32_t salida_pr = 0;

volatile int32_t salida_maxima = -32768;
volatile int32_t salida_minima = 32767;

volatile int32_t referencia_maxima = 0;
volatile int32_t referencia_minima = 0;

/* HISTORIAL PARA INSPECCION */

#define HISTORIAL_TAM 200U

static int16_t historial_ref[HISTORIAL_TAM];
static int16_t historial_pr[HISTORIAL_TAM];

/* ESTADOS DEL REGULADOR */

static int32_t e1 = 0;
static int32_t e2 = 0;
static int32_t y1 = 0;
static int32_t y2 = 0;

/* SATURACION Q1.15 */

static int32_t saturar_q15(int64_t valor)
{
    if (valor > 32767LL) {
        saturaciones++;
        return 32767;
    }

    if (valor < -32768LL) {
        saturaciones++;
        return -32768;
    }

    return (int32_t)valor;
}

/* REFERENCIA SINUSOIDAL */

static int32_t referencia_virtual(uint32_t fase)
{
    fase %= MUESTRAS_PERIODO;

    if (fase <= 50U)
        return seno_cuarto[fase];

    if (fase <= 100U)
        return seno_cuarto[100U - fase];

    if (fase <= 150U)
        return -seno_cuarto[fase - 100U];

    return -seno_cuarto[200U - fase];
}

/* REGULADOR PR DISCRETO */

static void ejecutar_pr(int32_t error_q15)
{
    int64_t acumulador;
    int64_t salida;

    int32_t y0;
    int32_t u0;

    acumulador =
        -(PR_A1_Q30 * (int64_t)y1)
        -(PR_A2_Q30 * (int64_t)y2)
        +(PR_B0_Q30 * (int64_t)error_q15)
        +(PR_B2_Q30 * (int64_t)e2);

    y0 = saturar_q15(acumulador >> 30);

    salida =
        PR_KP_Q30 * (int64_t)error_q15
        + Q30_UNIDAD * (int64_t)y0;

    u0 = saturar_q15(salida >> 30);

    salida_resonante = y0;
    salida_pr = u0;

    e2 = e1;
    e1 = error_q15;

    y2 = y1;
    y1 = y0;
}

/* PROGRAMA PRINCIPAL */

int main(void)
{
    uint32_t k;
    uint32_t fase;
    int32_t referencia;
    int32_t error;

    xil_printf("\r\n");
    xil_printf("================================\r\n");
    xil_printf("PROYECTO 4 - PRUEBA PR\r\n");
    xil_printf("================================\r\n");
    xil_printf("Prueba numerica sin hardware externo\r\n");

    /* Comprobar tabla sinusoidal */

    if (referencia_virtual(0U) != 0 ||
        referencia_virtual(50U) != 8192 ||
        referencia_virtual(100U) != 0 ||
        referencia_virtual(150U) != -8192 ||
        referencia_virtual(200U) != 0) {

        error_codigo = 1U;
        goto finalizar;
    }

    /* Reiniciar estados */

    e1 = 0;
    e2 = 0;
    y1 = 0;
    y2 = 0;

    /* Simulacion de 5 periodos de 50 Hz */

    for (k = 0U; k < NUM_MUESTRAS; k++)
    {
        fase = k % MUESTRAS_PERIODO;

        /* Referencia de corriente: 0.20 A pico */

        referencia =
            (int32_t)(
                ((int64_t)referencia_virtual(fase)
                  * REFERENCIA_MA)
                / CORRIENTE_BASE_MA
            );

        /* Entrada simulada: corriente igual a cero */

        corriente_medida = 0;

        error = referencia - corriente_medida;

        referencia_actual = referencia;
        error_actual = error;

        /* Ejecutar PR */

        ejecutar_pr(error);

        /* Guardar un periodo para inspeccion */

        if (k >= (NUM_MUESTRAS - HISTORIAL_TAM))
        {
            uint32_t indice =
                k - (NUM_MUESTRAS - HISTORIAL_TAM);

            historial_ref[indice] =
                (int16_t)referencia;

            historial_pr[indice] =
                (int16_t)salida_pr;
        }

        /* Registrar extremos */

        if (salida_pr > salida_maxima)
            salida_maxima = salida_pr;

        if (salida_pr < salida_minima)
            salida_minima = salida_pr;

        if (referencia > referencia_maxima)
            referencia_maxima = referencia;

        if (referencia < referencia_minima)
            referencia_minima = referencia;

        muestras_procesadas++;

        /* Detener ante saturacion */

        if (saturaciones != 0U) {
            error_codigo = 2U;
            break;
        }
    }

    /* Comprobar cantidad de muestras */

    if (error_codigo == 0U &&
        muestras_procesadas != NUM_MUESTRAS) {
        error_codigo = 3U;
    }

finalizar:

    if (error_codigo == 0U)
        prueba_ok = 1U;
    else
        prueba_ok = 0U;

    prueba_finalizada = 1U;

    xil_printf("\r\nRESULTADOS:\r\n");

    xil_printf("Muestras: %lu\r\n",
               (unsigned long)muestras_procesadas);

    xil_printf("Saturaciones: %lu\r\n",
               (unsigned long)saturaciones);

    xil_printf("Referencia maxima: %ld\r\n",
               (long)referencia_maxima);

    xil_printf("Referencia minima: %ld\r\n",
               (long)referencia_minima);

    xil_printf("Salida PR maxima: %ld\r\n",
               (long)salida_maxima);

    xil_printf("Salida PR minima: %ld\r\n",
               (long)salida_minima);

    xil_printf("Error codigo: %lu\r\n",
               (unsigned long)error_codigo);

    if (prueba_ok == 1U)
        xil_printf("PRUEBA NUMERICA: PASS\r\n");
    else
        xil_printf("PRUEBA NUMERICA: FAIL\r\n");

    /*
     * No activa PWM.
     * No realiza escrituras AXI.
     * No lee el sensor.
     *
     * El resultado PASS solo indica que esta
     * prueba numerica paso sus comprobaciones.
     * No valida estabilidad ni control real.
     */

    return (prueba_ok == 1U) ? 0 : 1;
}
