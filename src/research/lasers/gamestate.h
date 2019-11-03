#pragma once

#include "vidf/proto/text.h"


namespace lasers
{

using namespace vidf;
using namespace proto;



class GameState;
typedef std::shared_ptr<GameState> GameStatePtr;
typedef std::weak_ptr<GameState> GameStateRef;



class GameState : public std::enable_shared_from_this<GameState>, public CanvasListener
{
public:
	typedef std::function<bool(Time)> ActiveAction;
	typedef std::function<void()> LatentAction;

public:
	GameState(ProtoGL& _graphics, Text& _text);
	virtual ~GameState();
	void Update(Time deltaTime);
	void SetSubState(GameStatePtr _subState);
	void RemoveSubState();
	void FinishState();
	bool HasSubState() const { return subState || (changeSubState && nextSubState); }
	GameStatePtr GetParent() const { return parent.lock(); }
	void Draw();

protected:
	virtual void StateUpdate(Time deltaTime) {}
	virtual void StateDraw() {}
	void AddActiveAction(ActiveAction action) { activeActions.push_back(action); }
	void AddLatentAction(float deltaTime, LatentAction action) { latentActions.emplace_back(deltaTime, action); }
	ProtoGL& GetGraphics() { return graphics; }
	Text&    GetText() { return text; }

private:
	ProtoGL&     graphics;
	Text&        text;
	GameStatePtr subState;
	GameStatePtr nextSubState;
	GameStateRef parent;
	std::list<ActiveAction> activeActions;
	std::list<std::pair<float, LatentAction>> latentActions;
	bool changeSubState = false;
};




}
