
# Informe del hardware

## 1. Introducción

En el proyecto se desarrolló una plataforma digital para implementar el control de corriente de un inversor monofásico mediante un regulador proporcional-resonante (PR).

El sistema utiliza una FPGA Basys 3 y un procesador MicroBlaze V. La comunicación con los periféricos se realiza mediante AXI4-Lite.

Este informe se enfoca en el desarrollo del hardware digital, principalmente en el módulo PWM unipolar, su interfaz AXI y la integración con el sistema.

## 2. Implementación del PWM unipolar

El PWM se desarrolló en SystemVerilog y genera cuatro señales de salida para controlar un puente H:

- pwm_a_high
- pwm_a_low
- pwm_b_high
- pwm_b_low

El módulo permite habilitar y deshabilitar las salidas, seleccionar la frecuencia de conmutación y establecer la referencia de modulación.

También incorpora una señal de sincronización denominada pwm_trigger.

### Tiempo muerto

Se implementó un tiempo muerto de 500 ns entre las señales complementarias de cada rama del puente H.

Como el reloj de la FPGA funciona a 100 MHz, cada ciclo dura 10 ns. Por lo tanto:

500 ns / 10 ns = 50 ciclos

El tiempo muerto se configura mediante el parámetro deadtime_cycles, utilizando el valor 50.

Esta lógica busca evitar la conducción simultánea de los interruptores complementarios.

## 3. Interfaz AXI4-Lite

Se desarrolló una IP personalizada que permite controlar el PWM desde MicroBlaze V.

Los registros principales son:

| Dirección relativa | Registro | Descripción |
|---|---|---|
| 0x00 | CONTROL | Habilitación y selección de frecuencia |
| 0x04 | DUTY | Referencia de modulación |
| 0x08 | DEADTIME | Tiempo muerto |
| 0x0C | STATUS | Estado del periférico |

El uso de registros permite modificar los parámetros del PWM desde el programa desarrollado en C, sin modificar directamente el hardware.

## 4. Integración con MicroBlaze V

La IP personalizada se incorporó al diseño de bloques desarrollado en Vivado.

El sistema digital contempla los siguientes componentes:

- MicroBlaze V.
- Interconexión AXI.
- PWM personalizado.
- XADC.
- AXI Timer.
- Controlador de interrupciones.
- UART.

La integración permite que el procesador se comunique con los periféricos y ejecute las tareas de control y supervisión.

La aplicación definitiva requiere integrar la adquisición de corriente, el cálculo del regulador PR y la actualización del PWM de forma sincronizada.

## 5. Simulaciones y verificaciones

Se desarrollaron bancos de pruebas para el PWM unipolar y la interfaz AXI4-Lite.

Las pruebas digitales contemplan:

- Generación de las señales PWM.
- Habilitación y deshabilitación.
- Configuración de frecuencia.
- Funcionamiento de las señales complementarias.
- Tiempo muerto de 500 ns.
- Comunicación mediante registros AXI.
- Generación de la señal de sincronización.

También se realizaron actividades de validación, síntesis, implementación y generación del bitstream en Vivado.

Los archivos y las instrucciones correspondientes se encuentran en las carpetas Vivado y Simulaciones del repositorio.

## 6. Limitaciones

Las pruebas realizadas permiten evaluar el comportamiento digital del sistema, pero no sustituyen la validación física con la etapa de potencia.

La operación completa del regulador PR con corriente realimentada, la calibración del sensor y la respuesta del inversor bajo carga requieren verificaciones adicionales.

## 7. Conclusiones

Se desarrollaron los módulos digitales necesarios para generar señales PWM unipolares y configurarlas mediante una interfaz AXI4-Lite.

La implementación del tiempo muerto de 500 ns proporciona una separación temporal entre las señales complementarias, mientras que la integración con MicroBlaze V permite controlar los parámetros del periférico desde software.

El trabajo realizado establece una base para integrar el regulador PR y completar la validación del sistema de control de corriente.
