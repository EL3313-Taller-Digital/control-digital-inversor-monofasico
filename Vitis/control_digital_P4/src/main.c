#include <stdint.h>

#include "xparameters.h"
#include "xil_io.h"
#include "xil_exception.h"
#include "xintc.h"
#include "xstatus.h"
#include "xsysmon_hw.h"

/*
 * ============================================================
 * PROYECTO 4 - CONTROL DIGITAL DE INVERSOR MONOFASICO
 * Basys 3 - MicroBlaze V - Vitis 2026.1
 *
 * MODO POR DEFECTO: PRUEBA NUMERICA PR CON PWM DESHABILITADO
 *
 * IMPORTANTE:
 * - No conectar etapa de potencia.
 * - Calibrar el ACS712 SIN corriente de carga al iniciar.
 * - El PWM permanece fisicamente deshabilitado en modo sensor.
 * - Coeficientes PR provisionales.
 * - El modo PWM legado se conserva solo para osciloscopio sin potencia.
 * ============================================================
 */

/* CONFIGURACION */

/* 1: ejecutar 1000 muestras del PR, con PWM apagado y escritura AXI de DUTY. */
#define MODO_PRUEBA_DIGITAL_PR 1U

/* 1: probar XADC y calibrar ACS712. PWM siempre apagado. */
#define MODO_CALIBRACION_XADC    1U

/* Solo habilitar en una prueba posterior con osciloscopio y SIN potencia. */
#define HABILITAR_PWM_EN_PRUEBA  0U
#define USAR_CORRIENTE_CALIBRADA 0U

/* Todavia prohibido activar control real de la etapa de potencia. */
#if !MODO_PRUEBA_DIGITAL_PR && !MODO_CALIBRACION_XADC && USAR_CORRIENTE_CALIBRADA
#error "Control real de potencia no validado: falta proteccion analogica y PR"
#endif
#if !MODO_PRUEBA_DIGITAL_PR && !MODO_CALIBRACION_XADC && !HABILITAR_PWM_EN_PRUEBA
#error "Para volver a prueba de osciloscopio, habilitar PWM expresamente"
#endif

/* ========================================
 * CONFIGURACION DEL SENSOR ACS712 DE 20 A
 * ======================================== */

/* Cero aproximado en la ENTRADA del XADC despues del divisor.
 * Se reemplaza automaticamente al arrancar sin corriente. */
#define SENSOR_OFFSET_MV       100LL

/* Sensibilidad del LM358 ANTES del divisor: +270 mV/A. */
#define SENSOR_GANANCIA_MV_A   270LL

/* Divisor fisico: LM358 -- 47 kOhm -- VAUX6_P -- 10 kOhm -- GND.
 * Factor V_XADC / V_LM358 = 10 / (47 + 10) = 10/57. */
#define DIVISOR_R1_KOHM       47LL
#define DIVISOR_R2_KOHM       10LL
#define DIVISOR_TOTAL_KOHM    (DIVISOR_R1_KOHM + DIVISOR_R2_KOHM)

/* Ventana del cero y limites supervisados sobre el voltaje del XADC. */
#define SENSOR_OFFSET_MIN_MV   70
#define SENSOR_OFFSET_MAX_MV   160

/* Escala interna: 8192 cuentas equivalen a 1.5 A */
#define CORRIENTE_BASE_MA 1500LL
#define SENO_PICO_CUENTAS 8192LL

/* Referencia inicial de corriente: 0.2 A pico */
#define REFERENCIA_PICO_MA     200LL


/* Datos para calibracion y monitor USB-UART sin control de potencia. */
#define SENSOR_CAL_MUESTRAS      256U
#define SENSOR_BUFFER_MUESTRAS   1024U
#define SENSOR_INTERVALO_TICKS   10000U   /* objetivo de lectura: 100 us */
#define SENSOR_REPORTE_TICKS     20000000U /* reporte ~200 ms */
#define SENSOR_TIEMPO_ESTABLE    20000000U /* esperar 200 ms al arrancar */
#define SENSOR_MIN_CODIGO        41U      /* ~10 mV en XADC */
#define SENSOR_MAX_CODIGO        3686U    /* ~900 mV en XADC */

/* DIRECCIONES AXI */

#define PWM_BASE     XPAR_PWM_AXI_0_BASEADDR
#define XADC_BASE    XPAR_XADC_WIZ_0_BASEADDR
#define TIMER_BASE   XPAR_AXI_TIMER_0_BASEADDR

/* MicroBlaze V 2026.1: XIntc_Initialize recibe BASEADDR.
 * Mantener la direccion del AXI INTC exportada por Vivado. */
#if defined(XPAR_XINTC_0_BASEADDR)
#define INTC_BASE XPAR_XINTC_0_BASEADDR
#elif defined(XPAR_INTC_0_BASEADDR)
#define INTC_BASE XPAR_INTC_0_BASEADDR
#else
#define INTC_BASE 0x41200000U /* direccion verificada en este proyecto */
#endif

/* UARTLite existente en el mapa de memoria del proyecto: 0x40600000.
 * Si reexportas el XSA, confirma estas direcciones en xparameters.h. */
#if defined(XPAR_UARTLITE_0_BASEADDR)
#define UART_BASE XPAR_UARTLITE_0_BASEADDR
#elif defined(XPAR_AXI_UARTLITE_0_BASEADDR)
#define UART_BASE XPAR_AXI_UARTLITE_0_BASEADDR
#else
#define UART_BASE 0x40600000U
#endif

/* GPIO del boton BTNU, si esta conectado a AXI GPIO canal 1. */
#if defined(XPAR_AXI_GPIO_0_BASEADDR)
#define GPIO_BASE XPAR_AXI_GPIO_0_BASEADDR
#elif defined(XPAR_GPIO_0_BASEADDR)
#define GPIO_BASE XPAR_GPIO_0_BASEADDR
#else
#define GPIO_BASE 0x40000000U
#endif

#define USAR_BOTON_UART 0U /* activar solo tras comprobar GPIO/BTNU */

/* INTERRUPCION PWM */

#define PWM_IRQ_ID 1U

/* REGISTROS PWM */

#define PWM_CONTROL   0x00U
#define PWM_DUTY      0x04U
#define PWM_DEADTIME  0x08U
#define PWM_STATUS    0x0CU

#define PWM_APAGADO   0x02U
#define PWM_ENCENDIDO 0x03U

/* TIEMPO MUERTO: 500 ns a 100 MHz */

#define DEADTIME_50 50U

/* REGISTROS AXI TIMER */

#define TIMER_TCSR 0x00U
#define TIMER_TLR  0x04U
#define TIMER_TCR  0x08U

/* TEMPORIZACION */

#define TICKS_PERIODO       10000U
#define LIMITE_TRABAJO      9000U
#define WATCHDOG_IRQ_TICKS  (2U * TICKS_PERIODO)

/*
 * TICKS_PERIODO:
 * 10000 ciclos a 100 MHz = 100 us.
 * Frecuencia de muestreo esperada = 10 kHz.
 *
 * WATCHDOG:
 * Si no llega una interrupcion durante dos periodos,
 * se apaga el PWM.
 */

/* COEFICIENTES PR PROVISIONALES Q2.30 */

#define PR_A1_Q30   (-2143730768LL)
#define PR_A2_Q30   ( 1071047268LL)

#define PR_B0_Q30   (     673639LL)
#define PR_B2_Q30   (    -673639LL)

#define PR_KP_Q30   (  268435456LL)
#define Q30_UNIDAD  ( 1073741824LL)

/*
 * REFERENCIA SINUSOIDAL VIRTUAL
 *
 * Frecuencia: 50 Hz.
 * Frecuencia de control: 10 kHz.
 * Muestras por periodo: 200.
 *
 * Referencia de corriente: 0.20 A pico.
 * Corriente base: 1.50 A.
 *
 * Escala interna:
 * 8192 cuentas equivalen a 1.50 A.
 *
 * Referencia maxima aproximada:
 * 1092 cuentas.
 *
 * La calibracion del sensor esta pendiente.
 */

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

/* ============================================================
 * VARIABLES PARA WATCH
 * ============================================================
 */

/* Estado general */

volatile uint32_t modo_prueba_pwm = 1U;
volatile uint32_t sistema_activo = 0U;

/* Interrupciones */

volatile uint32_t irq_pwm_inicializado = 0U;
volatile uint32_t irq_pwm_contador = 0U;
volatile uint32_t irq_pwm_periodo_ticks = 0U;

/* Control PR */

volatile uint32_t pr_ejecutado = 0U;
volatile uint32_t pr_muestras = 0U;
volatile uint32_t pr_muestras_perdidas = 0U;
volatile uint32_t pr_atrasos = 0U;
volatile uint32_t pr_saturaciones = 0U;

/* Tiempo de procesamiento */

volatile uint32_t trabajo_ticks = 0U;
volatile uint32_t trabajo_ticks_max = 0U;

/* XADC */

volatile uint32_t xadc_vaux6_codigo = 0U;
volatile uint32_t xadc_estado_leido = 0U;

/* Magnitudes visibles desde la ventana WATCH de Vitis. */
volatile int32_t xadc_voltaje_mV = 0;
volatile int32_t corriente_medida_mA = 0;
volatile uint32_t sensor_offset_adc = 0U;
volatile int32_t sensor_offset_mV = SENSOR_OFFSET_MV;
volatile uint32_t sensor_calibrado = 0U;
volatile uint32_t sensor_error_rango = 0U;
volatile uint32_t sensor_muestras = 0U;
volatile uint32_t sensor_exportaciones = 0U;
volatile uint32_t uart_errores_tx = 0U;

/* Variables del controlador */

volatile int32_t pr_ref_q15 = 0;
volatile int32_t corriente_medida_q15 = 0;
volatile int32_t pr_error_q15 = 0;
volatile int32_t pr_resonante_q15 = 0;
volatile int32_t pr_salida_q15 = 0;

/* PWM */

volatile uint32_t pwm_duty_axi_ok = 0U;
volatile uint32_t pwm_deadtime_leido = 0U;
volatile uint32_t pwm_control_final = 0U;
volatile uint32_t pwm_estado_final = 0U;
volatile uint32_t pwm_apagado_final = 0U;

/* Estado de la prueba */

volatile uint32_t prueba_finalizada = 0U;
volatile uint32_t prueba_ok = 0U;
volatile uint32_t error_codigo = 0U;

/* ESTADOS INTERNOS DEL PR */

static int32_t e1 = 0;
static int32_t e2 = 0;
static int32_t y1 = 0;
static int32_t y2 = 0;

/* ============================================================
 * INTERRUPCION PWM
 * ============================================================
 */

static XIntc intc;

static volatile uint32_t ultima_marca_irq = 0U;

static void pwm_callback(void *referencia)
{
    uint32_t marca;

    (void)referencia;

    marca = Xil_In32(TIMER_BASE + TIMER_TCR);

    if (irq_pwm_contador != 0U)
    {
        irq_pwm_periodo_ticks =
            (uint32_t)(marca - ultima_marca_irq);
    }

    ultima_marca_irq = marca;

    irq_pwm_contador++;
}

/* ============================================================
 * SATURACION Q1.15
 * ============================================================
 */

static int32_t saturar_q15(int64_t valor)
{
    if (valor > 32767LL)
    {
        pr_saturaciones++;
        return 32767;
    }

    if (valor < -32768LL)
    {
        pr_saturaciones++;
        return -32768;
    }

    return (int32_t)valor;
}

/* ============================================================
 * REFERENCIA SINUSOIDAL VIRTUAL
 * ============================================================
 */

static int32_t referencia_virtual(uint32_t fase)
{
    fase %= 200U;

    if (fase <= 50U)
        return seno_cuarto[fase];

    if (fase <= 100U)
        return seno_cuarto[100U - fase];

    if (fase <= 150U)
        return -seno_cuarto[fase - 100U];

    return -seno_cuarto[200U - fase];
}


/* ========================================
 * LECTURA Y CONVERSION DEL ACS712
 * ======================================== */

static int32_t leer_corriente_q15(uint32_t codigo_adc)
{
    int64_t voltaje_mV;
    int64_t corriente_mA;
    int64_t corriente_cuentas;

    /* Limitar lectura del ADC a 12 bits */
    if (codigo_adc > 4095U)
        codigo_adc = 4095U;

    /* El XADC entrega 4096 niveles en modo unipolar 0..1V. */
    voltaje_mV =
        ((int64_t)codigo_adc * 1000LL + 2048LL) / 4096LL;
    xadc_voltaje_mV = (int32_t)voltaje_mV;

    /* En el montaje medido, el voltaje crece con I positiva.
     * Usar el offset tomado en la calibracion de arranque. */
    /* Primero recuperar el voltaje del LM358 antes del divisor.
     * ΔV_XADC = ΔV_LM358 * R2 / (R1+R2).
     * I_mA = ΔADC * 1e6 * (R1+R2) / (4096 * 270 * R2).
     * Mantener cuentas originales evita perder precision por redondeo mV. */
    corriente_mA =
        (((int64_t)((int32_t)codigo_adc - (int32_t)sensor_offset_adc))
         * 1000000LL * DIVISOR_TOTAL_KOHM)
         / (4096LL * SENSOR_GANANCIA_MV_A * DIVISOR_R2_KOHM);
    corriente_medida_mA = (int32_t)corriente_mA;
    sensor_error_rango =
        (codigo_adc < SENSOR_MIN_CODIGO ||
         codigo_adc > SENSOR_MAX_CODIGO) ? 1U : 0U;

    /* Conversion a la escala interna del PR */
    corriente_cuentas =
        (corriente_mA * SENO_PICO_CUENTAS)
        / CORRIENTE_BASE_MA;

    /* Saturacion numerica */
    if (corriente_cuentas > 32767LL)
    {
        pr_saturaciones++;
        return 32767;
    }

    if (corriente_cuentas < -32768LL)
    {
        pr_saturaciones++;
        return -32768;
    }

    return (int32_t)corriente_cuentas;
}

/* ============================================================
 * REGULADOR PR DISCRETO
 * ============================================================
 */

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

    pr_error_q15 = error_q15;
    pr_resonante_q15 = y0;
    pr_salida_q15 = u0;

    e2 = e1;
    e1 = error_q15;

    y2 = y1;
    y1 = y0;

    pr_ejecutado = 1U;
}

/* ============================================================
 * INICIALIZAR AXI TIMER
 * ============================================================
 */

static void iniciar_timer(void)
{
    Xil_Out32(
        TIMER_BASE + TIMER_TCSR,
        0U
    );

    Xil_Out32(
        TIMER_BASE + TIMER_TLR,
        0U
    );

    Xil_Out32(
        TIMER_BASE + TIMER_TCSR,
        0x20U
    );

    Xil_Out32(
        TIMER_BASE + TIMER_TCSR,
        0x90U
    );
}

/* ============================================================
 * MODO DE CALIBRACION DEL SENSOR EN FPGA (PWM DESHABILITADO)
 * ============================================================
 * UARTLite usa los registros AXI en la direccion del proyecto.
 * Permite observar y exportar mediciones sin usar Arduino como ADC.
 */

#define UART_RX_DATA   0x00U
#define UART_TX_DATA   0x04U
#define UART_STATUS    0x08U
#define UART_RX_VALID  0x01U
#define UART_TX_FULL   0x08U

typedef struct {
    uint16_t codigo;
    int16_t corriente_mA;
} MuestraADC;

static MuestraADC historial[SENSOR_BUFFER_MUESTRAS];
static uint32_t historial_indice = 0U;
static uint32_t historial_cantidad = 0U;

static void uart_caracter(char c)
{
    uint32_t intentos = 1000000U;
    while ((Xil_In32(UART_BASE + UART_STATUS) & UART_TX_FULL) != 0U)
    {
        if (--intentos == 0U)
        {
            uart_errores_tx++;
            return;
        }
    }
    Xil_Out32(UART_BASE + UART_TX_DATA, (uint32_t)(uint8_t)c);
}

static void uart_texto(const char *s)
{
    while (*s != '\0')
    {
        if (*s == '\n') uart_caracter('\r');
        uart_caracter(*s++);
    }
}

static void uart_numero(uint32_t valor)
{
    char buf[11];
    uint32_t n = 0U;
    do {
        buf[n++] = (char)('0' + (valor % 10U));
        valor /= 10U;
    } while (valor != 0U);
    while (n != 0U) uart_caracter(buf[--n]);
}

static void uart_entero(int32_t valor)
{
    if (valor < 0)
    {
        uart_caracter('-');
        uart_numero((uint32_t)(-(int64_t)valor));
    }
    else uart_numero((uint32_t)valor);
}

static uint32_t uart_hay_comando(void)
{
    return Xil_In32(UART_BASE + UART_STATUS) & UART_RX_VALID;
}

static char uart_leer_comando(void)
{
    return (char)(Xil_In32(UART_BASE + UART_RX_DATA) & 0xFFU);
}

static void esperar_ciclos(uint32_t ciclos)
{
    uint32_t inicio = Xil_In32(TIMER_BASE + TIMER_TCR);
    while ((uint32_t)(Xil_In32(TIMER_BASE + TIMER_TCR) - inicio) < ciclos) { }
}

static uint32_t leer_vaux6_12bit(void)
{
    uint32_t raw;
    xadc_estado_leido = Xil_In32(XADC_BASE + XSM_SR_OFFSET);
    raw = Xil_In32(XADC_BASE + XSM_AUX06_OFFSET);
    xadc_vaux6_codigo = (raw >> 4U) & 0x0FFFU;
    return xadc_vaux6_codigo;
}

/* IMPORTANTE: ejecutar SOLAMENTE sin corriente por IP+/IP-. */
static void calibrar_cero_xadc(void)
{
    uint64_t suma = 0U;
    uint32_t k;
    uint32_t codigo;
    uint32_t promedio;

    sensor_calibrado = 0U;
    uart_texto("CALIBRANDO CERO XADC (carga APAGADA)...\n");
    esperar_ciclos(SENSOR_TIEMPO_ESTABLE);

    for (k = 0U; k < SENSOR_CAL_MUESTRAS; ++k)
    {
        codigo = leer_vaux6_12bit();
        suma += codigo;
        esperar_ciclos(SENSOR_INTERVALO_TICKS);
    }
    promedio = (uint32_t)((suma + (SENSOR_CAL_MUESTRAS / 2U)) /
                          SENSOR_CAL_MUESTRAS);

    sensor_offset_adc = promedio;
    sensor_offset_mV = (int32_t)(((uint64_t)promedio * 1000U + 2048U) / 4096U);

    /* Solo aceptar un punto de cero plausible; no acredita ausencia de corriente. */
    if (sensor_offset_mV < SENSOR_OFFSET_MIN_MV ||
        sensor_offset_mV > SENSOR_OFFSET_MAX_MV)
    {
        uart_texto("ERROR: cero fuera de 70..160 mV. Revisa divisor/cableado/VAUX6.\n");
        error_codigo = 20U;
        return;
    }
    sensor_calibrado = 1U;
    uart_texto("CERO_ADC="); uart_numero(promedio);
    uart_texto(" OFFSET_mV="); uart_entero(sensor_offset_mV);
    uart_texto("\n");
}

static void guardar_muestra(uint32_t codigo, int32_t i_mA)
{
    if (i_mA > 32767) i_mA = 32767;
    if (i_mA < -32768) i_mA = -32768;
    historial[historial_indice].codigo = (uint16_t)codigo;
    historial[historial_indice].corriente_mA = (int16_t)i_mA;
    historial_indice = (historial_indice + 1U) % SENSOR_BUFFER_MUESTRAS;
    if (historial_cantidad < SENSOR_BUFFER_MUESTRAS)
        historial_cantidad++;
}

static void imprimir_medicion(void)
{
    uart_texto("ADC="); uart_numero(xadc_vaux6_codigo);
    uart_texto(" Vxadc="); uart_entero(xadc_voltaje_mV);
    uart_texto("mV  I="); uart_entero(corriente_medida_mA);
    uart_texto("mA  V0="); uart_entero(sensor_offset_mV);
    uart_texto("mV  rango="); uart_numero(sensor_error_rango);
    uart_texto("\n");
}

/* CSV para copiar a PC. No ejecutar durante control de potencia. */
static void exportar_historial(void)
{
    uint32_t k;
    uint32_t inicio;
    uint32_t pos;
    uart_texto("indice,codigo_adc,corriente_mA\n");
    inicio = (historial_indice + SENSOR_BUFFER_MUESTRAS - historial_cantidad)
           % SENSOR_BUFFER_MUESTRAS;
    for (k = 0U; k < historial_cantidad; ++k)
    {
        pos = (inicio + k) % SENSOR_BUFFER_MUESTRAS;
        uart_numero(k);
        uart_caracter(',');
        uart_numero(historial[pos].codigo);
        uart_caracter(',');
        uart_entero((int32_t)historial[pos].corriente_mA);
        uart_texto("\n");
    }
    sensor_exportaciones++;
}

static int ejecutar_calibracion_sensor(void)
{
    uint32_t ultimo_muestreo;
    uint32_t ultimo_reporte;
    uint32_t ahora;
    uint32_t codigo;
#if USAR_BOTON_UART
    uint32_t anterior_boton = 0U;
    uint32_t boton;
#endif

    /* Garantizar que el modulo de potencia permanezca apagado. */
    modo_prueba_pwm = 0U;
    sistema_activo = 0U;
    Xil_Out32(PWM_BASE + PWM_CONTROL, PWM_APAGADO);
    Xil_Out32(PWM_BASE + PWM_DUTY, 0U);
    Xil_Out32(PWM_BASE + PWM_DEADTIME, DEADTIME_50);
    iniciar_timer();

    uart_texto("P4: MODO SENSOR. PWM APAGADO. UART 115200.\n");
    uart_texto("Comandos: z=cero(sin corriente), p=lectura, d=CSV 1024 muestras\n");
    calibrar_cero_xadc();

    ultimo_muestreo = Xil_In32(TIMER_BASE + TIMER_TCR);
    ultimo_reporte = ultimo_muestreo;

    while (1)
    {
        ahora = Xil_In32(TIMER_BASE + TIMER_TCR);
        if (sensor_calibrado &&
            (uint32_t)(ahora - ultimo_muestreo) >= SENSOR_INTERVALO_TICKS)
        {
            ultimo_muestreo = ahora;
            codigo = leer_vaux6_12bit();
            corriente_medida_q15 = leer_corriente_q15(codigo);
            guardar_muestra(codigo, corriente_medida_mA);
            sensor_muestras++;
        }

        if ((uint32_t)(ahora - ultimo_reporte) >= SENSOR_REPORTE_TICKS)
        {
            ultimo_reporte = ahora;
            if (sensor_calibrado) imprimir_medicion();
            else uart_texto("Sensor sin calibrar; apaga carga y envia 'z'.\n");
        }

        if (uart_hay_comando())
        {
            char cmd = uart_leer_comando();
            if (cmd == 'z' || cmd == 'Z')
            {
                /* La fuente de carga DEBE estar apagada al enviar 'z'. */
                calibrar_cero_xadc();
                historial_indice = 0U;
                historial_cantidad = 0U;
            }
            else if (cmd == 'p' || cmd == 'P') imprimir_medicion();
            else if (cmd == 'd' || cmd == 'D') exportar_historial();
        }

#if USAR_BOTON_UART
        boton = Xil_In32(GPIO_BASE + 0x00U) & 1U;
        if (boton && !anterior_boton) exportar_historial();
        anterior_boton = boton;
#endif
    }
    return 0; /* no se alcanza: la supervisión es continua */
}

/* ============================================================
 * PROGRAMA PRINCIPAL
 * ============================================================
 */

#if MODO_PRUEBA_DIGITAL_PR

/*
 * Prueba numerica del PR con escritura/lectura AXI del DUTY.
 * La aplicacion permanece detenida al final para permitir WATCH.
 * Esta prueba NO valida el lazo cerrado ni los coeficientes de diseno.
 */
int main(void)
{
    uint32_t k;
    uint32_t fase;
    int32_t referencia;

    /* Estado inicial: todos los modos de PWM desactivados. */
    sistema_activo = 0U;
    modo_prueba_pwm = 0U;

    /* PRUEBA AXI4-LITE CON PWM DESHABILITADO */

    Xil_Out32(PWM_BASE + PWM_CONTROL, PWM_APAGADO);
    Xil_Out32(PWM_BASE + PWM_DUTY, 0U);
    Xil_Out32(PWM_BASE + PWM_DEADTIME, DEADTIME_50);

    pwm_deadtime_leido =
        Xil_In32(PWM_BASE + PWM_DEADTIME);

    pwm_control_final =
        Xil_In32(PWM_BASE + PWM_CONTROL);

    pwm_duty_axi_ok =
        ((Xil_In32(PWM_BASE + PWM_DUTY) & 0xFFFFU) == 0U)
        ? 1U : 0U;

    if (pwm_deadtime_leido != DEADTIME_50 ||
        (pwm_control_final & 1U) != 0U ||
        pwm_duty_axi_ok != 1U)
    {
        error_codigo = 30U;
        prueba_ok = 0U;
        prueba_finalizada = 1U;

        while (1)
        {
        }
    }

    e1 = 0;
    e2 = 0;
    y1 = 0;
    y2 = 0;

    pr_saturaciones = 0U;
    pr_muestras = 0U;
    pr_ejecutado = 0U;
    prueba_ok = 0U;
    prueba_finalizada = 0U;
    error_codigo = 0U;

    /* Cinco periodos de una referencia de 50 Hz a 10 kHz. */
    for (k = 0U; k < 1000U; ++k)
    {
        fase = k % 200U;
        referencia = (int32_t)(
            ((int64_t)referencia_virtual(fase) * REFERENCIA_PICO_MA)
            / CORRIENTE_BASE_MA
        );

        pr_ref_q15 = referencia;
        corriente_medida_q15 = 0; /* Entrada numerica de prueba. */
        pr_error_q15 = pr_ref_q15 - corriente_medida_q15;

        /* Calcular la salida actual antes de escribir DUTY por AXI. */
        ejecutar_pr(pr_error_q15);

        /* Actualizar DUTY con la salida del PR.
         * PWM permanece deshabilitado. */
        uint32_t duty_pr =
            (uint32_t)(uint16_t)pr_salida_q15;

        Xil_Out32(PWM_BASE + PWM_DUTY, duty_pr);

        uint32_t duty_verificado =
            Xil_In32(PWM_BASE + PWM_DUTY);

        pwm_duty_axi_ok =
            ((duty_verificado & 0xFFFFU) ==
             (duty_pr & 0xFFFFU)) ? 1U : 0U;

        if (pwm_duty_axi_ok == 0U)
        {
            error_codigo = 31U;
            break;
        }

        pr_muestras++;

        if (pr_saturaciones != 0U)
        {
            error_codigo = 1U;
            break;
        }
    }

    if (error_codigo == 0U && pr_muestras == 1000U && pr_ejecutado == 1U)
    {
        prueba_ok = 1U;
    }
    else if (error_codigo == 0U)
    {
        error_codigo = 2U;
    }

    prueba_finalizada = 1U;

    /* Revisar en WATCH: prueba_finalizada, prueba_ok,
     * error_codigo, pr_saturaciones, pr_muestras. */
    while (1)
    {
        /* Espera intencional para inspeccion con el depurador. */
    }

    return 0;
}

#elif MODO_CALIBRACION_XADC
int main(void)
{
    return ejecutar_calibracion_sensor();
}
#else
int main(void)
{
    uint32_t fase = 0U;

    uint32_t ultimo_evento = 0U;
    uint32_t evento_actual;

    uint32_t eventos_nuevos;

    uint32_t ultimo_tiempo_evento;
    uint32_t ahora;

    uint32_t inicio_trabajo;
    uint32_t fin_trabajo;

    uint32_t codigo_raw;

    uint32_t duty_escrito;
    uint32_t duty_leido;

    int estado;

    /* ========================================================
     * PASO 1: ESTADO INICIAL SEGURO
     * ========================================================
     */

    sistema_activo = 0U;

    Xil_Out32(
        PWM_BASE + PWM_CONTROL,
        PWM_APAGADO
    );

    Xil_Out32(
        PWM_BASE + PWM_DUTY,
        0U
    );

    Xil_Out32(
        PWM_BASE + PWM_DEADTIME,
        DEADTIME_50
    );

    /* ========================================================
     * PASO 2: COMPROBAR REGISTROS PWM
     * ========================================================
     */

    pwm_deadtime_leido =
        Xil_In32(PWM_BASE + PWM_DEADTIME);

    if (pwm_deadtime_leido != DEADTIME_50)
    {
        error_codigo = 1U;
        goto finalizar;
    }

    if (
        (Xil_In32(PWM_BASE + PWM_CONTROL) & 1U)
        != 0U
    )
    {
        error_codigo = 2U;
        goto finalizar;
    }

    /* ========================================================
     * PASO 3: INICIAR TEMPORIZADOR
     * ========================================================
     */

    iniciar_timer();

    /* ========================================================
     * PASO 4: CONFIGURAR INTERRUPCIONES
     * ========================================================
     */

    estado = XIntc_Initialize(
        &intc,
        INTC_BASE
    );

    if (estado != XST_SUCCESS)
    {
        error_codigo = 3U;
        goto finalizar;
    }

    estado = XIntc_Connect(
        &intc,
        PWM_IRQ_ID,
        (XInterruptHandler)pwm_callback,
        (void *)0
    );

    if (estado != XST_SUCCESS)
    {
        error_codigo = 4U;
        goto finalizar;
    }

    estado = XIntc_Start(
        &intc,
        XIN_REAL_MODE
    );

    if (estado != XST_SUCCESS)
    {
        error_codigo = 5U;
        goto finalizar;
    }

    XIntc_Enable(
        &intc,
        PWM_IRQ_ID
    );

    Xil_ExceptionInit();

    Xil_ExceptionRegisterHandler(
        XIL_EXCEPTION_ID_INT,
        (Xil_ExceptionHandler)XIntc_InterruptHandler,
        &intc
    );

    Xil_ExceptionEnable();

    irq_pwm_inicializado = 1U;

    /* ========================================================
     * PASO 5: HABILITAR PWM CONTINUAMENTE
     * ========================================================
     */

    ultimo_evento = irq_pwm_contador;

    ultimo_tiempo_evento =
        Xil_In32(TIMER_BASE + TIMER_TCR);

    Xil_Out32(
        PWM_BASE + PWM_CONTROL,
        PWM_ENCENDIDO
    );

    sistema_activo = 1U;

    /* ========================================================
     * PASO 6: CICLO CONTINUO DE CONTROL
     * ========================================================
     */

    while (1)
    {
        /* Consultar contador de interrupciones */

        evento_actual = irq_pwm_contador;

        /* Esperar siguiente evento */

        if (evento_actual == ultimo_evento)
        {
            ahora =
                Xil_In32(TIMER_BASE + TIMER_TCR);

            /*
             * Proteccion:
             * Si no llega una interrupcion durante
             * dos periodos, apagar el PWM.
             */

            if (
                (uint32_t)(ahora - ultimo_tiempo_evento)
                > WATCHDOG_IRQ_TICKS
            )
            {
                error_codigo = 10U;
                break;
            }

            continue;
        }

        eventos_nuevos =
            (uint32_t)(evento_actual - ultimo_evento);

        ultimo_evento = evento_actual;

        ultimo_tiempo_evento =
            Xil_In32(TIMER_BASE + TIMER_TCR);

        /* ====================================================
         * PASO 7: DETECTAR MUESTRAS PERDIDAS
         * ====================================================
         */

        if (eventos_nuevos > 1U)
        {
            pr_muestras_perdidas +=
                eventos_nuevos - 1U;

            error_codigo = 6U;
            break;
        }

        /* ====================================================
         * PASO 8: MEDIR TIEMPO DE PROCESAMIENTO
         * ====================================================
         */

        inicio_trabajo =
            Xil_In32(TIMER_BASE + TIMER_TCR);

        /* ====================================================
         * PASO 9: LEER XADC VAUX6
         * ====================================================
         */

        xadc_estado_leido =
            Xil_In32(
                XADC_BASE + XSM_SR_OFFSET
            );

        codigo_raw =
            Xil_In32(
                XADC_BASE + XSM_AUX06_OFFSET
            );

        xadc_vaux6_codigo =
            (codigo_raw >> 4) & 0x0FFFU;

        #if USAR_CORRIENTE_CALIBRADA

            corriente_medida_q15 =
                leer_corriente_q15(xadc_vaux6_codigo);

        #else

            /* Prueba PWM sin realimentacion fisica. */
            corriente_medida_q15 = 0;

        #endif

        /* ====================================================
         * PASO 10: REFERENCIA Y ERROR
         * ====================================================
         */
         pr_ref_q15 =
            (int32_t)(
                ((int64_t)referencia_virtual(fase) *
                 REFERENCIA_PICO_MA) /
                 CORRIENTE_BASE_MA
            );

        pr_error_q15 =
            pr_ref_q15 - corriente_medida_q15;
            
            
        /* Verificar rango de entrada del PR */
        if (pr_error_q15 > 32767 ||
            pr_error_q15 < -32768)
        {
            error_codigo = 11U;
        break;
        }


        /* ====================================================
         * PASO 11: EJECUTAR CONTROLADOR PR
         * ====================================================
         */

        ejecutar_pr(pr_error_q15);

        pr_ejecutado = 1U;

        /* ====================================================
         * PASO 12: ACTUALIZAR PWM POR AXI4-LITE
         * ====================================================
         */

        duty_escrito =
            (uint32_t)(uint16_t)pr_salida_q15;

        Xil_Out32(
            PWM_BASE + PWM_DUTY,
            duty_escrito
        );

        duty_leido =
            Xil_In32(
                PWM_BASE + PWM_DUTY
            );

        pwm_duty_axi_ok =
            (
                (duty_leido & 0xFFFFU)
                == (duty_escrito & 0xFFFFU)
            ) ? 1U : 0U;

        if (pwm_duty_axi_ok == 0U)
        {
            error_codigo = 7U;
            break;
        }

        /* ====================================================
         * PASO 13: PROTECCION POR SATURACION
         * ====================================================
         */

        if (pr_saturaciones != 0U)
        {
            error_codigo = 8U;
            break;
        }

        /* ====================================================
         * PASO 14: COMPROBAR TIEMPO DE EJECUCION
         * ====================================================
         */

        fin_trabajo =
            Xil_In32(
                TIMER_BASE + TIMER_TCR
            );

        trabajo_ticks =
            (uint32_t)(fin_trabajo - inicio_trabajo);

        if (trabajo_ticks > trabajo_ticks_max)
        {
            trabajo_ticks_max = trabajo_ticks;
        }

        if (trabajo_ticks >= LIMITE_TRABAJO)
        {
            pr_atrasos++;
            error_codigo = 9U;
            break;
        }

        /* ====================================================
         * PASO 15: ACTUALIZAR FASE Y CONTADORES
         * ====================================================
         */

        pr_muestras++;

        fase =
            (fase + eventos_nuevos) % 200U;

        /*
         * Indicador de funcionamiento correcto.
         * Se activa despues de 900 muestras validas.
         */

        if (
            pr_muestras >= 900U &&
            pr_muestras_perdidas == 0U &&
            pr_atrasos == 0U &&
            pr_saturaciones == 0U &&
            pwm_duty_axi_ok == 1U &&
            error_codigo == 0U
        )
        {
            prueba_ok = 1U;
        }
    }

    /* ========================================================
     * PASO 16: APAGADO SEGURO POR ERROR
     * ========================================================
     */

finalizar:

    sistema_activo = 0U;

    Xil_Out32(
        PWM_BASE + PWM_CONTROL,
        PWM_APAGADO
    );

    Xil_Out32(
        PWM_BASE + PWM_DUTY,
        0U
    );

    /* Deshabilitar interrupciones */

    if (irq_pwm_inicializado)
    {
        XIntc_Disable(
            &intc,
            PWM_IRQ_ID
        );

        Xil_ExceptionDisable();
    }

    /* ========================================================
     * PASO 17: VERIFICAR APAGADO
     * ========================================================
     */

    pwm_control_final =
        Xil_In32(
            PWM_BASE + PWM_CONTROL
        );

    pwm_estado_final =
        Xil_In32(
            PWM_BASE + PWM_STATUS
        );

    pwm_apagado_final =
        (
            ((pwm_control_final & 1U) == 0U) &&
            ((pwm_estado_final & 1U) == 0U)
        ) ? 1U : 0U;

    /* ========================================================
     * PASO 18: CONSERVAR RESULTADOS
     * ========================================================
     */

    if (error_codigo != 0U)
    {
        prueba_ok = 0U;
    }

    prueba_finalizada = 1U;

    while (1)
    {
        /* Mantener resultados disponibles para WATCH */
    }

    return 0;
}

#endif /* MODO_PRUEBA_DIGITAL_PR / MODO_CALIBRACION_XADC */
