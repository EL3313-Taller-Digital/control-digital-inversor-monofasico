
# Simulaciones del Proyecto 4

Esta carpeta contiene las pruebas de simulación utilizadas para verificar el funcionamiento de los módulos digitales del sistema de control de corriente para un inversor monofásico.

## Herramientas utilizadas

- Vivado 2026.1
- SystemVerilog
- Vivado Simulator (XSim)

## Simulación del PWM unipolar

Archivo: `Simulaciones/tb_pwm_unipolar.sv`

Este banco de pruebas permite verificar el funcionamiento del módulo PWM unipolar, incluyendo:

- Generación de señales PWM.
- Funcionamiento de las salidas complementarias.
- Comportamiento ante diferentes configuraciones de frecuencia.
- Activación y desactivación del módulo.
- Funcionamiento de la lógica de tiempo muerto.

El módulo RTL correspondiente se encuentra en `Vivado/Proyecto4_Control_Digital/rtl/pwm_unipolar.sv`.

## Simulación del periférico PWM AXI4-Lite

Archivo: `Vivado/pwm_axi_1_0/sim/tb_pwm_axi.sv`

Este banco de pruebas permite verificar la interfaz de comunicación AXI4-Lite y su interacción con el generador PWM.

Se consideran las siguientes verificaciones:

- Escritura y lectura de registros AXI4-Lite.
- Configuración de los parámetros del PWM.
- Habilitación y deshabilitación de las salidas.
- Generación de la señal de sincronización.
- Configuración del tiempo muerto.

El tiempo muerto establecido es de 500 ns, equivalente a 50 ciclos del reloj de 100 MHz.

## Ejecución de las simulaciones

1. Abrir el proyecto correspondiente en Vivado 2026.1.
2. Agregar el testbench que se desea ejecutar.
3. Configurar el testbench como Simulation Top.
4. Ejecutar Run Behavioral Simulation.
5. Revisar las formas de onda y los mensajes de la consola.
6. Comparar los resultados con el comportamiento esperado.

## Alcance de las pruebas

Las simulaciones permiten comprobar el comportamiento digital del PWM y de su interfaz AXI4-Lite.

Estas pruebas no sustituyen las mediciones físicas del inversor ni la validación del sistema de control con realimentación de corriente.
