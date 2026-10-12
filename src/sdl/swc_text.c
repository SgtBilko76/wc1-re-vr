/* SDL replacement for QuickDraw text, shared by the SWC cockpit and rooms. */
#include "swc.h"
#include "swc_hud_font.h"

#include <string.h>
#ifdef __APPLE__
#include <CoreText/CoreText.h>
#include <CoreGraphics/CoreGraphics.h>
#endif

SDL_Texture *SdlCreateSwcFont(SDL_Renderer *renderer)
{
    SDL_Surface *surface;
    SDL_Texture *font;
    uint32_t *row;
    int character;
    int x;
    int y;

    surface = SDL_CreateRGBSurfaceWithFormat(0, 16 * SWC_HUD_FONT_WIDTH,
        6 * SWC_HUD_FONT_HEIGHT, 32, SDL_PIXELFORMAT_ARGB8888);
    if (surface == NULL)
        return NULL;
    SDL_FillRect(surface, NULL, 0);
    for (character = 0; character < 95; character++) {
        for (y = 0; y < SWC_HUD_FONT_HEIGHT; y++) {
            row = (uint32_t *)((uint8_t *)surface->pixels +
                (character / 16 * SWC_HUD_FONT_HEIGHT + y) * surface->pitch);
            for (x = 0; x < SWC_HUD_FONT_WIDTH; x++) {
                if (abSwcHudFont[character][y] & (0x80 >> x))
                    row[character % 16 * SWC_HUD_FONT_WIDTH + x] = UINT32_C(0xffffffff);
            }
        }
    }
    font = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (font != NULL && SDL_SetTextureBlendMode(font, SDL_BLENDMODE_BLEND) != 0) {
        SDL_DestroyTexture(font);
        return NULL;
    }
    return font;
}

int SdlDrawSwcText(SDL_Renderer *renderer, SDL_Texture *font, int x,
                    int baseline, SDL_Color ink, const char *text)
{
    SDL_Rect source = {0, 0, SWC_HUD_FONT_WIDTH, SWC_HUD_FONT_HEIGHT};
    SDL_Rect destination = {x - SWC_HUD_FONT_BEARING,
        baseline - SWC_HUD_FONT_BASELINE, SWC_HUD_FONT_WIDTH, SWC_HUD_FONT_HEIGHT};
    unsigned char character;

    if (SDL_SetTextureColorMod(font, ink.r, ink.g, ink.b) != 0)
        return -1;
    while (*text != 0 && destination.x < SWC_FRAME_WIDTH) {
        character = (unsigned char)*text++;
        if (character < 32 || character > 126)
            character = '?';
        character -= 32;
        source.x = character % 16 * SWC_HUD_FONT_WIDTH;
        source.y = character / 16 * SWC_HUD_FONT_HEIGHT;
        if (SDL_RenderCopy(renderer, font, &source, &destination) != 0)
            return -1;
        destination.x += SWC_HUD_FONT_ADVANCE;
    }
    return 0;
}

int SdlDrawSwcSubtitle(SDL_Renderer *renderer, SDL_Texture *font,
                        const char *text)
{
    char lines[5][61];
    SDL_Color ink = {245, 245, 230, 255};
    SDL_Rect panel;
    size_t length;
    size_t count;
    int line = 0;
    int index;

    while (*text != 0 && line < (int)SDL_arraysize(lines)) {
        while (*text == ' ' || *text == '\n' || *text == '\r')
            text++;
        if (*text == 0)
            break;
        length = strcspn(text, "\r\n");
        count = SDL_min(length, sizeof(lines[0]) - 1);
        if (length > count) {
            while (count > 0 && text[count] != ' ')
                count--;
            if (count == 0)
                count = sizeof(lines[0]) - 1;
        }
        memcpy(lines[line], text, count);
        lines[line++][count] = 0;
        text += count;
    }
    panel = (SDL_Rect){4, 236 - line * 12, 312, line * 12 + 2};
    if (SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255) != 0 ||
        SDL_RenderFillRect(renderer, &panel) != 0)
        return -1;
    for (index = 0; index < line; index++) {
        if (SdlDrawSwcText(renderer, font, 10, panel.y + 9 + index * 12,
                           ink, lines[index]) != 0)
            return -1;
    }
    return 0;
}

/* CODE_06 AllocTextLayer +0x1d9a selects Helvetica (QuickDraw ID 21).
 * CODE_03 print_subtitle +0x1122 selects size 14. The demo contains no
 * FONT/NFNT resources: on macOS use the system face, without bundling it.
 * Other hosts retain the bundled bitmap until a Helvetica provider exists. */
int SdlCreateSwcRoomFont(SDL_Renderer *renderer, SwcRoomFont *font)
{
    int character;
#ifdef __APPLE__
    SDL_Surface *surface;
    CTFontRef face;
    CGColorSpaceRef colorSpace;
    CGContextRef context;
    UniChar code;
    CGGlyph glyph;
    CGSize advance;
    CGPoint origin;

    memset(font, 0, sizeof(*font));
    surface = SDL_CreateRGBSurfaceWithFormat(0, 16 * 24, 6 * 24, 32,
        SDL_PIXELFORMAT_ARGB8888);
    if (surface == NULL)
        return -1;
    SDL_FillRect(surface, NULL, 0);
    face = CTFontCreateWithName(CFSTR("Helvetica"), 14, NULL);
    colorSpace = CGColorSpaceCreateDeviceRGB();
    context = colorSpace == NULL ? NULL : CGBitmapContextCreate(surface->pixels,
        surface->w, surface->h, 8, surface->pitch, colorSpace,
        kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
    if (colorSpace != NULL)
        CGColorSpaceRelease(colorSpace);
    if (face == NULL || context == NULL) {
        if (face != NULL)
            CFRelease(face);
        if (context != NULL)
            CGContextRelease(context);
        SDL_FreeSurface(surface);
        return SDL_SetError("Cannot create SWC Helvetica text atlas");
    }
    CGContextSetShouldAntialias(context, false);
    CGContextSetShouldSmoothFonts(context, false);
    CGContextSetRGBFillColor(context, 1, 1, 1, 1);
    font->baseline = 18;
    font->bearing = 2;
    for (character = 0; character < 95; character++) {
        code = (UniChar)(character + 32);
        CTFontGetGlyphsForCharacters(face, &code, &glyph, 1);
        CTFontGetAdvancesForGlyphs(face, kCTFontOrientationHorizontal, &glyph, &advance, 1);
        font->glyphs[character] = (SDL_Rect){character % 16 * 24,
            character / 16 * 24, 24, 24};
        font->advances[character] = (int)(advance.width + 0.5);
        origin = CGPointMake(font->glyphs[character].x + 2,
            surface->h - font->glyphs[character].y - font->baseline);
        CTFontDrawGlyphs(face, &glyph, &origin, 1, context);
    }
    CGContextRelease(context);
    CFRelease(face);
    font->texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
#else
    memset(font, 0, sizeof(*font));
    font->texture = SdlCreateSwcFont(renderer);
    font->baseline = SWC_HUD_FONT_BASELINE;
    font->bearing = SWC_HUD_FONT_BEARING;
    for (character = 0; character < 95; character++) {
        font->glyphs[character] = (SDL_Rect){character % 16 * SWC_HUD_FONT_WIDTH,
            character / 16 * SWC_HUD_FONT_HEIGHT, SWC_HUD_FONT_WIDTH, SWC_HUD_FONT_HEIGHT};
        font->advances[character] = SWC_HUD_FONT_ADVANCE;
    }
#endif
    if (font->texture == NULL)
        return -1;
    return SDL_SetTextureBlendMode(font->texture, SDL_BLENDMODE_BLEND);
}

/* CODE_03 print_subtitle/GetLineLength and CODE_06 CenterPrint: remove
 * script newlines, wrap at layer width minus 25, center each line, and
 * advance by 15 pixels. init_text gives this layer a 300x200 extent and
 * print_subtitle moves it to (10,120). The Mac draws white text directly
 * over the picture; the record's textColour also identifies the speaker. */
int SdlDrawSwcRoomSubtitle(SDL_Renderer *renderer, const SwcRoomFont *font,
                            const char *text)
{
    char clean[256];
    size_t starts[256];
    size_t ends[256];
    int widths[256];
    size_t length = 0;
    size_t start = 0;
    size_t end;
    size_t space;
    size_t cursor;
    int lines = 0;
    int line;
    int width;
    int wordWidth;
    int character;
    int x;
    int baseline;
    SDL_Rect destination;

    while (*text != 0 && length < sizeof(clean) - 1) {
        if (*text != '\n' && *text != '\r')
            clean[length++] = *text;
        text++;
    }
    clean[length] = 0;
    while (start < length) {
        while (start < length && clean[start] == ' ')
            start++;
        if (start == length)
            break;
        width = wordWidth = 0;
        space = start;
        for (end = start; end < length; end++) {
            character = (unsigned char)clean[end];
            character = character < 32 || character > 126 ? '?' - 32 : character - 32;
            if (clean[end] == ' ') {
                space = end;
                wordWidth = width;
            }
            if (width + font->advances[character] > 275 && end > start)
                break;
            width += font->advances[character];
        }
        if (end < length && space > start) {
            end = space;
            width = wordWidth;
        }
        starts[lines] = start;
        ends[lines] = end;
        widths[lines++] = width;
        start = end;
    }
    baseline = 220 - (14 * lines + 1) / 2;
    /* Keep longer expanded pilot-name lines inside the 240-pixel host
     * picture while retaining the original position for shorter captions. */
    baseline = SDL_min(baseline, 234 - (lines - 1) * 15);
    if (SDL_SetTextureColorMod(font->texture, 255, 255, 255) != 0)
        return -1;
    for (line = 0; line < lines; line++) {
        x = 160 - widths[line] / 2;
        for (cursor = starts[line]; cursor < ends[line]; cursor++) {
            character = (unsigned char)clean[cursor];
            character = character < 32 || character > 126 ? '?' - 32 : character - 32;
            destination = font->glyphs[character];
            destination.x = x - font->bearing;
            destination.y = baseline - font->baseline;
            if (SDL_RenderCopy(renderer, font->texture, &font->glyphs[character], &destination) != 0)
                return -1;
            x += font->advances[character];
        }
        baseline += 15;
    }
    return 0;
}
