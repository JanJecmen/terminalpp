#if (defined ARCH_LINUX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)
#include <cmath>

#include "wayland_font.h"

namespace tpp {

std::unordered_map<FTFont*, unsigned> WaylandFont::ActiveFontsMap_;

WaylandFont::WaylandFont(ui::Font font, int cellHeight, int cellWidth)
    : Font<WaylandFont>{font, ui::Size{cellWidth, cellHeight}} {
    // update the cell size accordingly
    // get the font pattern
    pattern_ = FcPatternCreate();
    FcPatternAddBool(pattern_, FC_SCALABLE, FcTrue);
    FcPatternAddString(
        pattern_, FC_FAMILY,
        pointer_cast<FcChar8 const*>(
            tpp::Config::Instance().familyForFont(font).c_str()));
    FcPatternAddInteger(pattern_, FC_WEIGHT,
                        font.bold() ? FC_WEIGHT_BOLD : FC_WEIGHT_NORMAL);
    FcPatternAddInteger(pattern_, FC_SLANT,
                        font.italic() ? FC_SLANT_ITALIC : FC_SLANT_ROMAN);
    FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, fontSize_.height());
    initializeFromPattern();
}

WaylandFont::WaylandFont(WaylandFont const& base, char32_t codepoint)
    : Font<WaylandFont>{base.font_, base.fontSize_} {
    // get the font pattern
    pattern_ = FcPatternDuplicate(base.pattern_);
    FcPatternRemove(pattern_, FC_FAMILY, 0);
    FcPatternRemove(pattern_, FC_PIXEL_SIZE, 0);
    FcPatternAddDouble(pattern_, FC_PIXEL_SIZE, fontSize_.height());
    FcCharSet* charSet = FcCharSetCreate();
    FcCharSetAddChar(charSet, codepoint);
    FcPatternAddCharSet(pattern_, FC_CHARSET, charSet);
    initializeFromPattern();
}

void WaylandFont::initializeFromPattern() {}

} // namespace tpp

#endif
