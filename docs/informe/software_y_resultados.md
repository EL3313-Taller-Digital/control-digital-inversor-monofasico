# Implementación del software y resultados del control digital

## 1. Introducción

Para el Proyecto 4 se desarrolló un programa en C utilizando Vitis 2026.1 y el procesador MicroBlaze V implementado en la FPGA Basys 3.

El objetivo de este programa es realizar el control de corriente de un inversor monofásico mediante un regulador proporcional-resonante (PR). Para lograrlo, el procesador debe recibir las mediciones de corriente, compararlas con una referencia y calcular el valor que se enviará al generador PWM.

Durante el desarrollo se realizaron varias pruebas para comprobar el funcionamiento de los componentes por separado. Esto permitió identificar errores y verificar partes del sistema antes de intentar la integración completa con la etapa de potencia.

En este documento se explica cómo se organizó el software, las pruebas que se realizaron y los resultados obtenidos.

## 2. Funcionamiento general del programa

El programa principal se encuentra en `Vitis/control_digital_P4/src/main.c`.

Desde este archivo se manejan las principales funciones del sistema, entre ellas:

- Inicializar el procesador y los periféricos.
- Configurar las interrupciones.
- Obtener las mediciones de corriente mediante el XADC.
- Generar una referencia de corriente senoidal.
- Calcular la diferencia entre la corriente deseada y la medida.
- Ejecutar el regulador PR.
- Enviar el resultado al PWM mediante AXI4-Lite.
- Transmitir información por UART para revisar el comportamiento del sistema.

Una de las ventajas de esta organización es que permite realizar pruebas sin activar las salidas PWM. De esta manera se puede comprobar el funcionamiento de los cálculos y de la comunicación entre periféricos sin necesidad de conectar inmediatamente el inversor a la etapa de potencia.

## 3. Regulador proporcional-resonante (PR)

El regulador PR es una de las partes principales del programa, ya que se encarga de calcular la acción de control a partir del error de corriente.

Se utiliza este tipo de regulador porque está pensado para trabajar con señales sinusoidales, como la corriente que se desea controlar en el inversor.

Su expresión general en tiempo continuo es:

**G_PR(s) = Kp + (Kr · s) / (s² + ω₀²)**

Donde:

- **Kp:** ganancia proporcional.
- **Kr:** ganancia resonante.
- **ω₀:** frecuencia angular de la señal que se desea controlar.

Para este proyecto se trabaja con una frecuencia fundamental de 50 Hz, por lo que:

**ω₀ = 2π · 50 = 100π rad/s**

Como el controlador se ejecuta dentro del procesador, fue necesario utilizar una versión discreta, capaz de trabajar con muestras tomadas a intervalos definidos.

La frecuencia de muestreo establecida es de 10 kHz. Esto significa que el programa debe realizar un nuevo cálculo cada 100 µs.

**Tₛ = 1 / 10 000 = 100 µs**

Para realizar los cálculos se utilizaron operaciones en punto fijo, que permiten trabajar con los valores numéricos dentro del procesador.

Durante las pruebas se comprobó que el algoritmo podía ejecutarse y actualizar sus variables correctamente. Sin embargo, los valores de las ganancias todavía deben ajustarse y comprobarse con las condiciones reales del inversor.

## 4. Medición de corriente mediante el XADC

Para medir la corriente se utiliza un **sensor ACS712 de 20 A**, conectado al sistema de adquisición XADC de la FPGA mediante el canal VAUX6.

Este sensor entrega una tensión que cambia según la corriente que circula por él. Como la entrada del XADC tiene un rango de tensión limitado, es necesario acondicionar la señal antes de conectarla a la FPGA.

### Conversión de la señal del sensor

Para convertir la tensión del sensor a un valor de corriente se utiliza la siguiente relación:

**I_med = (V_sensor − V_offset) / S**

Donde:

- **I_med:** corriente que se desea calcular.
- **V_sensor:** tensión de salida del sensor.
- **V_offset:** tensión correspondiente a corriente cero.
- **S:** sensibilidad del sensor.

En la versión de 20 A del ACS712, la sensibilidad nominal habitual es de aproximadamente 100 mV/A, equivalente a 0.1 V/A.

Por lo tanto, utilizando directamente la tensión del sensor y suponiendo que se cumple esa sensibilidad:

**I_med = (V_sensor − V_offset) / 0.1**

Es importante mencionar que esta expresión debe ajustarse al circuito de acondicionamiento que se utiliza antes del XADC. La calibración definitiva debe tomar en cuenta tanto el sensor como los cambios de tensión introducidos por ese circuito.

### Cálculo del error

Una vez obtenida la corriente, el programa la compara con el valor de referencia.

La diferencia se calcula mediante:

**e[k] = i_ref[k] − i_med[k]**

Este resultado es el error que recibe el regulador PR para calcular la siguiente acción de control.

Durante las primeras pruebas del programa no se utilizó una medición física del sensor. En su lugar, se trabajó con una corriente medida de prueba igual a cero para comprobar los cálculos del regulador.

Por esta razón, todavía es necesario verificar el funcionamiento completo de la lectura del ACS712 y su conversión a corriente real.

## 5. Comunicación con el PWM

El programa se comunica con un generador PWM unipolar desarrollado para el proyecto.

Este módulo permite generar las cuatro señales de conmutación que necesita el puente H del inversor. La comunicación con MicroBlaze V se realiza mediante AXI4-Lite, utilizando registros que permiten modificar los parámetros del PWM.

Los registros principales son:

| Dirección relativa | Registro | Función |
|---|---|---|
| 0x00 | CONTROL | Habilitar el PWM y seleccionar su frecuencia |
| 0x04 | DUTY | Enviar la referencia de modulación |
| 0x08 | DEADTIME | Configurar el tiempo muerto |
| 0x0C | STATUS | Consultar el estado del periférico |

### Tiempo muerto

Para evitar que los interruptores complementarios de una misma rama se activen al mismo tiempo, se estableció un tiempo muerto de 500 ns.

Como el reloj de la FPGA funciona a 100 MHz, cada ciclo dura 10 ns.

El número de ciclos necesarios se calcula como:

**N_deadtime = 500 ns / 10 ns = 50 ciclos**

Por lo tanto, el registro del tiempo muerto se configuró con el valor 50.

### Pruebas de comunicación

Durante las pruebas se buscó comprobar que el procesador pudiera enviar valores al registro DUTY del PWM y leerlos nuevamente.

Para realizar estas verificaciones se mantuvieron deshabilitadas las salidas PWM. De esta manera se pudieron probar los registros sin generar señales de conmutación hacia la etapa de potencia.

Los resultados permitieron comprobar la comunicación digital entre MicroBlaze V y el periférico PWM.

## 6. Comunicación UART

El sistema también cuenta con comunicación UART configurada a 115200 baudios.

Esta interfaz se utiliza para enviar información desde la FPGA hacia una computadora, lo que facilita revisar los resultados y detectar posibles problemas durante las pruebas.

El programa contempla el almacenamiento de muestras y su posterior envío en formato CSV, para que puedan analizarse con otras herramientas.

Una de las funciones previstas es iniciar la exportación mediante un botón físico de la FPGA.

Sin embargo, todavía falta comprobar completamente esta función durante la ejecución periódica del controlador.

## 7. Pruebas realizadas en Vitis

Durante el desarrollo se realizaron pruebas para verificar tanto el funcionamiento numérico del regulador como su comunicación con el módulo PWM.

### 7.1 Prueba individual del regulador PR

Primero se utilizó una aplicación llamada `prueba_PR`, desarrollada para revisar el funcionamiento del regulador sin depender de otros periféricos.

En esta prueba se generó una referencia mediante software y se observaron las variables del programa utilizando las herramientas de depuración de Vitis.

Esto permitió comprobar que el regulador ejecutaba los cálculos y actualizaba sus valores internos.

En las condiciones probadas no se registraron saturaciones.

Esta prueba fue útil para revisar el funcionamiento del algoritmo antes de conectarlo con el resto del sistema.

### 7.2 Prueba del regulador PR con AXI4-Lite

Después se trabajó con la aplicación `control_digital_P4`, utilizando directamente el procesador MicroBlaze V.

En esta prueba se ejecutó el regulador durante 1000 iteraciones. En cada una se calculó un nuevo valor de control y se escribió en el registro DUTY del PWM.

También se realizó una lectura de ese registro para comprobar que los valores enviados se estaban almacenando correctamente.

Los resultados obtenidos mediante la herramienta WATCH de Vitis fueron los siguientes:

| Variable | Resultado |
|---|---|
| `prueba_ok` | 1 |
| `error_codigo` | 0 |
| `pr_ejecutado` | 1 |
| `pr_muestras` | 1000 |
| `pr_saturaciones` | 0 |
| `pwm_duty_axi_ok` | 1 |
| `pwm_deadtime_leido` | 50 |
| `pwm_control_final` | 0x00000002 |

A partir de estos resultados se pudo comprobar lo siguiente:

- La prueba terminó correctamente, sin registrar errores.
- El regulador ejecutó las 1000 iteraciones programadas.
- No se presentaron saturaciones durante esta prueba.
- La comunicación con el registro DUTY funcionó correctamente.
- El tiempo muerto configurado correspondía a 50 ciclos.
- El PWM permaneció deshabilitado durante la comprobación.

Estos resultados fueron importantes porque permitieron verificar que el cálculo del regulador y la comunicación AXI4-Lite podían funcionar juntos dentro del procesador.

### 7.3 Limitaciones de las pruebas

Aunque los resultados fueron satisfactorios, es importante aclarar que estas pruebas no representan todavía el funcionamiento completo del inversor.

Las 1000 iteraciones permitieron comprobar los cálculos y las escrituras a los registros, pero no demuestran por sí solas que todo el procesamiento pueda realizarse dentro de los 100 µs disponibles para cada ciclo de control.

Tampoco se utilizó la corriente real medida por el ACS712 durante esa prueba, ni se verificó la respuesta del inversor conectado a la etapa de potencia.

Por esta razón, los resultados deben considerarse una validación parcial de la programación y de la comunicación digital.

## 8. Aspectos pendientes

Aunque se logró avanzar en la implementación del software, todavía quedan algunas verificaciones necesarias para comprobar el funcionamiento completo del sistema.

Las principales son:

1. **Medición de corriente:** comprobar la lectura del XADC y completar la calibración del sensor ACS712 de 20 A.
2. **Control periódico:** verificar que el programa ejecute sus cálculos mediante las interrupciones del AXI Timer con el período establecido.
3. **Tiempo de procesamiento:** medir cuánto tarda el regulador PR en calcular la salida y actualizar el PWM.
4. **Comunicación UART:** comprobar el almacenamiento y la exportación de muestras mediante el botón físico.
5. **Funcionamiento seguro:** revisar las condiciones de habilitación, deshabilitación y protección del sistema.
6. **Integración completa:** realizar pruebas con la realimentación de corriente y la etapa de potencia conectada.

Estas verificaciones permitirán comprobar si el sistema puede mantener el funcionamiento esperado cuando todos los componentes trabajen juntos.

## 9. Conclusiones

Durante el desarrollo del Proyecto 4 se logró implementar una aplicación en C para MicroBlaze V que incorpora el regulador PR discreto y la comunicación con los periféricos digitales del sistema.

Las primeras pruebas permitieron revisar el funcionamiento numérico del controlador, mientras que las pruebas posteriores demostraron que el procesador podía calcular nuevos valores y enviarlos al PWM mediante AXI4-Lite.

También se verificó la configuración del tiempo muerto de 500 ns y se comprobó que las salidas PWM podían mantenerse deshabilitadas durante las pruebas.

Uno de los avances más importantes fue conseguir que el regulador y la interfaz PWM funcionaran juntos, ya que esto permitió comprobar una parte fundamental de la integración digital.

Sin embargo, todavía es necesario realizar pruebas con el sensor ACS712 de 20 A, verificar los tiempos de ejecución y evaluar el comportamiento del sistema con la etapa de potencia.

En general, el trabajo realizado permitió avanzar en la programación y en la comunicación entre los componentes digitales, dejando una base para continuar con las pruebas de integración y completar el control de corriente del inversor monofásico.
