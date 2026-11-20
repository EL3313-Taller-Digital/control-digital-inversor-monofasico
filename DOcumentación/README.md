# Documentación del sistema

Esta carpeta contiene la documentación técnica del sistema de control digital de corriente para un inversor monofásico, implementado sobre la FPGA Basys 3.

## Arquitectura del sistema

El diseño utiliza un procesador MicroBlaze V para ejecutar el programa de control y comunicarse con los periféricos mediante AXI4-Lite.

El sistema integra los siguientes componentes:

- XADC para adquirir la señal del sensor de corriente.
- Regulador proporcional-resonante (PR) discreto para calcular la acción de control.
- Generador PWM unipolar para producir las señales destinadas al inversor.
- Interfaz AXI4-Lite para configurar y controlar los periféricos.
- UART para comunicación y diagnóstico.

## Diagramas de diseño

La documentación incluye los diagramas de diferentes niveles de abstracción, desde la arquitectura general hasta los componentes internos y las conexiones del sistema.

## Integración con la etapa de potencia

La FPGA proporciona las señales de control PWM que serán utilizadas por la etapa de potencia.

La corriente medida se acondiciona antes de ingresar al XADC, permitiendo implementar la realimentación del controlador PR.

## Estado de la integración

Se han desarrollado y probado componentes individuales del sistema digital.

La validación física del sistema completo junto con el inversor y el sensor de corriente continúa pendiente.
