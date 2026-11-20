# Simulaciones del sistema digital

Esta carpeta contiene los archivos utilizados para verificar el funcionamiento de los módulos digitales del proyecto antes de su implementación física en la FPGA Basys 3.

Las simulaciones se realizan mediante XSim, utilizando Vivado 2026.1.

## PWM unipolar

Se desarrolló un testbench para verificar el funcionamiento del generador PWM unipolar.

Las pruebas incluyen:

- Generación de señales PWM complementarias.
- Configuración de diferentes períodos de conmutación.
- Verificación de la habilitación y deshabilitación de las salidas.
- Comprobación del funcionamiento del generador de señales.

El tiempo muerto requerido para el sistema es de 500 ns, equivalente a 50 ciclos de reloj de 100 MHz.

## Interfaz AXI4-Lite

Se realizaron pruebas del módulo PWM con interfaz AXI4-Lite para verificar la comunicación mediante registros.

Las pruebas incluyen:

- Escritura y lectura de registros.
- Configuración del generador PWM.
- Verificación del trigger periódico.
- Deshabilitación segura de las salidas.

## Estado de las simulaciones

Las pruebas realizadas del generador PWM y de la interfaz AXI4-Lite finalizaron satisfactoriamente.

La verificación del sistema completo con adquisición XADC, regulador PR y realimentación de corriente se documentará una vez completadas las pruebas correspondientes.
