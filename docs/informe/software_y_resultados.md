# Implementación del software y resultados del control digital

## 1. Introducción

El software del Proyecto 4 se desarrolló en lenguaje C utilizando Vitis 2026.1 y el procesador MicroBlaze V implementado sobre la FPGA Basys 3.

Su función es realizar el procesamiento digital necesario para controlar la corriente de un inversor monofásico mediante un regulador proporcional-resonante (PR) discreto.

La aplicación se comunica con los periféricos del sistema mediante interfaces AXI, permitiendo adquirir señales analógicas, ejecutar el controlador y actualizar los registros del generador PWM.

Durante el desarrollo se implementaron diferentes modos de prueba para verificar los componentes de manera independiente antes de realizar la integración completa.

## 2. Arquitectura del software

La aplicación principal se encuentra en `Vitis/control_digital_P4/src/main.c`.

El programa contempla las siguientes funciones:

- Inicialización del procesador MicroBlaze V y los periféricos.
- Configuración del sistema de interrupciones.
- Adquisición de corriente mediante XADC.
- Generación de la referencia sinusoidal.
- Cálculo del error de corriente.
- Ejecución del regulador PR discreto.
- Actualización del registro DUTY del PWM mediante AXI4-Lite.
- Comunicación UART y almacenamiento de muestras.
- Supervisión de condiciones de funcionamiento.

La arquitectura permite realizar pruebas numéricas del controlador sin habilitar las salidas PWM, lo que facilita la depuración del software antes de utilizar la etapa de potencia.

## 3. Regulador proporcional-resonante discreto

El controlador PR se utiliza para regular señales sinusoidales, combinando una acción proporcional con una componente resonante centrada en la frecuencia fundamental.

La expresión general del regulador ideal en tiempo continuo es:

\[
G_{PR}(s)=K_p+\frac{K_r s}{s^2+\omega_0^2}
\]

Donde:

- \(K_p\): ganancia proporcional.
- \(K_r\): ganancia resonante.
- \(\omega_0\): frecuencia angular de resonancia.

Para una frecuencia fundamental de 50 Hz:

\[
\omega_0=2\pi(50)=100\pi\ \text{rad/s}
\]

El regulador se implementó en forma discreta utilizando operaciones en punto fijo, con variables de estado que conservan información de las muestras anteriores.

La frecuencia nominal de muestreo establecida es de 10 kHz, correspondiente a un período de control de:

\[
T_s=\frac{1}{10000}=100\ \mu s
\]

Los coeficientes empleados durante las pruebas digitales son provisionales y todavía requieren validación con el modelo y las condiciones reales de la planta.

## 4. Adquisición y procesamiento de corriente

La adquisición de corriente se realiza mediante el módulo XADC de la FPGA, utilizando el canal auxiliar VAUX6.

El sensor seleccionado es el ACS712 de 5 A, cuya señal analógica requiere acondicionamiento para cumplir con el rango permitido por las entradas del XADC.

El software contempla la lectura del convertidor, la corrección del nivel correspondiente a corriente cero y el escalado de la muestra adquirida.

La corriente medida se utiliza para calcular el error respecto a la referencia:

\[
e[k]=i_{\mathrm{ref}}[k]-i_{\mathrm{med}}[k]
\]

Este error constituye la entrada del regulador PR.

Durante las pruebas numéricas independientes se utilizó una corriente medida de prueba igual a cero. Por lo tanto, dichas pruebas no representan una adquisición física desde el ACS712.

La calibración definitiva y la validación de la cadena de medición completa permanecen pendientes.

## 5. Comunicación con el periférico PWM

El sistema utiliza un periférico PWM unipolar personalizado, accesible desde el procesador mediante AXI4-Lite.

El software permite escribir valores en sus registros de configuración, incluyendo el registro DUTY, utilizado para representar la acción calculada por el controlador.

El tiempo muerto requerido es de 500 ns. Considerando un reloj de 100 MHz:

\[
N_{\mathrm{deadtime}}=
500\ \mathrm{ns}\times100\ \mathrm{MHz}=50
\]

Por lo tanto, el registro correspondiente se configuró con un valor de 50 ciclos.

Durante las pruebas de integración digital se mantuvo el PWM deshabilitado, mientras se escribían y verificaban los valores calculados por el regulador.

Esto permitió comprobar la comunicación entre MicroBlaze V y el periférico, sin activar las señales de conmutación de la etapa de potencia.

## 6. Comunicación UART y almacenamiento de muestras

La aplicación contempla una interfaz UART configurada a 115200 baudios para transmitir información hacia una computadora.

Esta comunicación se utiliza para diagnóstico, visualización de resultados y transferencia de datos adquiridos durante el funcionamiento.

El software incluye mecanismos de almacenamiento de muestras y funciones destinadas a su exportación en formato CSV.

El objetivo es permitir el análisis posterior de las señales de corriente y del comportamiento del controlador.

La activación de la exportación mediante un botón físico de la FPGA y su funcionamiento durante la ejecución periódica del control todavía requieren verificación funcional.

## 7. Pruebas realizadas en Vitis

### 7.1 Prueba numérica independiente del PR

Se desarrolló una aplicación denominada `prueba_PR` para evaluar el funcionamiento numérico del controlador sin utilizar periféricos externos.

La prueba permitió ejecutar el algoritmo con una referencia generada por software y observar sus variables internas mediante el depurador de Vitis.

Se comprobó que el controlador podía procesar las muestras previstas sin registrar saturaciones en las condiciones ensayadas.

### 7.2 Prueba de integración PR y AXI4-Lite

Posteriormente se utilizó la aplicación `control_digital_P4` para integrar el cálculo del regulador PR con la escritura dinámica del registro DUTY.

La prueba se realizó directamente sobre MicroBlaze V, utilizando una secuencia de 1000 iteraciones.

En cada iteración se ejecutó el controlador y se actualizó el registro DUTY mediante AXI4-Lite. Se comprobó la lectura de retorno de los valores escritos.

Los resultados obtenidos mediante WATCH fueron:

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

Los resultados indican que la prueba terminó satisfactoriamente, que se ejecutaron las 1000 muestras y que no se registraron saturaciones del regulador.

Además, se comprobó la escritura y lectura del registro DUTY y la configuración del tiempo muerto de 50 ciclos.

El valor leído del registro de control presentó el bit de habilitación desactivado durante la comprobación.

### 7.3 Alcance de los resultados

La prueba permitió verificar la integración digital entre el algoritmo PR y los registros del periférico PWM.

Sin embargo, no constituye una validación del control de corriente en lazo cerrado.

Las 1000 iteraciones se ejecutaron como una prueba numérica y no demuestran por sí mismas que la aplicación complete todo su procesamiento dentro de los 100 µs establecidos para el control periódico.

Tampoco se comprobó en esta prueba la forma de onda física del PWM ni la respuesta de la etapa de potencia.

## 8. Verificaciones pendientes

Para completar la validación del sistema digital se requiere:

1. Verificar la adquisición XADC y el procesamiento de la corriente medida dentro del ciclo de control.
2. Comprobar la ejecución periódica mediante AXI Timer e interrupciones a la frecuencia de muestreo establecida.
3. Medir el tiempo de procesamiento del regulador y la actualización del PWM.
4. Validar la exportación UART mediante la interacción con el botón físico.
5. Verificar el funcionamiento conjunto de las protecciones y las condiciones de deshabilitación.
6. Realizar las pruebas del lazo cerrado con el sensor y la etapa de potencia.

## 9. Conclusiones

Se desarrolló una aplicación en C para MicroBlaze V que incorpora el regulador PR discreto y la comunicación con los periféricos del sistema.

Las pruebas numéricas permitieron verificar la ejecución del algoritmo, mientras que las pruebas de integración demostraron la actualización dinámica del registro DUTY mediante AXI4-Lite.

También se comprobó la configuración del tiempo muerto de 500 ns y el funcionamiento del procesador durante la secuencia de prueba.

Estos resultados representan un avance importante en la integración digital del proyecto. No obstante, todavía se requiere verificar la temporización del control, la adquisición real de corriente y el funcionamiento integral del sistema antes de considerar validado el control del inversor monofásico.
