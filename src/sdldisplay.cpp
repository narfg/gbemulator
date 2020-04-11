#include "sdldisplay.h"

#include <SDL2/SDL.h>

SDLDisplay::SDLDisplay():
    renderer_(nullptr),
    window_(nullptr)
{

}

void SDLDisplay::init(uint16_t width, uint16_t height) {
    //

    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(width, height, 0, &window_, &renderer_);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0);
    SDL_RenderClear(renderer_);
}

void SDLDisplay::close() {
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

void SDLDisplay::update() {
    SDL_Event event;
    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT) {
        exit(EXIT_SUCCESS);
    }
    SDL_RenderPresent(renderer_);
}

void SDLDisplay::drawPixel(uint16_t x, uint16_t y, uint8_t color) {
    switch(color) {
    case 0:
        SDL_SetRenderDrawColor(renderer_, 224, 248, 208, 255);
        break;
    case 1:
        SDL_SetRenderDrawColor(renderer_, 136, 192, 112, 255);
        break;
    case 2:
        SDL_SetRenderDrawColor(renderer_, 52, 104, 86, 255);
        break;
    case 3:
        SDL_SetRenderDrawColor(renderer_, 8, 24, 32, 255);
        break;
    }

    SDL_RenderDrawPoint(renderer_, x, y);
}
