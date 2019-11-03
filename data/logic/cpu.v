module CPU(
input                   clk,   // clock
input                   rst,   // reset
inout [DATA_WIDTH-1:0]  iodb,  // input/output data bus
output                  rw,    // data bus read/write (1 = write, 0 = read)
output [DATA_WIDTH-1:0] abl,   // address bus low
output [DATA_WIDTH-1:0] abh,   // address bus high
);

reg  [DATA_WIDTH-1:0] dlo;   // output data latch

wire [DATA_WIDTH-1:0] db;   // data bus
wire [DATA_WIDTH-1:0] sb;   // secondary bus

wire       pci;
wire       pcl_abl;
wire       pch_abh;
wire       pcl_db;
wire       pch_db;
wire       abl_pcl;
wire       abh_pch;
wire [2:0] op;
wire       alue;
wire [1:0] dba;
wire [1:0] sba;
wire       dbe;
wire       sbe;
wire       dbld;
wire       dl_db;
wire       dl_abl;
wire       dl_abh;
wire       ral_adl;
wire       ral_adl;

RegisterBank rb(
.clk(clk),
.rst(rst),
.dbi(db),
.db(db),
.dba(dba),
.dbe(dbe),
.dbld(dbld),
.sb(sb),
.sba(sba),
.sbe(sbe),
.ral_adl(ral_adl),
.ral_adl(ral_adl),
);


ProgramCounter pc(
.clk(clk),
.rst(rst),
.db(db),
.abl(abl),
.abh(abh),
.abli(abl),
.abhi(abh),
.pcl_abl(pcl_abl),
.pch_abh(pch_abh),
.pcl_db(pcl_db),
.pch_db(pch_db),
.abl_pcl(abl_pcl),
.abh_pch(abh_pch),
.pci(pci),
);


ALU alu(
.clk(clk),
.a(db),
.b(sb),
//.cin,
.op(op),
.r(db),
//.cout,
.alue(alue),
);


Control control(
.clk(clk),
.rst(rst),
.iodb(iodb),
.pci(pci),
.dl_db(dl_db),
.dl_abl(dl_abl),
.dl_abh(dl_abh),
.pcl_abl(pcl_abl),
.pch_abh(pch_abh),
.pcl_db(pcl_db),
.pch_db(pch_db),
.abl_pcl(abl_pcl),
.abh_pch(abh_pch),
.op(op),
.alue(alue),
.dba(dba),
.sba(sba),
.dbe(dbe),
.sbe(sbe),
.ral_adl(ral_adl),
.rah_adh(rah_adh),
.dbld(dbld),
.rw(rw),
);


always @(posedge clk)
begin
	dlo <= db;
end


assign db = dl_db ? iodb : DATA_WIDTH'bz;
assign abl = dl_abl ? iodb : DATA_WIDTH'bz;
assign abh = dl_abh ? iodb : DATA_WIDTH'bz;
assign iodb = rw ? dlo : DATA_WIDTH'bz;

endmodule
