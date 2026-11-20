
# Proyecto 4 - Diseño digital en Vivado

Esta carpeta contiene el diseño de hardware del sistema de control de corriente para un inversor monofásico, implementado en la FPGA Basys 3 mediante Vivado.

## Descripción del sistema

El diseño integra un procesador MicroBlaze V con los periféricos necesarios para la adquisición, el procesamiento y la generación de señales PWM.

Los principales componentes son:

- MicroBlaze V: procesador principal del sistema.
- PWM AXI4-Lite: periférico personalizado para modulación PWM unipolar.
- XADC: adquisición de señales analógicas.
- AXI Timer: temporización del sistema.
- AXI Interrupt Controller: gestión de interrupciones.
- Interconexión AXI: comunicación entre procesador y periféricos.

## Organización de archivos

- `pwm_axi_1_0/`: archivos de la IP personalizada del PWM.
- `Proyecto4_Control_Digital/Proyecto4_Control_Digital.xpr`: proyecto principal de Vivado.
- `Proyecto4_Control_Digital/block_design/`: diseño de bloques.
- `Proyecto4_Control_Digital/rtl/`: fuentes RTL adicionales.
- `Proyecto4_Control_Digital/constraints/`: restricciones XDC de la Basys 3.
- `Proyecto4_Control_Digital/sistema_control_digital_wrapper.xsa`: plataforma de hardware exportada para Vitis.

## PWM unipolar

El periférico personalizado permite configurar la frecuencia de conmutación, la referencia de modulación y el tiempo muerto mediante registros AXI4-Lite.

El tiempo muerto configurado es de 500 ns, equivalente a 50 ciclos con un reloj de 100 MHz.

El PWM dispone de cuatro salidas para el puente H y una señal de sincronización `pwm_trigger`.

## Reconstrucción del proyecto

1. Descargar el repositorio.
2. Abrir Vivado 2026.1.
3. Abrir `Proyecto4_Control_Digital.xpr`.
4. Configurar el repositorio de IP personalizado utilizando `pwm_axi_1_0/`.
5. Verificar las rutas de las fuentes RTL, el Block Design y las restricciones XDC.
6. Comprobar que Vivado reconoce los componentes del diseño.
7. Ejecutar Validate Design.
8. Realizar síntesis, implementación y generación del bitstream cuando sea necesario.

El archivo XPR conserva referencias a la estructura original del proyecto. Por ello, al descargarlo en otra computadora puede ser necesario corregir rutas o volver a agregar las fuentes antes de reconstruir el diseño.

La reconstrucción desde una descarga limpia debe verificarse antes de considerarla reproducible.

## Verificación

Se realizaron pruebas digitales del PWM, incluyendo frecuencia, modulación, tiempo muerto y deshabilitación.

También se comprobó la comunicación AXI y la recepción de interrupciones del PWM en MicroBlaze V.

## Herramientas

- Vivado 2026.1
- Basys 3, FPGA Artix-7
- SystemVerilog
- AXI4-Lite
- MicroBlaze V
