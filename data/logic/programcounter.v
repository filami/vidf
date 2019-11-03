
module ProgramCounter(
input                   clk,      // clock
input                   rst,      // reset
output [DATA_WIDTH-1:0] db,       // data bus
output [DATA_WIDTH-1:0] abl,      // address bus low
output [DATA_WIDTH-1:0] abh,      // address bus high
input  [DATA_WIDTH-1:0] abli,     // address bus low input
input  [DATA_WIDTH-1:0] abhi,     // address bus high input
input                   pcl_abl,  // drive address bus low with program counter low
input                   pch_abh,  // drive address high low with program counter high
input                   pcl_db,   // drive data bus with program counter low
input                   pch_db,   // drive data bus low with program counter high
input                   abl_pcl,  // load data low program counter with data bus
input                   abh_pch,  // load data high program counter with data bus
input                   pci,      // increment program counter
);

reg [DATA_WIDTH-1:0] pcl;
reg [DATA_WIDTH-1:0] pch;

reg [DATA_WIDTH-1:0] newPcl;
reg [DATA_WIDTH-1:0] newPch;
reg cout;


always @*
begin
	newPcl = abl_pcl ? abli : pcl;
	newPch = abh_pch ? abhi : pch;
	if (pci)
	begin
		{cout, newPcl} = newPcl + 1;
		newPch = newPch + cout;
	end
end


always @(posedge clk or posedge rst)
begin
	if (rst)
	begin
		pcl <= DATA_WIDTH'b10000;
		pch <= DATA_WIDTH'b0;
	end
	else
	begin
		pcl <= newPcl;
		pch <= newPch;
	end
end


assign abl = pcl_abl ? newPcl : DATA_WIDTH'bz;
assign abh = pch_abh ? newPch : DATA_WIDTH'bz;
assign db = pcl_db ? newPcl : DATA_WIDTH'bz;
assign db = pch_db ? newPch : DATA_WIDTH'bz;

endmodule
