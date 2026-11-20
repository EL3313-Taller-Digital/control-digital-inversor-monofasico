# Control Digital de un Inversor Monofásico

## Proyecto 4 — Taller de Diseño Digital

**Instituto Tecnológico de Costa Rica**  
Escuela de Ingeniería Electrónica  
EL3313 — Taller de Diseño Digital  
II Semestre 2026

## Descripción del proyecto

En este proyecto se desarrolla un sistema de control digital de corriente para un inversor monofásico utilizando la FPGA Basys 3. El procesamiento se realiza mediante un MicroBlaze V, encargado de ejecutar el programa de control y comunicarse con los diferentes periféricos del sistema.

La idea principal es utilizar un regulador proporcional-resonante (PR) discreto para controlar la corriente del inversor. Para ello, el sistema contempla la lectura de corriente mediante XADC, el procesamiento de las muestras y la actualización de un generador PWM unipolar.

El proyecto se desarrolla en conjunto con el curso EL4201 — Procesamiento Electrónico de Potencia. Sin embargo, este repositorio se concentra en la parte digital, incluyendo el diseño en Vivado, la programación en C, las simulaciones y las pruebas de integración.

## Objetivos

- Implementar un sistema basado en MicroBlaze V utilizando la Basys 3.
- Desarrollar un generador PWM unipolar con tiempo muerto configurable.
- Implementar un regulador PR discreto para el control de corriente.
- Utilizar el XADC para adquirir la señal del sensor de corriente.
- Establecer la comunicación entre el procesador y los periféricos mediante AXI4-Lite.
- Implementar comunicación UART para transmitir información hacia una computadora.
- Verificar el funcionamiento de los diferentes módulos mediante simulaciones y pruebas digitales.

## Herramientas y características del sistema

| Elemento | Descripción |
|---|---|
| FPGA | Basys 3 |
| Procesador | MicroBlaze V (RISC-V) |
| Reloj principal | 100 MHz |
| Herramientas | Vivado y Vitis 2026.1 |
| Diseño de hardware | SystemVerilog |
| Programación | C |
| Comunicación interna | AXI4-Lite |
| Adquisición de corriente | XADC, canal VAUX6 |
| Sensor de corriente | ACS712 de 20 A |
| Comunicación serie | UART, 115200 baudios |
| Frecuencia de referencia | 50 Hz |
| Frecuencia de muestreo prevista | 10 kHz |
| Tiempo muerto del PWM | 500 ns |

## Funcionamiento general

El MicroBlaze V es el encargado de coordinar el funcionamiento del sistema. Desde el programa en C se configuran los periféricos, se procesan los datos de corriente y se calcula la señal de control que posteriormente se envía al PWM.

El diseño incluye los siguientes componentes:

- **MicroBlaze V:** ejecuta el programa principal y el regulador PR.
- **PWM unipolar:** genera las señales de conmutación. Incluye tiempo muerto configurable y registros accesibles mediante AXI4-Lite.
- **XADC:** permite adquirir la señal analógica proveniente del sensor de corriente.
- **AXI Timer:** se utiliza como base temporal para ejecutar el control de manera periódica.
- **UART:** permite enviar información de diagnóstico y muestras hacia una computadora.
- **GPIO e interrupciones:** se utilizan para gestionar señales de control y eventos del sistema.

La intención es que estos componentes trabajen en conjunto para adquirir la corriente, compararla con una referencia y actualizar el PWM a partir de la respuesta del regulador.

Aunque los módulos principales ya se encuentran desarrollados, todavía quedan algunas pruebas para comprobar su funcionamiento conjunto.


## Desarrollo en Vivado

El sistema de hardware se implementó utilizando Vivado 2026.1. Se construyó un diseño de bloques que integra MicroBlaze V con los periféricos necesarios para el funcionamiento del control digital.

Uno de los componentes principales es el generador PWM unipolar desarrollado en SystemVerilog. Este módulo cuenta con una interfaz AXI4-Lite, lo que permite modificar sus parámetros desde el programa en C.

Se estableció un tiempo muerto de 500 ns para evitar que las señales complementarias de una misma rama se activen simultáneamente. Como el sistema trabaja con un reloj de 100 MHz, este tiempo equivale a 50 ciclos.

Durante el desarrollo se realizaron simulaciones del PWM, pruebas de comunicación AXI4-Lite y verificaciones de síntesis e implementación.

También se logró generar el bitstream del diseño para la Basys 3.

## Desarrollo en Vitis

La programación se realizó en C utilizando Vitis 2026.1. Para organizar las pruebas y el desarrollo del controlador se trabajó principalmente con tres componentes.

### Aplicación control_digital_P4

Es la aplicación principal del proyecto. Incluye las funciones necesarias para trabajar con el XADC, procesar la corriente medida, ejecutar el regulador PR y actualizar los registros del PWM mediante AXI4-Lite.

También contempla el manejo de interrupciones, comunicación UART y almacenamiento de muestras.

El programa dispone de diferentes modos de funcionamiento que permiten realizar pruebas sin necesidad de activar la etapa de potencia.

### Aplicación prueba_PR

Esta aplicación se utilizó para comprobar el funcionamiento numérico del regulador PR de manera independiente.

La prueba permite generar una referencia desde el software, ejecutar el controlador y revisar sus resultados sin depender del sensor de corriente ni de las señales PWM.

### Plataforma platforma_P4

Contiene la configuración de MicroBlaze V y los archivos principales utilizados para trabajar con el hardware exportado desde Vivado.

Se incluyen el archivo XSA y las configuraciones básicas necesarias para preparar el entorno en Vitis.

## Pruebas realizadas

A lo largo del proyecto se realizaron pruebas individuales de los periféricos y del software para comprobar su funcionamiento antes de integrarlos.

Entre las pruebas realizadas se encuentran la simulación del PWM, las transacciones AXI4-Lite, la ejecución de programas sobre MicroBlaze V y la verificación numérica del regulador PR.

Una de las últimas pruebas consistió en ejecutar 1000 iteraciones del controlador PR y escribir los resultados en el registro DUTY del PWM mediante AXI4-Lite.

Durante esta prueba se mantuvo el PWM deshabilitado para comprobar únicamente el comportamiento digital.

Los resultados obtenidos en el depurador de Vitis fueron:

| Verificación | Resultado |
|---|---|
| Ejecución del regulador PR | Correcta |
| Muestras procesadas | 1000 |
| Saturaciones registradas | 0 |
| Escritura y lectura del registro DUTY | Correcta |
| Tiempo muerto configurado | 50 ciclos |
| Código de error | 0 |
| Resultado general de la prueba | PASS |

Con estos resultados se comprobó que MicroBlaze V puede ejecutar el regulador PR y actualizar los registros del periférico PWM.

Sin embargo, esta prueba no demuestra todavía el funcionamiento del control de corriente en lazo cerrado ni garantiza que el procesamiento completo se ejecute dentro del período de muestreo previsto.

## Cómo utilizar el proyecto

Para trabajar con los archivos se recomienda utilizar Vivado y Vitis 2026.1, que fueron las versiones empleadas durante el desarrollo.

El procedimiento general es el siguiente:

1. Descargar el repositorio en la computadora.
2. Abrir los archivos correspondientes al diseño de hardware en Vivado.
3. Revisar las fuentes, restricciones y conexiones del diseño.
4. Generar o utilizar el archivo de hardware exportado desde Vivado.
5. Abrir Vitis y configurar la plataforma MicroBlaze V con el archivo XSA.
6. Incorporar las aplicaciones en C y sus archivos de configuración.
7. Compilar los componentes y generar los archivos necesarios para la ejecución.
8. Programar la Basys 3 y realizar las pruebas correspondientes.

Es posible que algunas rutas deban actualizarse al utilizar los archivos en otra computadora, especialmente las relacionadas con el hardware exportado y la configuración de la plataforma.


## Estado actual

Hasta el momento se ha avanzado en el diseño de los módulos principales, la programación del controlador y las pruebas de comunicación entre MicroBlaze V y los periféricos.

**Entre los trabajos realizados se encuentran:**

- Implementación del sistema base MicroBlaze V.
- Desarrollo del PWM unipolar con AXI4-Lite.
- Configuración del tiempo muerto de 500 ns.
- Integración del XADC en el diseño.
- Desarrollo de la aplicación principal en C.
- Implementación del regulador PR discreto.
- Pruebas numéricas del controlador.
- Comunicación entre el PR y los registros del PWM.
- Pruebas individuales de UART y temporizador.
- Síntesis, implementación y generación del bitstream.

**Las principales actividades pendientes son:**

- Verificar que el procesamiento completo cumpla con el período de control de 100 µs.
- Comprobar el funcionamiento integrado de XADC, regulador PR y PWM.
- Completar la calibración del sensor ACS712 de 20 A.
- Verificar la exportación de muestras mediante un botón de la Basys 3.
- Comprobar el funcionamiento conjunto de las protecciones.
- Realizar las pruebas del control de corriente utilizando la etapa de potencia.

## Documentación del proyecto

Además de los archivos de programación y diseño, el repositorio contiene documentación relacionada con la arquitectura digital y los resultados obtenidos.

Se incluyen diagramas de bloques, explicaciones de los módulos, información sobre las interfaces y descripciones de las pruebas realizadas.

Esta documentación permite conocer cómo se desarrolló el sistema, qué partes han sido verificadas y cuáles necesitan pruebas adicionales.

## Conclusiones

Durante el desarrollo del proyecto se logró implementar una parte importante del sistema de control digital utilizando la Basys 3.

Se desarrolló el generador PWM unipolar, se integraron los periféricos principales y se programó el regulador PR discreto en C sobre MicroBlaze V.

Las pruebas realizadas permitieron comprobar el funcionamiento numérico del controlador y la comunicación con el PWM mediante AXI4-Lite. También se verificó la configuración del tiempo muerto y la ejecución de las aplicaciones sobre la FPGA.

A pesar de estos avances, todavía es necesario completar las pruebas de adquisición de corriente, verificar la temporización del controlador y comprobar el funcionamiento del sistema con la etapa de potencia.

Por lo tanto, el proyecto cuenta con una base digital desarrollada y parcialmente verificada, sobre la cual se puede continuar trabajando para completar la integración del control de corriente.
