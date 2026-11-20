`timescale 1ns / 1ps
module tb_pwm_unipolar_500ns;
    logic clk = 1'b0;
    always #5 clk = ~clk;    // Periodo 10 ns = 100 MHz

    logic rst = 1'b1;
    logic enable = 1'b0;
    logic [1:0] freq_sel = 2'b01;
    logic signed [15:0] duty_ref = 16'sd0;
    logic [15:0] deadtime_cycles = 16'd50;
    logic pwm_a_high, pwm_a_low, pwm_b_high, pwm_b_low, pwm_trigger;

    pwm_unipolar #(.CLK_HZ(100_000_000)) dut (
        .clk(clk), .rst(rst), .enable(enable),
        .freq_sel(freq_sel), .duty_ref(duty_ref),
        .deadtime_cycles(deadtime_cycles),
        .pwm_a_high(pwm_a_high), .pwm_a_low(pwm_a_low),
        .pwm_b_high(pwm_b_high), .pwm_b_low(pwm_b_low),
        .pwm_trigger(pwm_trigger)
    );

    integer ciclo = 0;
    integer apagado_a = -1, apagado_b = -1;
    logic a_high_prev = 1'b0, a_low_prev = 1'b0;
    logic b_high_prev = 1'b0, b_low_prev = 1'b0;
    integer transiciones_a = 0, transiciones_b = 0;

    // Monitor: muestrea despues de las asignaciones no bloqueantes.
    always @(posedge clk) begin
        #1;
        ciclo = ciclo + 1;
        if (pwm_a_high && pwm_a_low)
            $fatal(1, "ERROR: solapamiento en rama A, ciclo %0d", ciclo);
        if (pwm_b_high && pwm_b_low)
            $fatal(1, "ERROR: solapamiento en rama B, ciclo %0d", ciclo);
        if (rst || !enable) begin
            if (pwm_a_high || pwm_a_low || pwm_b_high || pwm_b_low)
                $fatal(1, "ERROR: PWM no esta apagado con reset/disable");
            apagado_a = -1;
            apagado_b = -1;
        end else begin
            if ((a_high_prev || a_low_prev) && !pwm_a_high && !pwm_a_low)
                apagado_a = ciclo;
            if ((b_high_prev || b_low_prev) && !pwm_b_high && !pwm_b_low)
                apagado_b = ciclo;

            if (!(a_high_prev || a_low_prev) && (pwm_a_high || pwm_a_low)
                && apagado_a >= 0) begin
                if (ciclo - apagado_a != 50)
                    $fatal(1,"ERROR: deadtime A = %0d ciclos (se esperan 50)",
                           ciclo - apagado_a);
                transiciones_a = transiciones_a + 1;
                apagado_a = -1;
            end
            if (!(b_high_prev || b_low_prev) && (pwm_b_high || pwm_b_low)
                && apagado_b >= 0) begin
                if (ciclo - apagado_b != 50)
                    $fatal(1,"ERROR: deadtime B = %0d ciclos (se esperan 50)",
                           ciclo - apagado_b);
                transiciones_b = transiciones_b + 1;
                apagado_b = -1;
            end
        end
        a_high_prev = pwm_a_high;
        a_low_prev  = pwm_a_low;
        b_high_prev = pwm_b_high;
        b_low_prev  = pwm_b_low;
    end

    task automatic probar_frecuencia(input logic [1:0] sel,
                                     input integer periodo_esperado);
        integer ultimo_trigger, ticks, disparos;
        begin
            @(negedge clk);
            enable = 1'b0;
            freq_sel = sel;
            duty_ref = 16'sd0;
            repeat (6) @(negedge clk);
            enable = 1'b1;
            ultimo_trigger = 0;
            disparos = 0;
            ticks = 0;
            while (disparos < 3) begin
                @(negedge clk);
                ticks = ticks + 1;
                if (pwm_trigger) begin
                    if (disparos > 0 && ticks - ultimo_trigger != periodo_esperado)
                        $fatal(1, "ERROR: freq_sel=%b periodo=%0d esperado=%0d",
                                  sel, ticks - ultimo_trigger, periodo_esperado);
                    ultimo_trigger = ticks;
                    disparos = disparos + 1;
                end
            end
            $display("PASS: freq_sel=%b, periodo=%0d ciclos (%0d Hz)",
                     sel, periodo_esperado, 100_000_000 / periodo_esperado);
        end
    endtask

    task automatic probar_duty(input logic signed [15:0] valor,
                              input integer comparacion);
        integer cuenta_a, cuenta_b, i;
        begin
            @(negedge clk);
            enable = 1'b0;
            freq_sel = 2'b01; // 10 kHz
            duty_ref = valor;
            repeat (6) @(negedge clk);
            enable = 1'b1;
            do @(negedge clk); while (!pwm_trigger);
            cuenta_a = 0;
            cuenta_b = 0;
            for (i = 0; i < 10000; i = i + 1) begin
                @(negedge clk);
                if (pwm_a_high) cuenta_a = cuenta_a + 1;
                if (pwm_b_high) cuenta_b = cuenta_b + 1;
            end
            if (comparacion > 0 && !(cuenta_a > cuenta_b))
                $fatal(1,"ERROR: duty positivo no eleva duty A sobre B");
            if (comparacion < 0 && !(cuenta_a < cuenta_b))
                $fatal(1,"ERROR: duty negativo no reduce duty A bajo B");
            $display("PASS: duty_ref=%0d, A_high=%0d, B_high=%0d ciclos",
                     valor, cuenta_a, cuenta_b);
        end
    endtask

    initial begin
        repeat (4) @(negedge clk);
        rst = 1'b0;
        probar_frecuencia(2'b00, 20000); // 5 kHz
        probar_frecuencia(2'b01, 10000); // 10 kHz
        probar_frecuencia(2'b10,  5000); // 20 kHz
        probar_frecuencia(2'b11, 10000); // 10 kHz alternativo
        probar_duty(16'sd8192,  1);     // +0.25 Q1.15
        probar_duty(-16'sd8192, -1);    // -0.25 Q1.15

        @(negedge clk);
        enable = 1'b0;
        repeat (3) @(negedge clk);
        if (transiciones_a < 4 || transiciones_b < 4)
            $fatal(1,"ERROR: faltaron transiciones medidas con tiempo muerto");
        $display("PASS: tiempo muerto EXACTO de 50 ciclos (500 ns) en A y B");
        $display("PASS: sin solapamientos y apagado seguro en reset/disable");
        $display("TODAS LAS PRUEBAS DEL PWM UNIPOLAR PASARON");
        $finish;
    end
endmodule
