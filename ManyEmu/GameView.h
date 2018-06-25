#pragma once

#include "Application.h"
#include "Graphics.h"

class GameSession;

class GameView : public Application::IView
{
public:
    GameView();
    void clear();
    void setGameSession(GameSession& gameSession, IGraphics& graphics, ITexture& texture, uint32_t texSizeX, uint32_t texSizeY);
    virtual const char* getName() const override;
    virtual void onGUI() override;

private:
    GameSession*            mGameSession;
    IGraphics*              mGraphics;
    ITexture*               mTexture;
    uint32_t                mTexSizeX;
    uint32_t                mTexSizeY;
};
