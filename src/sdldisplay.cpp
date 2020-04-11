#include "sdldisplay.h"

#include <SDL2/SDL.h>

#include "joypad.h"

SDLDisplay::SDLDisplay(uint8_t* ram, Joypad* joypad):
    ram_(ram),
    joypad_(joypad),
    renderer_(nullptr),
    window_(nullptr)
{

}

void SDLDisplay::init(uint16_t width, uint16_t height) {
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
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            exit(EXIT_SUCCESS);
        }
        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
            case SDLK_RIGHT:
                printf("right down\n");
                joypad_->setRight(true);
                ram_[0xFF0F] |= (1 << 4);
                break;
            case SDLK_a:
                printf("a down\n");
                joypad_->setA(true);
                ram_[0xFF0F] |= (1 << 4);
                break;
            case SDLK_LEFT:
                printf("left down\n");
                joypad_->setLeft(true);
                ram_[0xFF0F] |= (1 << 4);
                break;
            case SDLK_b:
                printf("b down\n");
                joypad_->setB(true);
                ram_[0xFF0F] |= (1 << 4);
                break;
            case SDLK_UP:
                printf("up down\n");
                joypad_->setUp(true);
                ram_[0xFF0F] |= (1 << 4);
                break;
            case SDLK_s:
                printf("select down\n");
                joypad_->setSelect(true);
                ram_[0xFF0F] |= (1 << 4);
                break;
            case SDLK_DOWN:
                printf("down down\n");
                joypad_->setDown(true);
                ram_[0xFF0F] |= (1 << 4);
                break;
            case SDLK_RETURN:
                printf("start down\n");
                joypad_->setStart(true);
                ram_[0xFF0F] |= (1 << 4);
                break;
            }
        }
        if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
            case SDLK_RIGHT:
                printf("right up\n");
                joypad_->setRight(false);
                break;
            case SDLK_a:
                printf("a up\n");
                joypad_->setA(false);
                break;
            case SDLK_LEFT:
                printf("left up\n");
                joypad_->setLeft(false);
                break;
            case SDLK_b:
                printf("b up\n");
                joypad_->setB(false);
                break;
            case SDLK_UP:
                printf("up up\n");
                joypad_->setUp(false);
                break;
            case SDLK_s:
                printf("select up\n");
                joypad_->setSelect(false);
                break;
            case SDLK_DOWN:
                printf("down up\n");
                joypad_->setDown(false);
                break;
            case SDLK_RETURN:
                printf("start up\n");
                joypad_->setStart(false);
                break;
            }
        }
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
