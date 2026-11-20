
# =====================================================
# PROYECTO 4 - CONTROL DIGITAL DE INVERSOR MONOFASICO
# FPGA: Basys 3 (Artix-7)
# PWM unipolar - MicroBlaze V
# =====================================================

# -----------------------------------------------------
# PWM - CONECTOR PMOD JA
# -----------------------------------------------------

# Rama A - Interruptor superior (JA1)
set_property PACKAGE_PIN J1 [get_ports {pwm_a_high_0}]
set_property IOSTANDARD LVCMOS33 [get_ports {pwm_a_high_0}]

# Rama A - Interruptor inferior (JA2)
set_property PACKAGE_PIN L2 [get_ports {pwm_a_low_0}]
set_property IOSTANDARD LVCMOS33 [get_ports {pwm_a_low_0}]

# Rama B - Interruptor superior (JA3)
set_property PACKAGE_PIN J2 [get_ports {pwm_b_high_0}]
set_property IOSTANDARD LVCMOS33 [get_ports {pwm_b_high_0}]

# Rama B - Interruptor inferior (JA4)
set_property PACKAGE_PIN G2 [get_ports {pwm_b_low_0}]
set_property IOSTANDARD LVCMOS33 [get_ports {pwm_b_low_0}]


# -----------------------------------------------------
# XADC - CANAL VAUX6
# -----------------------------------------------------

# Entrada positiva VAUXP6
set_property PACKAGE_PIN J3 [get_ports {Vaux6_0_v_p}]
set_property IOSTANDARD LVCMOS33 [get_ports {Vaux6_0_v_p}]

# Entrada negativa VAUXN6
set_property PACKAGE_PIN K3 [get_ports {Vaux6_0_v_n}]
set_property IOSTANDARD LVCMOS33 [get_ports {Vaux6_0_v_n}]


# -----------------------------------------------------
# RELOJ PRINCIPAL - 100 MHz
# -----------------------------------------------------

set_property PACKAGE_PIN W5 [get_ports {sys_clock}]
set_property IOSTANDARD LVCMOS33 [get_ports {sys_clock}]




# -----------------------------------------------------
# RESET GENERAL - BOTON CENTRAL BTNC
# -----------------------------------------------------

set_property PACKAGE_PIN U18 [get_ports {reset}]
set_property IOSTANDARD LVCMOS33 [get_ports {reset}]


# -----------------------------------------------------
# CONFIGURACION ELECTRICA DE LA BASYS 3
# -----------------------------------------------------

set_property CONFIG_VOLTAGE 3.3 [current_design]
set_property CFGBVS VCCO [current_design]

# =====================================================
# BOTON DE INICIO - BASYS 3
# BTNU - Pin T18
# =====================================================

set_property PACKAGE_PIN T18 [get_ports {button_start_tri_i[0]}]
set_property IOSTANDARD LVCMOS33 [get_ports {button_start_tri_i[0]}]

# =====================================================
# FIN DE LAS RESTRICCIONES
# =====================================================
