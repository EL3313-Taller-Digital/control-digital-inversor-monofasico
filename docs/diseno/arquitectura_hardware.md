
# Arquitectura de hardware digital - Proyecto 4

## 1. Descripción general

El Proyecto 4 consiste en el desarrollo de un sistema digital para controlar la corriente de un inversor monofásico mediante un regulador proporcional-resonante (PR).

La implementación utiliza una FPGA Basys 3 con un procesador MicroBlaze V y periféricos conectados mediante AXI4-Lite.

El sistema se desarrolla en Vivado y Vitis 2026.1.

## 2. Componentes principales

### MicroBlaze V

Es el procesador encargado de ejecutar el programa de control desarrollado en C.

Sus funciones previstas incluyen:

- Leer las mediciones de corriente.
- Calcular el regulador PR discreto.
- Actualizar la referencia de modulación PWM.
- Atender las interrupciones del sistema.
- Enviar información de diagnóstico mediante UART.

### XADC

Permite adquirir señales analógicas mediante los canales disponibles en la FPGA.

En el proyecto se utiliza el canal VAUX6 para la entrada de medición.

La conversión de los datos del XADC a valores de corriente requiere considerar la calibración del circuito de acondicionamiento.

### PWM unipolar

Genera las cuatro señales digitales necesarias para controlar las ramas de un puente H:

- `pwm_a_high`
- `pwm_a_low`
- `pwm_b_high`
- `pwm_b_low`

Incluye una lógica de tiempo muerto para evitar la activación simultánea de los interruptores complementarios de una misma rama.

El tiempo muerto es de 500 ns.

Con el reloj de 100 MHz:

- Periodo de reloj: 10 ns.
- Tiempo muerto: 50 ciclos.

### Interfaz AXI4-Lite

Permite que MicroBlaze V configure el periférico PWM mediante registros de memoria.

El mapa de registros de la IP PWM utiliza los siguientes desplazamientos:

| Offset | Registro | Función |
|---|---|---|
| 0x00 | CONTROL | Habilitación y selección de frecuencia |
| 0x04 | DUTY | Referencia de modulación |
| 0x08 | DEADTIME | Tiempo muerto en ciclos |
| 0x0C | STATUS | Estado del periférico |

Los desplazamientos se indican respecto a la dirección base asignada a la IP.

### AXI Timer

Proporciona la temporización necesaria para ejecutar periódicamente las tareas del sistema de control.

### AXI Interrupt Controller

Gestiona las interrupciones de los periféricos y permite que el procesador atienda los eventos correspondientes.

### UART

Se utiliza para transmitir información de diagnóstico y supervisar el funcionamiento del sistema desde una computadora.

## 3. Flujo de funcionamiento previsto

El funcionamiento del control completo sigue esta secuencia:

1. El XADC obtiene una muestra de la señal analógica de corriente.
2. MicroBlaze V recibe la medición y la convierte al formato de trabajo.
3. Se calcula el error entre la corriente de referencia y la corriente medida.
4. El regulador PR discreto calcula la nueva acción de control.
5. MicroBlaze V actualiza el registro de referencia del PWM mediante AXI4-Lite.
6. El periférico PWM genera las señales digitales correspondientes.
7. El sistema repite el proceso en el siguiente periodo de control.

Este flujo describe la arquitectura objetivo. Su operación completa con realimentación física debe validarse mediante las pruebas de integración.

## 4. Plataforma de implementación

- FPGA: Basys 3, Artix-7 XC7A35T.
- Frecuencia del reloj principal: 100 MHz.
- Procesador: MicroBlaze V.
- Lenguaje de hardware: SystemVerilog.
- Lenguaje de software: C.
- Interfaz de periféricos: AXI4-Lite.
- Herramientas: Vivado y Vitis 2026.1.

## 5. Alcance de la validación

Se han realizado pruebas digitales del PWM, de la interfaz AXI4-Lite y de periféricos del sistema.

