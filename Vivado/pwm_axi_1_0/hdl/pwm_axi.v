
`timescale 1 ns / 1 ps

module pwm_axi #(
    parameter integer C_S00_AXI_DATA_WIDTH = 32,
    parameter integer C_S00_AXI_ADDR_WIDTH = 4
)(
    // =========================================
    // Salidas hacia los drivers del puente H
    // =========================================
    output wire pwm_a_high,
    output wire pwm_a_low,
    output wire pwm_b_high,
    output wire pwm_b_low,

    // Sincronizacion hacia el XADC
    output wire pwm_trigger,

    // =========================================
    // Interfaz AXI4-Lite S00_AXI
    // =========================================
    input wire s00_axi_aclk,
    input wire s00_axi_aresetn,

    input wire [C_S00_AXI_ADDR_WIDTH-1:0] s00_axi_awaddr,
    input wire [2:0] s00_axi_awprot,
    input wire s00_axi_awvalid,
    output wire s00_axi_awready,

    input wire [C_S00_AXI_DATA_WIDTH-1:0] s00_axi_wdata,
    input wire [(C_S00_AXI_DATA_WIDTH/8)-1:0] s00_axi_wstrb,
    input wire s00_axi_wvalid,
    output wire s00_axi_wready,

    output wire [1:0] s00_axi_bresp,
    output wire s00_axi_bvalid,
    input wire s00_axi_bready,

    input wire [C_S00_AXI_ADDR_WIDTH-1:0] s00_axi_araddr,
    input wire [2:0] s00_axi_arprot,
    input wire s00_axi_arvalid,
    output wire s00_axi_arready,

    output wire [C_S00_AXI_DATA_WIDTH-1:0] s00_axi_rdata,
    output wire [1:0] s00_axi_rresp,
    output wire s00_axi_rvalid,
    input wire s00_axi_rready
);

    // =========================================
    // 1. Senales internas de configuracion
    // =========================================

    // Habilitacion del generador
    wire pwm_enable;

    // Seleccion de frecuencia:
    // 00 = 5 kHz
    // 01 = 10 kHz
    // 10 = 20 kHz
    // 11 = reservado (valor por defecto 10 kHz)
    wire [1:0] pwm_freq_sel;

    // Referencia de modulacion Q1.15
    wire signed [15:0] pwm_duty_ref;

    // Tiempo muerto expresado en ciclos de reloj
    wire [15:0] pwm_deadtime_cycles;

    // Estado de habilitacion
    wire pwm_running;

    assign pwm_running =
        s00_axi_aresetn && pwm_enable;

    // =========================================
    // 2. Interfaz AXI4-Lite
    // =========================================

    pwm_axi_slave_lite_v1_0_S00_AXI #(
        .C_S_AXI_DATA_WIDTH(C_S00_AXI_DATA_WIDTH),
        .C_S_AXI_ADDR_WIDTH(C_S00_AXI_ADDR_WIDTH)
    ) pwm_axi_slave_lite_v1_0_S00_AXI_inst (

        .S_AXI_ACLK(s00_axi_aclk),
        .S_AXI_ARESETN(s00_axi_aresetn),

        .S_AXI_AWADDR(s00_axi_awaddr),
        .S_AXI_AWPROT(s00_axi_awprot),
        .S_AXI_AWVALID(s00_axi_awvalid),
        .S_AXI_AWREADY(s00_axi_awready),

        .S_AXI_WDATA(s00_axi_wdata),
        .S_AXI_WSTRB(s00_axi_wstrb),
        .S_AXI_WVALID(s00_axi_wvalid),
        .S_AXI_WREADY(s00_axi_wready),

        .S_AXI_BRESP(s00_axi_bresp),
        .S_AXI_BVALID(s00_axi_bvalid),
        .S_AXI_BREADY(s00_axi_bready),

        .S_AXI_ARADDR(s00_axi_araddr),
        .S_AXI_ARPROT(s00_axi_arprot),
        .S_AXI_ARVALID(s00_axi_arvalid),
        .S_AXI_ARREADY(s00_axi_arready),

        .S_AXI_RDATA(s00_axi_rdata),
        .S_AXI_RRESP(s00_axi_rresp),
        .S_AXI_RVALID(s00_axi_rvalid),
        .S_AXI_RREADY(s00_axi_rready),

        // Nuevas conexiones de los registros
        .pwm_enable(pwm_enable),
        .pwm_freq_sel(pwm_freq_sel),
        .pwm_duty_ref(pwm_duty_ref),
        .pwm_deadtime_cycles(pwm_deadtime_cycles),

        // Informacion de estado hacia AXI
        .pwm_running(pwm_running)
    );

    // =========================================
    // 3. Generador PWM unipolar
    // =========================================

    pwm_unipolar #(
        .CLK_HZ(100_000_000)
    ) pwm_unipolar_inst (

        // Reloj y reset
        .clk(s00_axi_aclk),
        .rst(~s00_axi_aresetn),

        // Parametros recibidos por AXI
        .enable(pwm_enable),
        .freq_sel(pwm_freq_sel),
        .duty_ref(pwm_duty_ref),
        .deadtime_cycles(pwm_deadtime_cycles),

        // Salidas hacia el puente H
        .pwm_a_high(pwm_a_high),
        .pwm_a_low(pwm_a_low),
        .pwm_b_high(pwm_b_high),
        .pwm_b_low(pwm_b_low),

        // Trigger para adquisicion XADC
        .pwm_trigger(pwm_trigger)
    );

endmodule
