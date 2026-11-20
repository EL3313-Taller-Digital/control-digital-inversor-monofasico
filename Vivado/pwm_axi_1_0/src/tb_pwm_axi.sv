`timescale 1ns / 1ps

module tb_pwm_axi;

    // Reloj de 100 MHz y reset AXI activo en bajo.
    reg clk = 1'b0;
    always #5 clk = ~clk;
    reg aresetn = 1'b0;

    // Maestro AXI4-Lite de prueba.
    reg  [3:0]  awaddr = 4'd0;
    reg  [2:0]  awprot = 3'd0;
    reg         awvalid = 1'b0;
    wire        awready;
    reg  [31:0] wdata = 32'd0;
    reg  [3:0]  wstrb = 4'd0;
    reg         wvalid = 1'b0;
    wire        wready;
    wire [1:0]  bresp;
    wire        bvalid;
    reg         bready = 1'b0;
    reg  [3:0]  araddr = 4'd0;
    reg  [2:0]  arprot = 3'd0;
    reg         arvalid = 1'b0;
    wire        arready;
    wire [31:0] rdata;
    wire [1:0]  rresp;
    wire        rvalid;
    reg         rready = 1'b0;

    wire pwm_a_high, pwm_a_low, pwm_b_high, pwm_b_low;
    wire pwm_trigger;

    localparam [3:0] REG_CONTROL  = 4'h0;
    localparam [3:0] REG_DUTY_REF = 4'h4;
    localparam [3:0] REG_DEADTIME = 4'h8;
    localparam [3:0] REG_STATUS   = 4'hC;

    pwm_axi #(
        .C_S00_AXI_DATA_WIDTH(32),
        .C_S00_AXI_ADDR_WIDTH(4)
    ) dut (
        .pwm_a_high(pwm_a_high),
        .pwm_a_low(pwm_a_low),
        .pwm_b_high(pwm_b_high),
        .pwm_b_low(pwm_b_low),
        .pwm_trigger(pwm_trigger),
        .s00_axi_aclk(clk),
        .s00_axi_aresetn(aresetn),
        .s00_axi_awaddr(awaddr),
        .s00_axi_awprot(awprot),
        .s00_axi_awvalid(awvalid),
        .s00_axi_awready(awready),
        .s00_axi_wdata(wdata),
        .s00_axi_wstrb(wstrb),
        .s00_axi_wvalid(wvalid),
        .s00_axi_wready(wready),
        .s00_axi_bresp(bresp),
        .s00_axi_bvalid(bvalid),
        .s00_axi_bready(bready),
        .s00_axi_araddr(araddr),
        .s00_axi_arprot(arprot),
        .s00_axi_arvalid(arvalid),
        .s00_axi_arready(arready),
        .s00_axi_rdata(rdata),
        .s00_axi_rresp(rresp),
        .s00_axi_rvalid(rvalid),
        .s00_axi_rready(rready)
    );

    // Dos tareas independientes: prueban que AW y W pueden llegar
    // simultaneamente o en diferentes ciclos, como permite AXI4-Lite.
    task automatic enviar_aw(input [3:0] direccion);
        begin
            @(negedge clk);
            awaddr  = direccion;
            awvalid = 1'b1;
            do @(posedge clk); while (awready !== 1'b1);
            @(negedge clk);
            awvalid = 1'b0;
        end
    endtask

    task automatic enviar_w(input [31:0] dato, input [3:0] mascara);
        begin
            @(negedge clk);
            wdata  = dato;
            wstrb  = mascara;
            wvalid = 1'b1;
            do @(posedge clk); while (wready !== 1'b1);
            @(negedge clk);
            wvalid = 1'b0;
        end
    endtask

    // orden = 0: simultaneo; 1: direccion primero; 2: datos primero.
    task automatic escribir_axi(
        input [3:0] direccion,
        input [31:0] dato,
        input [3:0] mascara,
        input integer orden
    );
        begin
            case (orden)
                0: fork
                    enviar_aw(direccion);
                    enviar_w(dato, mascara);
                join
                1: begin
                    enviar_aw(direccion);
                    repeat (2) @(negedge clk);
                    enviar_w(dato, mascara);
                end
                2: begin
                    enviar_w(dato, mascara);
                    repeat (2) @(negedge clk);
                    enviar_aw(direccion);
                end
                default: $fatal(1, "Orden de escritura invalido");
            endcase

            // Aplicar contrapresion: BREADY permanece en cero
            // hasta comprobar que existe una respuesta valida.
            wait (bvalid === 1'b1);
            if (bresp !== 2'b00)
                $fatal(1, "BRESP no es OKAY");
            repeat (2) @(negedge clk);
            if (bvalid !== 1'b1)
                $fatal(1, "BVALID no se mantuvo con BREADY=0");
            bready = 1'b1;
            @(posedge clk);
            @(negedge clk);
            bready = 1'b0;
        end
    endtask

    task automatic leer_comprobar_axi(
        input [3:0] direccion,
        input [31:0] esperado
    );
        begin
            @(negedge clk);
            araddr  = direccion;
            arvalid = 1'b1;
            do @(posedge clk); while (arready !== 1'b1);
            @(negedge clk);
            arvalid = 1'b0;

            wait (rvalid === 1'b1);
            if (rresp !== 2'b00)
                $fatal(1, "RRESP no es OKAY");
            if (rdata !== esperado)
                $fatal(1, "Lectura 0x%01h: recibido 0x%08h, esperado 0x%08h",
                       direccion, rdata, esperado);

            // RDATA y RVALID deben conservarse hasta el handshake R.
            repeat (2) @(negedge clk);
            if (rvalid !== 1'b1 || rdata !== esperado)
                $fatal(1, "RDATA/RVALID inestables durante contrapresion");
            rready = 1'b1;
            @(posedge clk);
            @(negedge clk);
            rready = 1'b0;
        end
    endtask

    // Verificacion continua: prohibido activar ambos switches
    // de una misma rama del puente H.
    always @(posedge clk) begin
        #1;
        if (pwm_a_high && pwm_a_low)
            $fatal(1, "Solapamiento detectado en la rama A");
        if (pwm_b_high && pwm_b_low)
            $fatal(1, "Solapamiento detectado en la rama B");
    end

    time t1, t2;

    initial begin
        repeat (5) @(negedge clk);
        aresetn = 1'b1;

        // Reset: CONTROL=2 (10 kHz deshabilitado),
        // DUTY=0, DEADTIME=40, STATUS=0.
        leer_comprobar_axi(REG_CONTROL,  32'h0000_0002);
        leer_comprobar_axi(REG_DUTY_REF, 32'h0000_0000);
        leer_comprobar_axi(REG_DEADTIME, 32'h0000_0028);
        leer_comprobar_axi(REG_STATUS,   32'h0000_0000);
        $display("PASS: Valores iniciales y lecturas AXI");

        // Direccion y datos al mismo tiempo: duty = +0.5 (Q1.15).
        escribir_axi(REG_DUTY_REF, 32'h0000_4000, 4'b1111, 0);
        leer_comprobar_axi(REG_DUTY_REF, 32'h0000_4000);

        // Direccion antes que datos: 40 ciclos de tiempo muerto.
        escribir_axi(REG_DEADTIME, 32'h0000_0028, 4'b1111, 1);
        leer_comprobar_axi(REG_DEADTIME, 32'h0000_0028);

        // Datos antes que direccion: modifica solo el byte bajo.
        escribir_axi(REG_DUTY_REF, 32'h0000_00AA, 4'b0001, 2);
        leer_comprobar_axi(REG_DUTY_REF, 32'h0000_40AA);
        $display("PASS: Escrituras AXI simultaneas y separadas, WSTRB");

        // Registros STATUS y bit running son de solo lectura.
        escribir_axi(REG_STATUS, 32'hFFFF_FFFF, 4'b1111, 0);
        leer_comprobar_axi(REG_STATUS, 32'h0000_0000);

        // CONTROL: bit 0 enable; bits 2:1 freq_sel=01 (10 kHz).
        // El bit 3 se debe ignorar al escribir, y se lee de estado.
        escribir_axi(REG_CONTROL, 32'h0000_000B, 4'b1111, 0);
        leer_comprobar_axi(REG_CONTROL, 32'h0000_000B);
        leer_comprobar_axi(REG_STATUS, 32'h0000_0003);
        $display("PASS: CONTROL y STATUS, campos de solo lectura");

        // Confirmar el periodo PWM de 10 kHz (100 us = 100000 ns).
        repeat (2) @(posedge pwm_trigger);
        t1 = $time;
        @(posedge pwm_trigger);
        t2 = $time;
        if (t2 - t1 != 100_000)
            $fatal(1, "Periodo PWM incorrecto: %0t ns", t2 - t1);
        $display("PASS: PWM a 10 kHz y pwm_trigger periodico");

        // Deshabilitar salidas sin modificar la frecuencia.
        escribir_axi(REG_CONTROL, 32'h0000_0002, 4'b0001, 0);
        repeat (3) @(posedge clk);
        #1;
        if (pwm_a_high || pwm_a_low || pwm_b_high || pwm_b_low)
            $fatal(1, "Las salidas PWM no se apagaron");
        leer_comprobar_axi(REG_CONTROL, 32'h0000_0002);
        leer_comprobar_axi(REG_STATUS,  32'h0000_0000);
        $display("PASS: Deshabilitacion segura desde AXI");

        $display("TODAS LAS PRUEBAS AXI4-LITE PASARON");
        $finish;
    end

    // Timeout de seguridad.
    initial begin
        #1_000_000;
        $fatal(1, "TIMEOUT: la simulacion AXI no termino");
    end

endmodule
