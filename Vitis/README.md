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
