
# Validación del hardware 

## 1. Herramientas utilizadas

- Vivado 2026.1
- FPGA Basys 3 (Artix-7 XC7A35T)
- Reloj principal de 100 MHz
- SystemVerilog

## 2. Verificación del diseño

Se realizaron las siguientes actividades durante el desarrollo:

- Validación del Block Design en Vivado.
- Síntesis del diseño digital.
- Implementación del sistema en la FPGA.
- Generación del bitstream.
- Revisión de las restricciones XDC.
- Verificación de funcionamiento de periféricos mediante Vitis y UART.

## 3. Verificación del PWM

El módulo PWM unipolar incorpora:

- Cuatro señales de salida para el puente H.
- Configuración de la frecuencia de conmutación.
- Habilitación y deshabilitación de las salidas.
- Tiempo muerto de 500 ns, correspondiente a 50 ciclos de reloj de 100 MHz.
- Interfaz AXI4-Lite para configurar el periférico desde MicroBlaze V.

Se dispone de bancos de pruebas para el módulo PWM y para su interfaz AXI4-Lite.

## 4. Validación de periféricos

Durante las pruebas digitales se comprobó el funcionamiento de:

- Procesador MicroBlaze V.
- Comunicación UART.
- GPIO.
- AXI Timer.
- Controlador de interrupciones.

Las pruebas de integración permitieron comprobar la comunicación entre el procesador y periféricos digitales.

## 5. Limitaciones

La validación digital no demuestra por sí sola el funcionamiento del inversor conectado a la etapa de potencia.

Quedan fuera de estas verificaciones:

- Validación del puente H con potencia.
- Medición física de corriente en lazo cerrado.
- Calibración definitiva del sistema de adquisición.
- Comprobación experimental del desempeño del regulador PR con carga real.

## 6. Evidencias

Los archivos RTL, restricciones XDC, Block Design y proyecto Vivado se encuentran en este repositorio.
