#pragma once
#include "display.h"

#include "SDL2/SDL.h"

class SDLDisplay : public Display
{
public:
    SDLDisplay();
    void init(uint16_t width, uint16_t height) override;
    void close() override;
    void update() override;
    void drawPixel(uint16_t x, uint16_t y, uint8_t color) override;

private:
    SDL_Renderer *renderer_;
    SDL_Window *window_;
};

