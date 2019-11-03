#include "pch.h"
#include "renderpass.h"

namespace vidf::dx11
{



	void PassNode::Connect(PassNodePtr source, const char* sourceName, PassNodePtr target, const char* targetName)
	{
	}



	void PassNode::AddInput(const char* name)
	{
		assert(find_if(inputNames.begin(), inputNames.end(), [name](auto input) { return strcmp(name, input.c_str()) == 0; }) == inputNames.end());
		inputs.emplace_back();
		inputNames.push_back(name);
	}



	void PassNode::AddOutput(const char* name)
	{
		assert(find_if(outputNames.begin(), outputNames.end(), [name](auto input) { return strcmp(name, input.c_str()) == 0; }) == outputNames.end());
		outputs.emplace_back();
		outputNames.push_back(name);
	}



	PassEdgePtr PassNode::GetInput(const char* name) const
	{
		auto it = find_if(inputNames.begin(), inputNames.end(), [name](auto input) { return strcmp(name, input.c_str()) == 0; });
		assert(it != inputNames.end());
		return inputs[it - inputNames.begin()];
	}



//	PassEdgePtr PassNode::GetOutput(const char* name) const
//	{
//		auto it = find_if(outputNames.begin(), outputNames.end(), [name](auto input) { return strcmp(name, input.c_str()) == 0; });
//		assert(it != outputNames.end());
//	}



}
