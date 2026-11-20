`timescale 1 ns / 1 ps

module pwm_unipolar #(
    parameter int CLK_HZ = 100_000_000
)(
    input  logic clk,
    input  logic rst,
    input  logic enable,
    input  logic [1:0] freq_sel,
    input  logic signed [15:0] duty_ref,
    input  logic [15:0] deadtime_cycles,
    output logic pwm_a_high,
    output logic pwm_a_low,
    output logic pwm_b_high,
    output logic pwm_b_low,
    output logic pwm_trigger
);

    // 00 -> 5 kHz, 01 -> 10 kHz, 10 -> 20 kHz, 11 -> 10 kHz
    // para CLK_HZ=100 MHz.
    localparam int HALF_00 = CLK_HZ / 10_000;
    localparam int HALF_01 = CLK_HZ / 20_000;
    localparam int HALF_10 = CLK_HZ / 40_000;
    localparam int HALF_W  = $clog2(HALF_00 + 1);
    localparam int COUNT_W = $clog2(2 * HALF_00 + 1);

    logic [1:0] freq_active;
    logic signed [15:0] duty_active;
    logic [HALF_W-1:0] half_period_active;
    logic [COUNT_W-1:0] period_counts;
    logic [COUNT_W-1:0] phase_counter;
    logic [HALF_W-1:0] carrier;
    logic [HALF_W-1:0] threshold_a_active;
    logic [HALF_W-1:0] threshold_b_active;
    logic raw_a, raw_b;
    logic [15:0] dead_a, dead_b;

    logic [HALF_W-1:0] half_input;
    logic [HALF_W-1:0] half_p0, half_p1;
    logic [16:0] duty_a_p0, duty_b_p0;
    logic [1:0] freq_p0, freq_p1;
    logic signed [15:0] duty_p0, duty_p1;
    logic [HALF_W+16:0] product_a_p1, product_b_p1;
    logic [16:0] duty_ref_ext;

    // CORRECCION: despues de apagar el transistor, el contador usa N-1
    // para que el transistor complementario encienda exactamente N
    // flancos de reloj mas tarde. N=50 a 100 MHz equivale a 500 ns.
    // La peticion N=0 tambien conserva un flanco sin solapamiento.
    logic [15:0] deadtime_reload;
    assign deadtime_reload = (deadtime_cycles == 16'd0)
                           ? 16'd0 : (deadtime_cycles - 16'd1);

    always_comb begin
        case (freq_sel)
            2'b00: half_input = HALF_00;
            2'b01: half_input = HALF_01;
            2'b10: half_input = HALF_10;
            default: half_input = HALF_01;
        endcase
        duty_ref_ext = {duty_ref[15], duty_ref};
    end

    always_ff @(posedge clk or posedge rst) begin
        if (rst) begin
            duty_a_p0    <= 17'd32768;
            duty_b_p0    <= 17'd32768;
            half_p0      <= HALF_01;
            freq_p0      <= 2'b01;
            duty_p0      <= 16'sd0;
            product_a_p1 <= (17'd32768 * HALF_01);
            product_b_p1 <= (17'd32768 * HALF_01);
            half_p1      <= HALF_01;
            freq_p1      <= 2'b01;
            duty_p1      <= 16'sd0;
        end else begin
            duty_a_p0 <= 17'd32768 + duty_ref_ext;
            duty_b_p0 <= 17'd32768 - duty_ref_ext;
            half_p0   <= half_input;
            freq_p0   <= freq_sel;
            duty_p0   <= duty_ref;

            product_a_p1 <= duty_a_p0 * half_p0;
            product_b_p1 <= duty_b_p0 * half_p0;
            half_p1      <= half_p0;
            freq_p1      <= freq_p0;
            duty_p1      <= duty_p0;
        end
    end

    assign period_counts = {half_period_active, 1'b0};

    always_ff @(posedge clk or posedge rst) begin
        if (rst) begin
            phase_counter      <= '0;
            freq_active        <= 2'b01;
            duty_active        <= 16'sd0;
            half_period_active <= HALF_01;
            threshold_a_active <= HALF_01 / 2;
            threshold_b_active <= HALF_01 / 2;
            pwm_trigger        <= 1'b0;
        end else if (!enable) begin
            phase_counter      <= '0;
            freq_active        <= freq_p1;
            duty_active        <= duty_p1;
            half_period_active <= half_p1;
            threshold_a_active <= product_a_p1[HALF_W+15:16];
            threshold_b_active <= product_b_p1[HALF_W+15:16];
            pwm_trigger        <= 1'b0;
        end else if (phase_counter >= period_counts - 1'b1) begin
            phase_counter      <= '0;
            freq_active        <= freq_p1;
            duty_active        <= duty_p1;
            half_period_active <= half_p1;
            threshold_a_active <= product_a_p1[HALF_W+15:16];
            threshold_b_active <= product_b_p1[HALF_W+15:16];
            pwm_trigger        <= 1'b1;
        end else begin
            phase_counter <= phase_counter + 1'b1;
            pwm_trigger   <= 1'b0;
        end
    end

    always_comb begin
        if (phase_counter < half_period_active)
            carrier = phase_counter;
        else
            carrier = period_counts - 1'b1 - phase_counter;

        raw_a = (carrier < threshold_a_active);
        raw_b = (carrier < threshold_b_active);
    end

    // Rama A: no permite encender simultaneamente los dos transistores.
    always_ff @(posedge clk or posedge rst) begin
        if (rst) begin
            pwm_a_high <= 1'b0;
            pwm_a_low  <= 1'b0;
            dead_a     <= '0;
        end else if (!enable) begin
            pwm_a_high <= 1'b0;
            pwm_a_low  <= 1'b0;
            dead_a     <= '0;
        end else begin
            if ((pwm_a_high && !raw_a) || (pwm_a_low && raw_a)) begin
                pwm_a_high <= 1'b0;
                pwm_a_low  <= 1'b0;
                dead_a     <= deadtime_reload;
            end else if (!pwm_a_high && !pwm_a_low) begin
                if (dead_a != 0)
                    dead_a <= dead_a - 1'b1;
                else if (raw_a)
                    pwm_a_high <= 1'b1;
                else
                    pwm_a_low <= 1'b1;
            end
        end
    end

    // Rama B: misma proteccion y mismo tiempo muerto exacto.
    always_ff @(posedge clk or posedge rst) begin
        if (rst) begin
            pwm_b_high <= 1'b0;
            pwm_b_low  <= 1'b0;
            dead_b     <= '0;
        end else if (!enable) begin
            pwm_b_high <= 1'b0;
            pwm_b_low  <= 1'b0;
            dead_b     <= '0;
        end else begin
            if ((pwm_b_high && !raw_b) || (pwm_b_low && raw_b)) begin
                pwm_b_high <= 1'b0;
                pwm_b_low  <= 1'b0;
                dead_b     <= deadtime_reload;
            end else if (!pwm_b_high && !pwm_b_low) begin
                if (dead_b != 0)
                    dead_b <= dead_b - 1'b1;
                else if (raw_b)
                    pwm_b_high <= 1'b1;
                else
                    pwm_b_low <= 1'b1;
            end
        end
    end
endmodule

