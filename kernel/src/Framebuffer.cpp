#include "Framebuffer.h"
#include "Stivale2Interface.h"
#include "Serial.h"
#include "Assert.h"
#include "Memory/Memory.h"

uint32_t* Framebuffer::virtAddr = nullptr;
uint16_t Framebuffer::width = 0;
uint16_t Framebuffer::height = 0;
uint8_t Framebuffer::redShift = 0;
uint8_t Framebuffer::blueShift = 0;
uint8_t Framebuffer::greenShift = 0;

void Framebuffer::Initialize()
{
    auto framebufferStruct = (stivale2_struct_tag_framebuffer*)GetStivale2Tag(STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);

    Assert(framebufferStruct->memory_model == 1);
    Assert(framebufferStruct->framebuffer_bpp == 32);

    Serial::Log("Framebuffer width: %d", framebufferStruct->framebuffer_width);
    Serial::Log("Framebuffer height: %d", framebufferStruct->framebuffer_height);
    Serial::Log("Framebuffer pitch: %d", framebufferStruct->framebuffer_pitch);

    virtAddr = (uint32_t*)framebufferStruct->framebuffer_addr;
    width = framebufferStruct->framebuffer_width;
    height = framebufferStruct->framebuffer_height;
    redShift = framebufferStruct->red_mask_shift;
    greenShift = framebufferStruct->green_mask_shift;
    blueShift = framebufferStruct->blue_mask_shift;

    memset(virtAddr, 0, width * height * sizeof(uint32_t));
}

void Framebuffer::PlotPixel(unsigned int x, unsigned int y, Colour colour)
{
    Assert(x < width);
    Assert(y < height);
    uint32_t* addr = virtAddr + (y * width) + x;
    uint32_t rgb = colour.red << redShift | colour.green << greenShift | colour.blue << blueShift;
    *addr = rgb;
}

void Framebuffer::TranslateVertical(long deltaY, const Colour& fillColour)
{
    Assert(deltaY > 0);

    long deltaPixels = deltaY * width;
    uint32_t* source = virtAddr + deltaPixels;
    uint32_t* destination = virtAddr;
    for (long i = 0; i < width * height - deltaPixels; ++i)
    {
        destination[i] = source[i];
    }

    uint32_t* fillStart = virtAddr + width * height - deltaPixels - 1;
    for (long i = 0; i < deltaPixels; ++i)
    {
        fillStart[i] = fillColour.red << redShift | fillColour.green << greenShift | fillColour.blue << blueShift;
    }
}

long Framebuffer::Width()
{
    return width;
}

long Framebuffer::Height()
{
    return height;
}