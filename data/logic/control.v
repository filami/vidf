module Control(
input wire                  clk,  // clock
input wire                  rst,  // reset
input wire [DATA_WIDTH-1:0] iodb, // data bus

output reg       dl_db,
output reg       dl_abl,
output reg       dl_abh,
output reg       pci,
output reg       pcl_abl,
output reg       pch_abh,
output reg       pcl_db,
output reg       pch_db,
output reg       abl_pcl,
output reg       abh_pch,
output reg [2:0] op,
output reg       alue,
output reg [1:0] dba,
output reg [1:0] sba,
output reg       dbe,
output reg       sbe,
output reg       dbld,
output reg       ral_adl,
output reg       rah_adh,
output reg       dbld,
output reg       rw,
);

// internal signals
reg step;
reg retire;
reg preLoadOp;
reg [DATA_WIDTH-1:0] opcode;
reg [DATA_WIDTH-1:0] preop;
reg [2:0] state;
parameter [7:0] T0 = 8'b00000001;
parameter [7:0] T1 = 8'b00000010;
parameter [7:0] T2 = 8'b00000100;
parameter [7:0] T3 = 8'b00001000;
parameter [7:0] T4 = 8'b00010000;
parameter [7:0] T5 = 8'b00100000;
parameter [7:0] T6 = 8'b01000000;
parameter [7:0] T7 = 8'b10000000;



function IsAluOp(input [7:0] op);
begin
	if (opcode[7:7] == 1'b1)
		IsAluOp = 1'b1;
	else
		IsAluOp = 1'b0;
end
endfunction



function IsLoad(input [7:0] op);
begin
	if (opcode[7:2] == 6'b001000)
		IsLoad = 1'b1;
	else
		IsLoad = 1'b0;
end
endfunction



function IsStore(input [7:0] op);
begin
	if (opcode[7:2] == 6'b010000)
		IsStore = 1'b1;
	else
		IsStore = 1'b0;
end
endfunction



function IsLoadStore(input [7:0] op);
begin
	IsLoadStore = (IsLoad(op) || IsStore(op)) ? 1'b1 : 1'b0;
end
endfunction



always @*
begin
	pci = 1'b0;
	dl_db = 1'b0;
	dl_abl = 1'b0;
	dl_abh = 1'b0;
	pcl_abl = 1'b0;
	pch_abh = 1'b0;
	pcl_db = 1'b0;
	pch_db = 1'b0;
	abl_pcl = 1'b0;
	abh_pch = 1'b0;
	op = 3'bx;
	alue = 1'b0;
	dba = 2'bx;
	sba = 2'bx;
	dbe = 1'b0;
	sbe = 1'b0;
	dbld = 1'b0;
	rw = 1'b0;
	ral_adl = 1'b0;
	rah_adh = 1'b0;

	preLoadOp = 1'b0;
	step = 1'b1;
	retire = 1'b0;
	
	case (state)
	T0:
	begin
		pci = 1'b1;
		pcl_abl = 1'b1;
		pch_abh = 1'b1;
		preLoadOp = 1'b1;
	end
	endcase
	
	if (IsAluOp(opcode))
		case (state)
		T0:
		begin
			op = opcode[6:4];
			dba[1:0] = opcode[1:0];
			sba[1:0] = opcode[3:2];
			dbe = 1'b1;
			sbe = 1'b1;
		end
		T1:
		begin
			alue = 1'b1;
			dba[1:0] = opcode[1:0];
			dbld = 1'b1;
			retire = 1'b1;
		end
		endcase
	
	else if (IsLoadStore(opcode))
		case (state)
		T1:
		begin
			dba[1:0] = opcode[1:0];
			ral_adl = 1'b1;
			rah_adh = 1'b1;
			if (IsStore(opcode))
			begin
				dbe = 1'b1;
				rw = 1'b1;
			end
			else if (IsLoad(opcode))
			begin
				dbld = 1'b1;
				dl_db = 1'b1;
			end
			retire = 1'b1;
		end
		endcase
		
	else
		case (state)
		T1:
		begin
			retire = 1'b1;
		end
		endcase
end



always @(posedge clk)
begin
	if (rst || retire)
	begin
		state <= T0;
		opcode <= retire ? preop : 8'b00000000;
	end
	else if (step)
		state <= (state << 1);
	if (preLoadOp)
		preop <= iodb;
end


endmodule
