#include "GameSession.h"
#include "GameView.h"
#include "Texture.h"

GameView::GameView()
    : mGameSession(nullptr)
    , mTexture(nullptr)
{
}

void GameView::clear()
{
    mGameSession = nullptr;
    mTexture = nullptr;
}

void GameView::setGameSession(GameSession& gameSession, Texture& texture)
{
    mGameSession = &gameSession;
    mTexture = &texture;
}

const char* GameView::getName() const
{
    return "Game";
}

void GameView::onGUI()
{
    if (!mGameSession || !mGameSession->isValid())
        return;

    auto size = ImGui::GetContentRegionAvail();
    uint32_t screenSizeX = static_cast<uint32_t>(size.x);
    uint32_t screenSizeY = static_cast<uint32_t>(size.y);

    uint32_t imageRectW = mTexture->getWidth();
    uint32_t imageRectH = mTexture->getHeight();
    mGameSession->getDisplaySize(imageRectW, imageRectH);

    uint32_t screenRectW = 0;
    uint32_t screenRectH = 0;
    uint32_t screenRectX = 0;
    uint32_t screenRectY = 0;
    uint32_t scaledX = screenSizeY * imageRectW / imageRectH;
    if (screenSizeX > scaledX)
    {
        screenRectW = scaledX;
        screenRectH = screenSizeY;
        screenRectX = (screenSizeX - screenRectW + 1) >> 1;
    }
    else
    {
        screenRectW = screenSizeX;
        screenRectH = screenSizeX * imageRectH / imageRectW;
        screenRectY = (screenSizeY - screenRectH + 1) >> 1;
    }

    ImGui::Image(
        mTexture->getImTextureID(),
        ImVec2(static_cast<float>(screenRectW), static_cast<float>(screenRectH)),
        ImVec2(0, 0),
        ImVec2(static_cast<float>(imageRectW) / mTexture->getWidth(), static_cast<float>(imageRectH) / mTexture->getHeight()));
}
