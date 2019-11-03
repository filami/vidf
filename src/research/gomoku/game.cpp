#include "pch.h"
#include "vidf/common/random.h"

using namespace vidf;
using namespace proto;


namespace
{

	const uint tableSize = 15;


	vidf::Rand48 rand48;
	vidf::UniformReal<float> snorm(-1.0f, 1.0f);



	enum class State : uint8
	{
		Empty,
		Black,
		White,
	};



	typedef std::array<State, tableSize*tableSize> TableState;
	typedef std::array<uint8, tableSize*tableSize> TableMarks;



	void DrawTable(const TableState& tableState, const TableMarks& tableMarks, State currentPlayer, Vector2i ghostPiecePos, bool showGhostPiece)
	{
		auto DrawBorder = [](uint tableSize, float borderSize, bool wireframe)
		{
			if (wireframe)
				glBegin(GL_LINE_LOOP);
			else
				glBegin(GL_QUADS);
			glVertex2f(-borderSize, -borderSize);
			glVertex2f(tableSize + borderSize - 1.0f, -borderSize);
			glVertex2f(tableSize + borderSize - 1.0f, tableSize + borderSize - 1.0f);
			glVertex2f(-borderSize, tableSize + borderSize - 1.0f);
			glEnd();
		};

		const float borderSize = 0.75f;
		glColor4ub(255, 205, 196, 255);
		DrawBorder(tableSize, borderSize, false);
		glEnd();
		glColor4ub(0, 0, 0, 255);
		if (currentPlayer == State::Black)
		{
			glLineWidth(7.0f);
			DrawBorder(tableSize, borderSize, true);
			glEnd();
		}
		else if (currentPlayer == State::White)
		{
			const float outterBorderSize = 0.15f;
			glLineWidth(1.0f);
			DrawBorder(tableSize, borderSize, true);
			DrawBorder(tableSize, borderSize + 0.15f, true);
		}

		glLineWidth(3.0f);
		glColor4ub(96, 96, 96, 255);
		glBegin(GL_LINES);
		for (uint i = 0; i < tableSize; ++i)
		{
			glVertex2f(i, 0.0f);
			glVertex2f(i, tableSize-1);
			glVertex2f(0.0f, i);
			glVertex2f(tableSize-1, i);
		}
		glEnd();

		const float goPieceSize = 0.45f;
		auto DrawCircle = [](Vector2f center, float radius, bool wireframe)
		{
			const uint numSegments = 24;
			const float twopi = 2.0f * std::acos(-1.0f);
			const float step = twopi / float(numSegments);
			if (wireframe)
				glBegin(GL_LINE_STRIP);
			else
				glBegin(GL_POLYGON);
			for (float t = 0.0f; t < twopi; t += step)
			{
				Vector2f p = Vector2f(std::cos(t), std::sin(t)) * radius + center;
				glVertex2f(p.x, p.y);
			}
			glEnd();
		};
		auto glSetPlayerColor = [](State player, uint8 alpha)
		{
			if (player == State::Black)
				glColor4ub(8, 8, 8, alpha);
			else if (player == State::White)
				glColor4ub(255, 255, 255, alpha);
		};

		glLineWidth(3.0f);
		for (uint y = 0; y < tableSize; ++y)
		{
			for (uint x = 0; x < tableSize; ++x)
			{
				const auto state = tableState[x + y*tableSize];
				if (state == State::Empty)
					continue;
				glSetPlayerColor(state, 255);
				DrawCircle(Vector2f(x, y), goPieceSize, false);
				if (tableMarks[x + y*tableSize] == 0)
					glColor4ub(156, 156, 156, 255);
				else
					glColor4ub(255, 64, 64, 255);
				DrawCircle(Vector2f(x, y), goPieceSize, true);
			}
		}
		if (showGhostPiece)
		{
			glSetPlayerColor(currentPlayer, 128);
			DrawCircle(Vector2f(ghostPiecePos), goPieceSize, false);
			glSetPlayerColor(currentPlayer, 255);
			DrawCircle(Vector2f(ghostPiecePos), goPieceSize, true);
		}
	}



	State FinishedState(const TableState& tableState)
	{
		const uint minRowSize = 5;
		auto CountRow = [minRowSize](const TableState& tableState, Vector2i position, Vector2i direction, State checkState)
		{
			uint rowSize = 1;
			for (uint i = 1; i < minRowSize; ++i)
			{
				auto thisPos = position + direction * i;
				if (thisPos.x < 0 || thisPos.y < 0)
					break;
				if (thisPos.x >= tableSize || thisPos.y >= tableSize)
					break;
				auto thisState = tableState[thisPos.x + thisPos.y*tableSize];
				if (thisState == checkState)
					rowSize++;
				else
					break;
			}
			return rowSize;
		};

		const Vector2i directions[] =
		{
			Vector2i(1,  0),
			Vector2i(0,  1),
			Vector2i(1,  1),
			Vector2i(1, -1),
		};
		for (uint y = 0; y < tableSize; ++y)
		{
			for (uint x = 0; x < tableSize; ++x)
			{
				const auto checkState = tableState[x + y*tableSize];
				if (checkState == State::Empty)
					continue;
				for (uint i = 0; i < 4; ++i)
				{
					uint count = CountRow(tableState, Vector2i(x, y), directions[i], checkState);
					if (count == minRowSize)
						return checkState;
				}
			}
		}
		return State::Empty;
	};



	// #TODO copy pasted of FinishedState. make it templated
	State FinishedStateAndMArk(const TableState& tableState, TableMarks* tableMarks)
	{
		const uint minRowSize = 5;
		auto CountRow = [minRowSize](const TableState& tableState, Vector2i position, Vector2i direction, State checkState)
		{
			uint rowSize = 1;
			for (uint i = 1; i < minRowSize; ++i)
			{
				auto thisPos = position + direction * i;
				if (thisPos.x < 0 || thisPos.y < 0)
					break;
				if (thisPos.x >= tableSize || thisPos.y >= tableSize)
					break;
				auto thisState = tableState[thisPos.x + thisPos.y*tableSize];
				if (thisState == checkState)
					rowSize++;
				else
					break;
			}
			return rowSize;
		};
		auto MarkRow = [](TableMarks* tableMarks, Vector2i position, Vector2i direction, uint count)
		{
			for (uint i = 0; i < count; ++i)
			{
				auto thisPos = position + direction * i;
				(*tableMarks)[thisPos.x + thisPos.y*tableSize] = 1;
			}
		};

		State returnState = State::Empty;
		const Vector2i directions[] =
		{
			Vector2i(1,  0),
			Vector2i(0,  1),
			Vector2i(1,  1),
			Vector2i(1, -1),
		};
		for (uint y = 0; y < tableSize; ++y)
		{
			for (uint x = 0; x < tableSize; ++x)
			{
				const auto checkState = tableState[x + y*tableSize];
				if (checkState == State::Empty)
					continue;
				for (uint i = 0; i < 4; ++i)
				{
					uint count = CountRow(tableState, Vector2i(x, y), directions[i], checkState);
					if (count == minRowSize)
					{
						MarkRow(tableMarks, Vector2i(x, y), directions[i], count);
						returnState = checkState;
					}
				}
			}
		}
		return returnState;
	};



	State OtherPlayer(State player)
	{
		return player == State::Black ? State::White : State::Black;
	}


	
	const float infinity = std::numeric_limits<float>::max();


	struct AlphaBetaResult
	{
		Vector2i position;
		float bestValue;
	};



	float Heuristic(const TableState& tableState, State player)
	{
		const uint minRowSize = 5;
		auto CountRow = [minRowSize](const TableState& tableState, Vector2i position, Vector2i direction, State checkState)
		{
			std::pair<uint, uint> rowSize = std::pair<uint, uint>(1, 1);
			for (uint i = 1; i < minRowSize; ++i)
		//	for (uint i = 1; i < tableSize; ++i)
			{
				auto thisPos = position + direction * i;
				if (thisPos.x < 0 || thisPos.y < 0)
					break;
				if (thisPos.x >= tableSize || thisPos.y >= tableSize)
					break;
				auto thisState = tableState[thisPos.x + thisPos.y*tableSize];
				if (thisState != State::Empty && thisState != checkState)
					break;
				rowSize.second++;
				if (thisState == checkState)
					rowSize.first++;
			}
			return rowSize;
		};

		std::pair<uint, uint> bestWhite(0, 0);
		std::pair<uint, uint> bestBlack(0, 0);
		auto MaxPair = [](std::pair<uint, uint> a, std::pair<uint, uint> b)
		{
			return std::pair<uint, uint>(Max(a.first, b.first), Max(a.second, b.second));
		};

		const Vector2i directions[] =
		{
			Vector2i(1,  0),
			Vector2i(0,  1),
			Vector2i(1,  1),
			Vector2i(1, -1),
			Vector2i(-1, 0),
			Vector2i(0, -1),
			Vector2i(-1,-1),
			Vector2i(-1, 1),
		};
		for (uint y = 0; y < tableSize; ++y)
		{
			for (uint x = 0; x < tableSize; ++x)
			{
				const auto checkState = tableState[x + y*tableSize];
				if (checkState == State::Empty)
					continue;
				for (uint i = 0; i < 8; ++i)
				{
					auto count = CountRow(tableState, Vector2i(x, y), directions[i], checkState);
					if (count.second != 5)
						continue;
					if (checkState == State::White)
						bestWhite = MaxPair(bestWhite, count);
					if (checkState == State::Black)
						bestBlack = MaxPair(bestBlack, count);
				}
			}
		}

		float value = 0.0f;
		if (player == State::Black)
		{
			if (bestWhite.first > bestBlack.first)
				value = -float(bestWhite.first);
			else
				value = bestBlack.second;
		}
		else
		{
			if (bestWhite.first > bestBlack.first)
				value = bestWhite.second;
			else
				value = -float(bestBlack.first);
		}

		return value;
	}



	typedef std::array<Vector2i, tableSize*tableSize> AlphaBetaPoints;


	AlphaBetaResult AlphaBeta(const AlphaBetaPoints& points, const TableState& tableState, uint depth, float alpha, float beta, State player, bool isMaximizing)
	{
		const State thisPlayer = isMaximizing ? player : OtherPlayer(player);
		AlphaBetaResult result;
		result.position = Vector2i(-1, -1);
		result.bestValue = isMaximizing ? -infinity : infinity;

		if (FinishedState(tableState) != State::Empty)
		{
			result.bestValue *= 0.5f;
			return result;
		}
		if (depth == 0)
		{
			result.bestValue = Heuristic(tableState, isMaximizing ? OtherPlayer(player) : player);
			return result;
		}

		for (uint i = 0; i < points.size(); ++i)
		{
			const auto point = points[i];
			const auto state = tableState[point.x + point.y*tableSize];
			if (state != State::Empty)
				continue;
			TableState newTable = tableState;
			newTable[point.x + point.y*tableSize] = thisPlayer;

			const float newValue = AlphaBeta(points, newTable, depth - 1, alpha, beta, player, !isMaximizing).bestValue;

			if (isMaximizing && result.bestValue < newValue)
			{
				result.bestValue = newValue;
				alpha = Max(alpha, result.bestValue);
				result.position = point;
			}
			else if (!isMaximizing && result.bestValue > newValue)
			{
				result.bestValue = newValue;
				beta = Min(beta, result.bestValue);
				result.position = point;
			}
			if (beta <= alpha)
				break;
		}
		return result;
	}



	bool IsValidSpot(const TableState& tableState, Vector2i position)
	{
		if (position.x < 0 || position.y < 0)
			return false;
		if (position.x >= tableSize || position.y >= tableSize)
			return false;
		return tableState[position.x + position.y*tableSize] == State::Empty;
	};



	/////////////////////////////////////////////////////////////////


	template<typename T>
	class range
	{
	public:
		range() = default;
		range(T* __begin, T* __end)
			: _begin(__begin)
			, _end(__end) {}

		T* begin() { return _begin; }
		T* end() { return _end; }
		T* cbegin() const { return _begin; }
		T* cend() const { return _end; }
		size_t size() const { return _end - _begin; }
		T& operator[] (size_t idx) { assert(idx < size()); return _begin[idx]; }
		const T operator[] (size_t idx) const { assert(idx < size()); return _begin[idx]; }

	private:
		T* _begin;
		T* _end;
	};


	struct NeuralNetworkDesc
	{
		uint numInputs = 0;
		uint numOutputs = 0;
		uint neuronsPerLayer = 0;
		uint numLayers = 0;
		uint numFeedbackNeurons = 0;
	};


	template<typename T>
	class NeuralNetwork
	{
	public:
		NeuralNetwork()
		{
		}

		void Create(const NeuralNetworkDesc& _desc)
		{
			desc = _desc;
			feedbackInputOffset = desc.numInputs;
			outputOffset = desc.numInputs + desc.numFeedbackNeurons + desc.neuronsPerLayer * desc.numLayers;
			feedbackOutputOffset = outputOffset + desc.numOutputs + desc.numFeedbackNeurons;
			const uint numNeurons = feedbackOutputOffset + desc.numFeedbackNeurons;
			neurons.resize(numNeurons);

			uint numWeigths =
				(desc.numInputs + desc.numFeedbackNeurons) * desc.neuronsPerLayer +
				(desc.numOutputs + desc.numFeedbackNeurons) * desc.neuronsPerLayer;
			for (uint l = 0; l < desc.numLayers - 1; ++l)
				numWeigths += desc.neuronsPerLayer * desc.neuronsPerLayer;
			weigths.resize(numWeigths);
		}

		range<T> GetInputNeurons()
		{
			return range<T>(neurons.data(), neurons.data() + desc.numInputs);
		}

		range<const T> GetOutputNeurons() const
		{
			return range<const T>(neurons.data() + outputOffset, neurons.data() + outputOffset + desc.numOutputs);
		}

		range<T> GetWeigths()
		{
			return range<T>(weigths.data(), weigths.data() + weigths.size());
		}

		void Update()
		{
			memcpy(neurons.data() + feedbackInputOffset, neurons.data() + feedbackOutputOffset, sizeof(T) * desc.numFeedbackNeurons);

			range<T> inputRange = range<T>(neurons.data(), neurons.data() + desc.numInputs + desc.numFeedbackNeurons);
			range<T> outputRange = range<T>(inputRange.end(), inputRange.end() + desc.neuronsPerLayer);
			range<T> weightsRange = range<T>(weigths.data(), weigths.data() + (desc.numInputs + desc.numFeedbackNeurons) * desc.neuronsPerLayer);
			UpdateLayer(inputRange, outputRange, weightsRange);

			for (uint l = 0; l < desc.numLayers - 1; ++l)
			{
				inputRange = outputRange;
				outputRange = range<T>(outputRange.end(), outputRange.end() + desc.neuronsPerLayer);
				weightsRange = range<T>(weightsRange.end(), weightsRange.end() + desc.neuronsPerLayer * desc.neuronsPerLayer);
				UpdateLayer(inputRange, outputRange, weightsRange);
			}

			inputRange = outputRange;
			outputRange = range<T>(outputRange.end(), outputRange.end() + desc.numOutputs + desc.numFeedbackNeurons);
			weightsRange = range<T>(weightsRange.end(), weightsRange.end() + (desc.numOutputs + desc.numFeedbackNeurons) * desc.neuronsPerLayer);
			UpdateLayer(inputRange, outputRange, weightsRange);
		}

	private:
		void UpdateLayer(range<T> inputs, range<T> outputs, range<T> weigths)
		{
			uint wIdx = 0;
			for (uint o = 0; o < outputs.size(); ++o)
			{
				T w = 0.0f;
				for (uint i = 0; i < outputs.size(); ++i)
					w += inputs[i] * weigths[wIdx++];
				outputs[o] = w;
			}
		}

	private:
		NeuralNetworkDesc desc;
		uint feedbackInputOffset = 0;
		uint feedbackOutputOffset = 0;
		uint outputOffset = 0;
		std::vector<T> neurons;
		std::vector<T> weigths;
	};



	/////////////////////////////////////////////////////////////////



	class Player
	{
	public:
		Player(State _thisPlayer)
			: thisPlayer(_thisPlayer)
			, otherPlayer(OtherPlayer(_thisPlayer)) {}

		virtual bool Update(TableState* tableState) = 0;

		virtual void Draw(const TableState& tableState, const TableMarks& tableMarks) {}

		const State thisPlayer;
		const State otherPlayer;
	};



	class HumanPlayer : public Player
	{
	public:
		HumanPlayer(State _thisPlayer, const CameraOrtho2D& _camera)
			: Player(_thisPlayer)
			, camera(_camera)
			, lbuttonWasDown(false) {}

		bool Update(TableState* tableState) override
		{
			bool played = false;

			const Vector2f mousePos = camera.CursorPosition();
			const Vector2i cursorPos = Vector2i(uint(mousePos.x + 0.5f), uint(mousePos.y + 0.5f));
			const bool isValidSpot = IsValidSpot(*tableState, cursorPos);

			const bool lbuttonDown = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
			if (!lbuttonDown)
			{
				lbuttonWasDown = false;
			}
			else if (lbuttonDown && !lbuttonWasDown)
			{
				lbuttonWasDown = true;
				if (isValidSpot)
				{
					(*tableState)[cursorPos.x + cursorPos.y*tableSize] = thisPlayer;
					played = true;
				}
			}

			return played;
		}

		void Draw(const TableState& tableState, const TableMarks& tableMarks) override
		{
			const Vector2f mousePos = camera.CursorPosition();
			const Vector2i cursorPos = Vector2i(uint(mousePos.x + 0.5f), uint(mousePos.y + 0.5f));
			const bool isValidSpot = IsValidSpot(tableState, cursorPos);
			DrawTable(tableState, tableMarks, thisPlayer, cursorPos, isValidSpot);
		}

	private:
		const CameraOrtho2D& camera;
		bool lbuttonWasDown;
	};



	class AlphaBetaPlayer : public Player
	{
	public:
		AlphaBetaPlayer(State _thisPlayer)
			: Player(_thisPlayer)
		{
			Vector2i center = Vector2i(tableSize / 2, tableSize / 2);
			alphaBetaPoints[0] = center;
			uint idx = 1;
			for (int i = 1; i <= center.x; ++i)
			{
				for (int j = -i; j < i; ++j)
					alphaBetaPoints[idx++] = center + Vector2i(j, i);
				for (int j = i; j > -i; --j)
					alphaBetaPoints[idx++] = center + Vector2i(i, j);
				for (int j = i; j > -i; --j)
					alphaBetaPoints[idx++] = center + Vector2i(j, -i);
				for (int j = -i; j < i; ++j)
					alphaBetaPoints[idx++] = center + Vector2i(-i, j);
			}
		}

		bool Update(TableState* tableState) override
		{
			const auto play = AlphaBeta(alphaBetaPoints, *tableState, 3, -infinity, infinity, thisPlayer, true);
			(*tableState)[play.position.x + play.position.y*tableSize] = thisPlayer;
			return true;
		}

		void Draw(const TableState& tableState, const TableMarks& tableMarks) override
		{
			DrawTable(tableState, tableMarks, thisPlayer, Vector2i{zero}, false);
		}

	private:
		AlphaBetaPoints alphaBetaPoints;
	};



	class NeuralPlayer : public Player
	{
	public:
		NeuralPlayer(State _thisPlayer)
			: Player(_thisPlayer)
		{
			const uint numPlaces = tableSize * tableSize;
			NeuralNetworkDesc nnDesc;
			nnDesc.numInputs = nnDesc.numOutputs = numPlaces;
			nnDesc.numFeedbackNeurons = tableSize;
			nnDesc.neuronsPerLayer = numPlaces + tableSize;
			nnDesc.numLayers = 2;
			neuralNetwork.Create(nnDesc);

			auto weigths = neuralNetwork.GetWeigths();
			for (uint i = 0; i < weigths.size(); ++i)
				weigths[i] = snorm(rand48);
		}

		bool Update(TableState* tableState) override
		{
			const uint numPlaces = tableSize * tableSize;
			auto inputs = neuralNetwork.GetInputNeurons();
			for (uint i = 0; i < numPlaces; ++i)
			{
				if ((*tableState)[i] == thisPlayer)
					inputs[i] = 1.0f;
				if ((*tableState)[i] == otherPlayer)
					inputs[i] = -1.0f;
				else
					inputs[i] = 0.0f;
			}
			neuralNetwork.Update();
			auto outputs = neuralNetwork.GetOutputNeurons();
			float bestWeigth = -std::numeric_limits<float>::max();
			Vector2i bestPos = Vector2i{ zero };
			for (uint y = 0; y < tableSize; ++y)
			{
				for (uint x = 0; x < tableSize; ++x)
				{
					if (!IsValidSpot(*tableState, Vector2i(x, y)))
						continue;
					if (outputs[x + y*tableSize] > bestWeigth)
					{
						bestWeigth = outputs[x + y*tableSize];
						bestPos = Vector2i(x, y);
					}
				}
			}
			(*tableState)[bestPos.x + bestPos.y*tableSize] = thisPlayer;
			return true;
		}

		void Draw(const TableState& tableState, const TableMarks& tableMarks) override
		{
			DrawTable(tableState, tableMarks, thisPlayer, Vector2i{ zero }, false);
		}

	private:
		NeuralNetwork<float> neuralNetwork;
	};



	struct MatchResult
	{
		State winner = State::Empty;
		uint numPlays = 0;
	};



	std::mutex coutMtx;



	void OutputResult(const MatchResult& result, uint matchIndex)
	{
		std::lock_guard<std::mutex> guard(coutMtx);

		std::cout << "match " << matchIndex << " - ";
		switch (result.winner)
		{
		case State::Black:
			std::cout << "BLACK won";
			break;
		case State::White:
			std::cout << "WHITE won";
			break;
		default:
			std::cout << "TIE";
			break;
		}
		std::cout << " after " << result.numPlays << " plays" << std::endl;
	}


	MatchResult GomokuMatch(NeuralPlayer* player0, NeuralPlayer* player1)
	{
		MatchResult result;

		TableState tableState;
		TableMarks tableMarks;
		tableState.fill(State::Empty);
		tableState[tableSize / 2 + tableSize / 2 * tableSize] = State::White;
		tableMarks.fill(0);

		State finishedState = State::Empty;

		Player* currentPlayer = player0;
		Player* otherPlayer = player1;

		while (result.winner == State::Empty)
		{
			currentPlayer->Update(&tableState);
			std::swap(currentPlayer, otherPlayer);
			result.winner = FinishedStateAndMArk(tableState, &tableMarks);
			result.numPlays++;
		}

		return result;
	}


}


#if 0

void Gomoku()
{
	rand48.Seed(uint64(vidf::GetTime().GetTicks()));

/*	TableState tableTest;
	tableTest.fill(State::Empty);
	tableTest[0] = State::White;
	tableTest[1] = State::White;
	tableTest[2] = State::White;
	tableTest[15] = State::Black;
	tableTest[16] = State::Black;
	tableTest[17] = State::Black;
	tableTest[18] = State::Black;
	*/
	// const auto testPlay = AlphaBeta(tableTest, 2, -infinity, infinity, State::White, true);

	AlphaBetaPoints alphaBetaPoints;
/*	for (uint y = 0; y < tableSize; ++y)
	{
		for (uint x = 0; x < tableSize; ++x)
		{
			alphaBetaPoints[x + y*tableSize] = Vector2i(x, y);
		}
	}*/
	{
		Vector2i center = Vector2i(tableSize / 2, tableSize / 2);
		alphaBetaPoints[0] = center;
		uint idx = 1;
		for (int i = 1; i <= center.x; ++i)
		{
			for (int j = -i; j < i; ++j)
				alphaBetaPoints[idx++] = center + Vector2i(j, i);
			for (int j = i; j > -i; --j)
				alphaBetaPoints[idx++] = center + Vector2i(i, j);
			for (int j = i; j > -i; --j)
				alphaBetaPoints[idx++] = center + Vector2i(j, -i);
			for (int j = -i; j < i; ++j)
				alphaBetaPoints[idx++] = center + Vector2i(-i, j);
		}
	}

	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera(protoGL.GetCanvas(), CameraOrtho2D::Upward, CameraListener_None);
	camera.SetCamera(Vector2f(tableSize*0.5f, tableSize*0.5f), tableSize*1.25f);

	TableState tableState;
	TableMarks tableMarks;
	tableState.fill(State::Empty);
	tableMarks.fill(0);

	State currentPlayer = State::Black;
	bool lbuttonWasDown = false;

	State finishedState = State::Empty;
	uint scoreBlack = 0;
	uint scoreWhite = 0;

	while (protoGL.Update())
	{
		/*
		const bool isPlayer = (currentPlayer == State::Black);
		const bool isAI = (currentPlayer == State::White);
		*/
		
		const bool isPlayer = false;
		const bool isAI = currentPlayer != State::Empty;
		
		bool isValidSpot = false;
		Vector2i cursorPos;

		if (isPlayer)
		{
			const Vector2f mousePos = camera.CursorPosition();
			cursorPos = Vector2i(uint(mousePos.x + 0.5f), uint(mousePos.y + 0.5f));
			isValidSpot = (finishedState == State::Empty) && IsValidSpot(tableState, cursorPos);

			const bool lbuttonDown = GetAsyncKeyState(VK_LBUTTON) & 0x8000;
			if (!lbuttonDown)
			{
				lbuttonWasDown = false;
			}
			else if (finishedState == State::Empty && lbuttonDown && !lbuttonWasDown)
			{
				lbuttonWasDown = true;
				if (isValidSpot)
				{
					tableState[cursorPos.x + cursorPos.y*tableSize] = currentPlayer;
					currentPlayer = OtherPlayer(currentPlayer);
				}
			}
		}
		else if (isAI)
		{
			const auto play = AlphaBeta(alphaBetaPoints, tableState, 3, -infinity, infinity, currentPlayer, true);
			tableState[play.position.x + play.position.y*tableSize] = currentPlayer;
			currentPlayer = OtherPlayer(currentPlayer);
		}

		finishedState = FinishedStateAndMArk(tableState, &tableMarks);
		if (finishedState != State::Empty)
			currentPlayer = State::Empty;

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera.CommitToGL();

		DrawTable(tableState, tableMarks, currentPlayer, cursorPos, isPlayer && isValidSpot);

		protoGL.Swap();
	}
}
#endif



class TaskManager
{
public:
	typedef std::function<void()> Task;
	typedef std::deque<Task> Tasks;

public:
	TaskManager(uint numThreads)
		: threads(numThreads)
	{
		int threadId = 0;
		for (auto& thread : threads)
		{
			thread = std::thread([threadId, this]{ this->TaskLoop(threadId); });
			++threadId;
		}
	}

	~TaskManager()
	{
		WaitForAll();
		running = false;
		newTaskNotify.notify_all();
		for (auto& thread : threads)
			thread.join();
	}

	void AddTask(Task task)
	{
		{
			std::lock_guard<std::mutex> guard(tasksMutex);
			tasks.push_back(task);
		}
		newTaskNotify.notify_one();
	}

	void WaitForAll()
	{
		while (HasTasksLeft())
		{
			std::unique_lock<std::mutex> removedTaskLoc(removedTaskMutex);
			removedTaskNotify.wait(removedTaskLoc);
		}
	}

private:
	bool HasTasksLeft() const
	{
		std::lock_guard<std::mutex> guard(tasksMutex);
		return !tasks.empty();
	}

	Task PopTask()
	{
		std::lock_guard<std::mutex> guard(tasksMutex);
		Task task = tasks.front();
		tasks.pop_front();
		removedTaskNotify.notify_all();
		return task;
	}

	void TaskLoop(uint threadId)
	{
		while (running)
		{
			if (HasTasksLeft())
			{
				Task task = PopTask();
				task();
			}
			else
			{
				std::unique_lock<std::mutex> newTaskLoc(newTaskMutex);
				newTaskNotify.wait(newTaskLoc);
			}
		}
	}

	std::vector<std::thread> threads;
	Tasks tasks;

	mutable std::mutex tasksMutex;
	mutable std::mutex newTaskMutex;
	mutable std::mutex removedTaskMutex;
	std::condition_variable newTaskNotify;
	std::condition_variable removedTaskNotify;
	std::atomic<bool> running = true;
};


void Gomoku()
{
	/*
	{
		const uint numCpus = std::thread::hardware_concurrency();
		::TaskManager taskManager{ numCpus };

		const uint numPlays = 100;
		std::cout << "Match" << std::endl;
		for (uint i = 0; i < numPlays; ++i)
		{
			taskManager.AddTask([i](){
				{
					std::lock_guard<std::mutex> guard(coutMtx);
					std::cout << "starting match " << i << std::endl;
				}
				MatchResult result = GomokuMatch();
				OutputResult(result, i);
			});
		}
	}
	*/

	/*
	{
		const uint numCpus = std::thread::hardware_concurrency();
		::TaskManager taskManager{ numCpus };

		const uint numPlayers = 50;
		std::vector<NeuralPlayer> players;
		players.resize(numPlayers);
		std::cout << "Match" << std::endl;
		for (uint i = 0; i < numPlayers; ++i)
		{
			for (uint j = numPlayers + 1; j < numPlayers; ++j)
			{
				taskManager.AddTask([&players, i, j]() {
					{
						std::lock_guard<std::mutex> guard(coutMtx);
						std::cout << "starting match " << i << std::endl;
					}
					MatchResult result = GomokuMatch(&players[i], &players[j]);
					OutputResult(result, i);
				});
			}
		}
	}
	*/
	
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera(protoGL.GetCanvas(), CameraOrtho2D::Upward, CameraListener_None);
	camera.SetCamera(Vector2f(tableSize*0.5f, tableSize*0.5f), tableSize*1.25f);

	TableState tableState;
	TableMarks tableMarks;
	tableState.fill(State::Empty);
	tableState[tableSize / 2 + tableSize / 2 * tableSize] = State::White;
	tableMarks.fill(0);

	State finishedState = State::Empty;

//	HumanPlayer player0{ State::Black, camera };
//	NeuralPlayer player0{ State::Black };
	AlphaBetaPlayer player0{ State::Black };
	AlphaBetaPlayer player1{ State::White };
//	NeuralPlayer player1{ State::White };
	Player* currentPlayer = &player0;
	Player* otherPlayer = &player1;

	while (protoGL.Update())
	{
		if (finishedState == State::Empty)
		{
			if (currentPlayer->Update(&tableState))
			{
				std::swap(currentPlayer, otherPlayer);
				finishedState = FinishedStateAndMArk(tableState, &tableMarks);
			}
		}

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera.CommitToGL();

		currentPlayer->Draw(tableState, tableMarks);

		protoGL.Swap();
	}
}
