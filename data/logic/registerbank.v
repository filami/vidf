
module RegisterBank(
input                   clk,   // clock
input                   rst,   // reset

input  [DATA_WIDTH-1:0] dbi,   // data bus input
output [DATA_WIDTH-1:0] db,    // data bus output
input  [1:0]            dba,   // data bus address
input                   dbe,   // data bus enable
input                   dbld,  // data bus load

output [DATA_WIDTH-1:0] sb,    // secondary bus output
input  [1:0]            sba,   // secondary bus address
input                   sbe,   // secondary bus enable

input                   ral_adl, // drive address bus low with address register low
input                   rah_adh, // drive address bus high with address register high
);

reg [DATA_WIDTH-1:0] registers [0:3];

always @(posedge clk)
begin
	if (dbld)
		registers[dba] <= dbi;
end

assign db = dbe ? registers[dba] : DATA_WIDTH'bz;
assign sb = sbe ? registers[sba] : DATA_WIDTH'bz;

endmodule
