`timescale 1 ns / 1 ps

// Interfaz AXI4-Lite para PWM unipolar del puente H.
// Registro 0x00 CONTROL : bit 0 enable (RW), bits 2:1 freq_sel (RW),
//                         bit 3 running (RO); restantes 0.
// Registro 0x04 DUTY_REF: bits 15:0 referencia Q1.15 con signo (RW).
// Registro 0x08 DEADTIME: bits 15:0 tiempo muerto en ciclos de clk (RW).
// Registro 0x0C STATUS  : bit 0 running (RO), bit 1 enable (RO).
//
// Acepta los canales de direccion AW y datos W por separado, como
// permite AXI4-Lite. Atiende una transaccion de escritura y una de
// lectura como maximo en cada sentido. Aplica WSTRB a cada byte.

module pwm_axi_slave_lite_v1_0_S00_AXI #(
    parameter integer C_S_AXI_DATA_WIDTH = 32,
    parameter integer C_S_AXI_ADDR_WIDTH = 4
)(
    // Senales de usuario que alimentan pwm_unipolar.sv
    output reg               pwm_enable,
    output reg [1:0]         pwm_freq_sel,
    output reg signed [15:0] pwm_duty_ref,
    output reg [15:0]        pwm_deadtime_cycles,
    input  wire              pwm_running,

    // Interfaz AXI4-Lite, conservando los nombres de Vivado
    input  wire                               S_AXI_ACLK,
    input  wire                               S_AXI_ARESETN,
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]      S_AXI_AWADDR,
    input  wire [2:0]                         S_AXI_AWPROT,
    input  wire                               S_AXI_AWVALID,
    output wire                               S_AXI_AWREADY,
    input  wire [C_S_AXI_DATA_WIDTH-1:0]      S_AXI_WDATA,
    input  wire [(C_S_AXI_DATA_WIDTH/8)-1:0]  S_AXI_WSTRB,
    input  wire                               S_AXI_WVALID,
    output wire                               S_AXI_WREADY,
    output wire [1:0]                         S_AXI_BRESP,
    output wire                               S_AXI_BVALID,
    input  wire                               S_AXI_BREADY,
    input  wire [C_S_AXI_ADDR_WIDTH-1:0]      S_AXI_ARADDR,
    input  wire [2:0]                         S_AXI_ARPROT,
    input  wire                               S_AXI_ARVALID,
    output wire                               S_AXI_ARREADY,
    output wire [C_S_AXI_DATA_WIDTH-1:0]      S_AXI_RDATA,
    output wire [1:0]                         S_AXI_RRESP,
    output wire                               S_AXI_RVALID,
    input  wire                               S_AXI_RREADY
);

    // Direcciones por palabra de 32 bits (addr[3:2]).
    localparam [1:0] REG_CONTROL  = 2'd0; // 0x00
    localparam [1:0] REG_DUTY_REF = 2'd1; // 0x04
    localparam [1:0] REG_DEADTIME = 2'd2; // 0x08
    localparam [1:0] REG_STATUS   = 2'd3; // 0x0C

    // Registro temporal para direccion y datos de escritura.
    // AWVALID y WVALID pueden llegar en distintos ciclos.
    reg aw_pending;
    reg [C_S_AXI_ADDR_WIDTH-1:0] awaddr_saved;
    reg w_pending;
    reg [C_S_AXI_DATA_WIDTH-1:0] wdata_saved;
    reg [(C_S_AXI_DATA_WIDTH/8)-1:0] wstrb_saved;
    reg bvalid_reg;

    // Registro temporal para devolver lecturas AXI.
    reg [C_S_AXI_DATA_WIDTH-1:0] rdata_reg;
    reg rvalid_reg;

    // Canales de escritura. Solo una respuesta pendiente.
    assign S_AXI_AWREADY = S_AXI_ARESETN && !aw_pending && !bvalid_reg;
    assign S_AXI_WREADY  = S_AXI_ARESETN && !w_pending  && !bvalid_reg;
    assign S_AXI_BVALID  = bvalid_reg;
    assign S_AXI_BRESP   = 2'b00; // OKAY

    // Canal de lectura. Conserva RDATA estable hasta RREADY.
    assign S_AXI_ARREADY = S_AXI_ARESETN && !rvalid_reg;
    assign S_AXI_RVALID  = rvalid_reg;
    assign S_AXI_RDATA   = rdata_reg;
    assign S_AXI_RRESP   = 2'b00; // OKAY

    // =============================================================
    // ESCRITURAS AXI4-Lite
    // =============================================================
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            aw_pending          <= 1'b0;
            awaddr_saved        <= {C_S_AXI_ADDR_WIDTH{1'b0}};
            w_pending           <= 1'b0;
            wdata_saved         <= {C_S_AXI_DATA_WIDTH{1'b0}};
            wstrb_saved         <= {(C_S_AXI_DATA_WIDTH/8){1'b0}};
            bvalid_reg          <= 1'b0;

            // Valores seguros al encender el sistema.
            pwm_enable          <= 1'b0;
            pwm_freq_sel        <= 2'b01;  // 10 kHz
            pwm_duty_ref        <= 16'sd0;
            pwm_deadtime_cycles <= 16'd50; // 500 ns @ 100 MHz
        end else begin
            // Captura independiente de la direccion de escritura.
            if (S_AXI_AWVALID && S_AXI_AWREADY) begin
                awaddr_saved <= S_AXI_AWADDR;
                aw_pending   <= 1'b1;
            end

            // Captura independiente de los datos de escritura.
            if (S_AXI_WVALID && S_AXI_WREADY) begin
                wdata_saved <= S_AXI_WDATA;
                wstrb_saved <= S_AXI_WSTRB;
                w_pending   <= 1'b1;
            end

            // Aplicar escritura solo cuando AMBOS canales se aceptaron.
            if (aw_pending && w_pending && !bvalid_reg) begin
                case (awaddr_saved[3:2])
                    REG_CONTROL: begin
                        // Solo byte 0. Bit 3 (running) es RO.
                        if (wstrb_saved[0]) begin
                            pwm_enable   <= wdata_saved[0];
                            pwm_freq_sel <= wdata_saved[2:1];
                        end
                    end

                    REG_DUTY_REF: begin
                        // Bits 15:0 Q1.15. Bits 31:16 reservados.
                        if (wstrb_saved[0])
                            pwm_duty_ref[7:0] <= wdata_saved[7:0];
                        if (wstrb_saved[1])
                            pwm_duty_ref[15:8] <= wdata_saved[15:8];
                    end

                    REG_DEADTIME: begin
                        // El valor se interpreta en ciclos de reloj.
                        if (wstrb_saved[0])
                            pwm_deadtime_cycles[7:0] <= wdata_saved[7:0];
                        if (wstrb_saved[1])
                            pwm_deadtime_cycles[15:8] <= wdata_saved[15:8];
                    end

                    REG_STATUS: begin
                        // Solo lectura; ignorar escrituras.
                    end
                endcase

                aw_pending <= 1'b0;
                w_pending  <= 1'b0;
                bvalid_reg <= 1'b1;
            end
            // Respuesta de escritura aceptada por el maestro.
            else if (bvalid_reg && S_AXI_BREADY) begin
                bvalid_reg <= 1'b0;
            end
        end
    end

    // =============================================================
    // LECTURAS AXI4-Lite
    // =============================================================
    always @(posedge S_AXI_ACLK) begin
        if (!S_AXI_ARESETN) begin
            rdata_reg  <= {C_S_AXI_DATA_WIDTH{1'b0}};
            rvalid_reg <= 1'b0;
        end else begin
            if (S_AXI_ARVALID && S_AXI_ARREADY) begin
                // Capturar el valor en el handshake AR; se mantiene
                // estable mientras RVALID=1 y el maestro no acepta.
                case (S_AXI_ARADDR[3:2])
                    REG_CONTROL:
                        rdata_reg <= {28'd0, pwm_running,
                                      pwm_freq_sel, pwm_enable};

                    REG_DUTY_REF:
                        rdata_reg <= {16'd0, pwm_duty_ref};

                    REG_DEADTIME:
                        rdata_reg <= {16'd0, pwm_deadtime_cycles};

                    REG_STATUS:
                        rdata_reg <= {30'd0, pwm_enable, pwm_running};

                    default:
                        rdata_reg <= {C_S_AXI_DATA_WIDTH{1'b0}};
                endcase

                rvalid_reg <= 1'b1;
            end else if (rvalid_reg && S_AXI_RREADY) begin
                rvalid_reg <= 1'b0;
            end
        end
    end

endmodule
