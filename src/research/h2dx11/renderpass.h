#pragma once
#include "vidf/rendererdx11/resources.h"

namespace vidf::dx11
{

	using namespace std;


	class PassEdge;
	class PassNode;
	typedef shared_ptr<PassEdge> PassEdgePtr;
	typedef shared_ptr<PassNode> PassNodePtr;


	class PassGBufferEdge;



	class PassEdge
	{
	public:
		virtual PassGBufferEdge* AsGBuffer() { return nullptr; }

	private:
		PassNodePtr sourceNode;
		PassNodePtr targetNode;
		uint sourceIndex = 0;
		uint targetIndex = 0;
	};



	class PassGBufferEdge : public PassEdge
	{
	public:
		virtual PassGBufferEdge* AsGBuffer() { return this; }

	private:
		GPUBuffer gbuffer;
	};



	class PassNode
	{
	public:
		typedef std::deque<PassEdgePtr> OutputEdges;

	public:
		virtual void Build(uint outputIdx) = 0;

		static void Connect(PassNodePtr source, const char* sourceName, PassNodePtr target, const char* targetName);

	protected:
		void AddInput(const char* name);
		void AddOutput(const char* name);

		PassEdgePtr GetInput(const char* name) const;
	//	PassEdgePtr GetOutput(const char* name) const;

	private:
		deque<PassEdgePtr> inputs;
		deque<OutputEdges> outputs;
		deque<string> inputNames;
		deque<string> outputNames;
	};



}
