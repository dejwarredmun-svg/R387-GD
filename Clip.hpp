#pragma once
#include <Geode/Geode.hpp>
#include <optional>

#ifdef GEODE_IS_WINDOWS
#include <Windows.h>
#endif

namespace r387 {
// Returns a .bmp file's bytes if the clipboard holds a picture (Windows only for now).
inline std::optional<geode::ByteVector> clipboardImage() {
#ifdef GEODE_IS_WINDOWS
    if (!OpenClipboard(nullptr)) return std::nullopt;
    std::optional<geode::ByteVector> out;
    if (HANDLE h = GetClipboardData(CF_DIB)) {
        auto size = GlobalSize(h);
        auto* p = (uint8_t*)GlobalLock(h);
        if (p && size > sizeof(BITMAPINFOHEADER)) {
            auto* bi = (BITMAPINFOHEADER*)p;
            uint32_t off = 14 + bi->biSize + bi->biClrUsed * 4 +
                           ((bi->biCompression == BI_BITFIELDS && bi->biSize == 40) ? 12 : 0);
            uint32_t total = 14 + (uint32_t)size;
            geode::ByteVector v(total);
            v[0] = 'B'; v[1] = 'M';
            memcpy(&v[2], &total, 4);
            memcpy(&v[10], &off, 4);
            memcpy(&v[14], p, size);
            out = std::move(v);
        }
        if (p) GlobalUnlock(h);
    }
    CloseClipboard();
    return out;
#else
    return std::nullopt;
#endif
}
}
