#pragma once

#include "Application.h"

class GameSession;
class Texture;

class GameView : public Application::IView
{
public:
    GameView();
    void clear();
    void setGameSession(GameSession& gameSession, Texture& texture);
    virtual const char* getName() const override;
    virtual void onGUI() override;

private:
    GameSession*            mGameSession;
    Texture*                mTexture;
};
