#pragma once



namespace vidf
{


	inline bool IsFlagSet(int bitfield, int flag)
	{
		return (bitfield & flag) != 0;
	}



	inline int SetFlag(int bitfield, int flag)
	{
		return bitfield | flag;
	}



	inline int RemoveFlag(int bitfield, int flag)
	{
		return bitfield & ~flag;
	}



	inline int SetFlagIf(int bitfield, int flag, bool set)
	{
		if (set)
			return SetFlag(bitfield, flag);
		else
			return RemoveFlag(bitfield, flag);
	}


}
