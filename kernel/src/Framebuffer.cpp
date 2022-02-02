#include "Framebuffer.h"
#include "Stivale2Interface.h"
#include "Serial.h"
#include "Panic.h"
#include "Memory/Memory.h"

Framebuffer* Framebuffer::instance = nullptr;

void Framebuffer::Initialize()
{
    auto framebufferStruct = (stivale2_struct_tag_framebuffer*)GetStivale2Tag(STIVALE2_STRUCT_TAG_FRAMEBUFFER_ID);

    KAssert(framebufferStruct->memory_model == 1, "Framebuffer is not using RGB.");
    KAssert(framebufferStruct->framebuffer_bpp == 32, "Framebuffer is not 32-bit colour.");

    Serial::Printf("Framebuffer width: %d", framebufferStruct->framebuffer_width);
    Serial::Printf("Framebuffer height: %d", framebufferStruct->framebuffer_height);
    Serial::Printf("Framebuffer pitch: %d", framebufferStruct->framebuffer_pitch);

    instance = new Framebuffer();
    instance->virtAddr = (uint32_t*)framebufferStruct->framebuffer_addr;
    instance->width = framebufferStruct->framebuffer_width;
    instance->height = framebufferStruct->framebuffer_height;
    instance->redShift = framebufferStruct->red_mask_shift;
    instance->greenShift = framebufferStruct->green_mask_shift;
    instance->blueShift = framebufferStruct->blue_mask_shift;

    Memset(instance->virtAddr, 0, instance->width * instance->height * sizeof(uint32_t));
}

void Framebuffer::PlotPixel(unsigned int x, unsigned int y, Colour colour)
{
    uint32_t* addr = virtAddr + (y * width) + x;
    uint32_t rgb = colour.red << redShift | colour.green << greenShift | colour.blue << blueShift;
    *addr = rgb;
}