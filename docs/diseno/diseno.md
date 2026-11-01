
# Documento de Diseño — Proyecto 4

## Control de corriente en un inversor monofásico mediante un regulador PR discreto sobre MicroBlaze V

**Instituto Tecnológico de Costa Rica**  
**Escuela de Ingeniería Electrónica**  
**EL3313 — Taller de Diseño Digital**  
**II Semestre 2026**
**Equipo:** 06  

### Integrantes
Yerlin Angelitte Gamboa Ortiz 2022184205

Anthony Fabián Chacón Montero 2022117452


---

## 1. Introducción

El proyecto consiste en el diseño e implementación de un sistema embebido en FPGA para controlar la corriente de un inversor monofásico mediante un regulador proporcional-resonante (PR) discreto.

La arquitectura utiliza el procesador MicroBlaze V, periféricos conectados mediante AXI4-Lite, adquisición de corriente mediante XADC, generación de PWM unipolar y comunicación UART.

El sistema digital se desarrolla en coordinación con el curso de Procesamiento Electrónico de Potencia.

## 2. Arquitectura del sistema digital

### 2.1. Diagrama de primer nivel

![Diagrama de primer nivel](imagenes/diagrama_nivel_1.png)

**Figura 1.** Arquitectura general del sistema digital implementado en FPGA.

### 2.2. Descripción del primer nivel

El diagrama representa los módulos principales del sistema digital y sus conexiones.

MicroBlaze V ejecuta el algoritmo de control PR y se comunica con los periféricos mediante la interconexión AXI. La memoria BRAM permite almacenar instrucciones y datos, mientras que la caché mejora el acceso a instrucciones y el MDM facilita la depuración.

El AXI Timer genera las interrupciones periódicas necesarias para ejecutar el controlador.

El módulo PWM personalizado produce cuatro señales de conmutación para los drivers del puente H y genera `pwm_trigger_o` para sincronizar la adquisición de corriente.

El XADC convierte la señal analógica acondicionada del sensor de corriente en datos digitales. Finalmente, UART permite enviar las muestras almacenadas hacia una computadora.

### 2.3. Entradas y salidas

| Señal | Dirección | Función |
|---|---|---|
| clk | Entrada | Reloj de 100 MHz |
| reset | Entrada | Reinicio del sistema |
| btn_export | Entrada | Solicitud de exportación de muestras |
| i_sens | Entrada | Señal analógica del sensor de corriente |
| pwm[3:0] | Salida | Control de los drivers del puente H |
| TX | Salida | Transmisión UART hacia la PC |

La señal `pwm_trigger_o` corresponde a una conexión interna entre el PWM y el sistema de adquisición.
