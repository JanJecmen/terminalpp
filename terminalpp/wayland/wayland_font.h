#pragma once
#if (defined ARCH_UNIX && defined RENDERER_NATIVE && defined DISPLAY_WAYLAND)

#include <unordered_map>

#include "helpers/helpers.h"

#include "wayland.h"

#include "../font.h"
#include "wayland_application.h"

#include "../config.h"

namespace tpp {

class FTFont {};

class WaylandFont : public Font<WaylandFont> {
  public:
    ~WaylandFont() override { FcPatternDestroy(pattern_); }

    bool supportsCodepoint(char32_t codepoint) {
        MARK_AS_UNUSED(codepoint);
        return false;
    }

  private:
    friend class Font<WaylandFont>;

    WaylandFont(ui::Font font, int cellHeight, int cellWidth = 0);

    WaylandFont(WaylandFont const& base, char32_t codepoint);

    void initializeFromPattern();

    FcPattern* pattern_;

    static std::unordered_map<FTFont*, unsigned> ActiveFontsMap_;
};

} // namespace tpp

#endif
