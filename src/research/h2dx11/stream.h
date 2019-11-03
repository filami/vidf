#pragma once

namespace h2
{



	enum class StreamResult
	{
		Ok,
		Fail,
	};



	template<typename TStream>
	struct StreamTraits
	{
	};



	template<>
	struct StreamTraits<std::istream>
	{
		static constexpr bool IsInput() { return true; }
		static constexpr bool IsOutput() { return false; }
		static constexpr bool IsBinary() { return true; }
	};
	template<> struct StreamTraits<std::ifstream> : public StreamTraits<std::istream> {};



	inline StreamResult Stream(std::istream& stream, vidf::int16& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return StreamResult::Ok;
	};



	inline StreamResult Stream(std::istream& stream, vidf::uint16& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return StreamResult::Ok;
	};



	inline StreamResult Stream(std::istream& stream, vidf::int32& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return StreamResult::Ok;
	};



	inline StreamResult Stream(std::istream& stream, vidf::uint32& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return StreamResult::Ok;
	};



	inline StreamResult Stream(std::istream& stream, float& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return StreamResult::Ok;
	};



	template<typename TStream, typename T>
	StreamResult Stream(TStream& stream, vidf::Vector3<T>& value)
	{
		Stream(stream, value.x);
		Stream(stream, value.y);
		Stream(stream, value.z);
		return StreamResult::Ok;
	};



	template<int N>
	StreamResult Stream(std::istream& stream, char(&value)[N])
	{
		stream.read(value, N);
		return StreamResult::Ok;
	};



	template<int N>
	StreamResult Stream(std::istream& stream, vidf::uint8(&value)[N])
	{
		stream.read((char*)value, N);
		return StreamResult::Ok;
	};



	template<typename TStream, typename It>
	StreamResult Stream(TStream& stream, It& begin, It& end)
	{
		for (It it = begin; it != end; ++it)
			Stream(stream, *it);
		return StreamResult::Ok;
	};



	template<int N>
	StreamResult Stream(std::istream& stream, std::array<char, N>& value)
	{
		stream.read(value.data(), N);
		return StreamResult::Ok;
	};


	template<typename TStream>
	StreamResult Stream(TStream& stream, std::vector<char>& value)
	{
		stream.read(value.data(), value.size());
		return StreamResult::Ok;
	};



}
