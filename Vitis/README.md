# Software de control en Vitis

Esta carpeta contiene el software desarrollado en C para el sistema de control digital de corriente de un inversor monofásico, implementado sobre un procesador MicroBlaze V en la FPGA Basys 3.

El programa permite configurar y utilizar los periféricos del sistema, incluyendo la adquisición de corriente mediante XADC, el cálculo del regulador proporcional-resonante (PR) discreto y la actualización del generador PWM unipolar.

## Funciones principales

- Inicialización del procesador y los periféricos.
- Lectura de muestras del XADC.
- Procesamiento y escalado de la corriente medida.
- Cálculo del error entre la referencia y la corriente medida.
- Ejecución del regulador PR discreto.
- Actualización de los registros del PWM mediante AXI4-Lite.
- Comunicación UART para diagnóstico y seguimiento del funcionamiento.

## Entorno de desarrollo

El software se desarrolla utilizando Vitis 2026.1 y una plataforma de hardware exportada desde Vivado.

El sistema utiliza un procesador MicroBlaze V con arquitectura RISC-V.

## Estado de implementación

Se han desarrollado y compilado aplicaciones en C y se han realizado pruebas de comunicación y funcionamiento de periféricos individuales.

La validación del lazo cerrado completo, incluyendo la medición de corriente, el regulador PR y la etapa de potencia, continúa pendiente.

## Aplicación principal de control digital

La aplicación `control_digital_P4` fue desarrollada en C utilizando Vitis 2026.1 y se ejecuta sobre el procesador MicroBlaze V integrado en la FPGA Basys 3.

Su objetivo es implementar el procesamiento digital necesario para regular la corriente de un inversor monofásico mediante un controlador proporcional-resonante (PR) discreto.

El software se encarga de generar la referencia de corriente, procesar las muestras adquiridas, calcular la acción de control y comunicarse con el periférico PWM mediante AXI4-Lite.

### Regulador PR discreto

El regulador PR permite controlar señales sinusoidales mediante una acción proporcional y una componente resonante centrada en la frecuencia fundamental.

Para este proyecto se utiliza una referencia de 50 Hz y una frecuencia nominal de muestreo de 10 kHz, equivalente a un período de control de 100 µs.

La implementación utiliza operaciones en punto fijo para ejecutar los cálculos sobre MicroBlaze V. El controlador mantiene estados internos correspondientes a muestras anteriores, necesarios para implementar su ecuación discreta.

Los coeficientes utilizados durante las pruebas son provisionales y requieren validación con la planta real.

### Modos de funcionamiento

El programa contempla diferentes modos para facilitar las pruebas y la integración:

**Prueba digital del regulador PR:** permite ejecutar el controlador utilizando referencias generadas por software, sin depender de la adquisición física de corriente. Se utiliza para verificar el cálculo del regulador y la comunicación con los registros PWM.

**Calibración y adquisición XADC:** contempla la lectura del canal VAUX6 y el procesamiento de la señal correspondiente al sensor de corriente ACS712.

**Control integrado:** contempla la adquisición de corriente, el cálculo del error, la ejecución del regulador PR y la actualización del PWM. La validación integral de este modo continúa pendiente.

### Comunicación con el PWM

El procesador se comunica con el periférico PWM mediante registros accesibles a través del bus AXI4-Lite.

Durante las pruebas digitales se comprobó la escritura y lectura del registro DUTY. También se verificó la configuración de un tiempo muerto de 50 ciclos del reloj de 100 MHz, equivalente a 500 ns.

Para estas pruebas se mantuvo el PWM deshabilitado, evitando activar las salidas hacia la etapa de potencia.

### Resultados de la prueba digital

Se ejecutó una prueba de 1000 iteraciones del regulador PR sobre MicroBlaze V.

Los resultados observados mediante el depurador de Vitis fueron:

| Variable | Resultado |
|---|---|
| `prueba_ok` | 1 |
| `error_codigo` | 0 |
| `pr_ejecutado` | 1 |
| `pr_muestras` | 1000 |
| `pr_saturaciones` | 0 |
| `pwm_duty_axi_ok` | 1 |
| `pwm_deadtime_leido` | 50 |
| `pwm_control_final` | 0x02 |

Estos resultados permitieron comprobar la ejecución numérica del PR y la actualización dinámica del registro DUTY mediante AXI4-Lite.

La prueba no demuestra todavía la estabilidad del lazo cerrado ni el cumplimiento temporal del período de control.

### Verificaciones pendientes

Se mantienen pendientes la validación completa de la adquisición XADC, la temporización de la ejecución periódica, la interacción física con el usuario y las pruebas del control de corriente utilizando la etapa de potencia.
