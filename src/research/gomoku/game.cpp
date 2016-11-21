#include "pch.h"
#include "vidf/common/random.h"

using namespace vidf;
using namespace proto;


namespace
{

	const uint tableSize = 15;



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


}


void Gomoku()
{
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

	auto IsValidSpot = [](const TableState& tableState, Vector2i position)
	{
		if (position.x < 0 || position.y < 0)
			return false;
		if (position.x >= tableSize || position.y >= tableSize)
			return false;
		return tableState[position.x + position.y*tableSize] == State::Empty;
	};

	State finishedState = State::Empty;
	uint scoreBlack = 0;
	uint scoreWhite = 0;

	while (protoGL.Update())
	{
		
		const bool isPlayer = (currentPlayer == State::Black);
		const bool isAI = (currentPlayer == State::White);
		
		/*
		const bool isPlayer = false;
		const bool isAI = currentPlayer != State::Empty;
		*/
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
			if (!(play.position != Vector2i(-1, -1)))
				__debugbreak();
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
