#pragma once


namespace vidf
{

	const struct Zero {} zero;


	typedef __int8				int8;
	typedef unsigned __int8		uint8;
	typedef __int16				int16;
	typedef unsigned __int16	uint16;
	typedef __int32				int32;
	typedef unsigned __int32	uint32;
	typedef __int64				int64;
	typedef unsigned __int64	uint64;
	typedef float				real32;
	typedef double				real64;
	struct vector3 {real32 values[3];};
	struct vector4 {real32 values[4];};
	struct matrix34 {real32 values[3][4];};

	typedef unsigned int uint;


}
