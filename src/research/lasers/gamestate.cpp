#include "pch.h"
#include "gamestate.h"


namespace lasers
{


GameState::GameState(ProtoGL& _graphics, Text& _text)
	: graphics(_graphics)
	, text(_text)
{
	graphics.GetCanvas()->AddListener(this);
}



GameState::~GameState()
{
	graphics.GetCanvas()->RemoveListener(this);
}



void GameState::Update(Time deltaTime)
{
	if (changeSubState)
	{
		std::swap(subState, nextSubState);
		nextSubState.reset();
		changeSubState = false;
	}

	if (subState)
	{
		subState->Update(deltaTime);
	}
	else
	{
		StateUpdate(deltaTime);
		for (auto it = activeActions.begin(); it != activeActions.end(); )
		{
			bool keep = (*it)(deltaTime);
			if (!keep)
			{
				auto thisIt = it;
				++it;
				activeActions.erase(thisIt);
			}
			else
				++it;
		}
		for (auto it = latentActions.begin(); it != latentActions.end(); )
		{
			it->first -= deltaTime.AsFloat();
			if (it->first <= 0.0f)
			{
				auto thisIt = it;
				++it;
				(thisIt->second)();
				latentActions.erase(thisIt);
			}
			else
				++it;
		}
	}
}



void GameState::Draw()
{
	if (subState)
		subState->Draw();
	else
		StateDraw();
}



void GameState::SetSubState(GameStatePtr _subState)
{
	assert(!_subState->parent.lock());
	nextSubState = _subState;
	nextSubState->parent = shared_from_this();
	changeSubState = true;
}



void GameState::RemoveSubState()
{
	nextSubState.reset();
	changeSubState = true;
}



void GameState::FinishState()
{
	assert(parent.lock());
	assert(parent.lock()->subState == shared_from_this());
	parent.lock()->RemoveSubState();
}



}
