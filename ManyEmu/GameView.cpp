#include "GameSession.h"
#include "GameView.h"

GameView::GameView()
{
    clear();
}

void GameView::clear()
{
    mGameSession = nullptr;
    mGraphics = nullptr;
    mTexture = nullptr;
    mTexSizeX = 0;
    mTexSizeY = 0;
}

void GameView::setGameSession(GameSession& gameSession, IGraphics& graphics, ITexture& texture, uint32_t texSizeX, uint32_t texSizeY)
{
    mGameSession = &gameSession;
    mGraphics = &graphics;
    mTexture = &texture;
    mTexSizeX = texSizeX;
    mTexSizeY = texSizeY;
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

    uint32_t imageRectW = mTexSizeX;
    uint32_t imageRectH = mTexSizeX;
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
        mGraphics->imGuiRenderer_getTextureID(*mTexture),
        ImVec2(static_cast<float>(screenRectW), static_cast<float>(screenRectH)),
        ImVec2(0, 0),
        ImVec2(static_cast<float>(imageRectW) / mTexSizeX, static_cast<float>(imageRectH) / mTexSizeY));
}
