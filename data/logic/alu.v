
module ALU(
input                  clk,   // clock

input [DATA_WIDTH-1:0] a,
input [DATA_WIDTH-1:0] b,
input [0:0]            cin,
input [2:0]            op,
input                  alue,

output wire [DATA_WIDTH-1:0] r,
// output                 z,
// output                 n,
output reg                   cout,
);



reg [DATA_WIDTH-1:0] res;



always @*
begin
	res = DATA_WIDTH'b0;
	cout = 1'b0;
	
	case (op)
		3'b000 : {cout, res} = a + b + cin;
		3'b001 : {cout, res} = a - b + cin;
		3'b010 : res = a & b;
		3'b011 : res = a | b;
		3'b100 : res = -a;
		3'b101 : res = a << 1;
		3'b110 : res = a >> 1;
	endcase
	
//	z = (r == 0);
//	n = r[7:7];
end



assign r = alue ? res : DATA_WIDTH'bz;



endmodule
