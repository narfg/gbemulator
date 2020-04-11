#include "joypad.h"

Joypad::Joypad():
    buttons_(0xCF),
    directions_(0xCF)
{

}

void Joypad::setDown(bool down) {
    if (down) {
        directions_ &= ~(0x08);
    } else {
        directions_ |= 0x08;
    }
}

void Joypad::setUp(bool down) {
    if (down) {
        directions_ &= ~(0x04);
    } else {
        directions_ |= 0x04;
    }
}

void Joypad::setLeft(bool down) {
    if (down) {
        directions_ &= ~(0x02);
    } else {
        directions_ |= 0x02;
    }
}

void Joypad::setRight(bool down) {
    if (down) {
        directions_ &= ~(0x01);
    } else {
        directions_ |= 0x01;
    }
}

void Joypad::setStart(bool down) {
    if (down) {
        buttons_ &= ~(0x08);
    } else {
        buttons_ |= 0x08;
    }
}

void Joypad::setSelect(bool down) {
    if (down) {
        buttons_ &= ~(0x04);
    } else {
        buttons_ |= 0x04;
    }
}

void Joypad::setB(bool down) {
    if (down) {
        buttons_ &= ~(0x02);
    } else {
        buttons_ |= 0x02;
    }
}

void Joypad::setA(bool down) {
    if (down) {
        buttons_ &= ~(0x01);
    } else {
        buttons_ |= 0x01;
    }
}
