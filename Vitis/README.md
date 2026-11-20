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

**Calibración y adquisición XADC:** contempla la lectura del canal VAUX6 y el procesamiento de la señal correspondiente al sensor de corriente ACS712 de 20 A.

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

## Adquisición de corriente mediante XADC

El sistema utiliza el módulo XADC de la FPGA Basys 3 para adquirir la señal analógica correspondiente a la corriente medida.

La adquisición se realiza a través del canal auxiliar VAUX6, configurado en Vivado mediante XADC Wizard. El procesador MicroBlaze V utiliza los datos adquiridos para calcular la corriente y posteriormente el error respecto a la referencia.

### Sensor de corriente ACS712

El sensor seleccionado es el **ACS712 de 20 A**, cuya salida analógica depende de la corriente que circula por el conductor.

Debido a los límites de entrada del XADC, la señal del sensor requiere acondicionamiento y reducción de tensión antes de conectarse a la FPGA.

El software contempla una etapa de calibración del nivel correspondiente a corriente cero y la conversión de las muestras digitales a una representación numérica utilizada por el controlador.

La calibración física definitiva del sensor y la validación de la precisión de las mediciones continúan pendientes.

## Comunicación UART

La interfaz UART permite intercambiar información entre el sistema MicroBlaze V y una computadora.

Se utiliza para transmitir mensajes de diagnóstico, resultados de pruebas y datos relacionados con el funcionamiento del controlador.

La comunicación serie está configurada a 115200 baudios.

### Almacenamiento y exportación de muestras

La aplicación contempla un buffer de memoria para almacenar muestras durante la ejecución.

El objetivo es permitir el registro de la corriente medida y su posterior exportación mediante UART para analizar los datos desde una computadora.

El programa incorpora funciones para la transmisión de los datos en formato CSV.

La activación de la exportación desde un botón físico de la Basys 3 y su funcionamiento durante el control periódico todavía requieren validación.

## Estado de validación de periféricos

| Elemento | Estado |
|---|---|
| XADC VAUX6 | Configurado e integrado en el diseño |
| Lectura de corriente en C | Implementada; requiere validación integral |
| ACS712 de 20 A | Acondicionamiento y calibración en desarrollo |
| UART a 115200 baudios | Comunicación probada |
| Buffer de muestras | Implementado en software |
| Exportación CSV | Implementada; pendiente de validación completa |
| Activación mediante botón físico | Pendiente de verificación |

Las pruebas digitales realizadas hasta el momento no sustituyen la calibración del sensor ni la comprobación de las mediciones con la etapa de potencia.
