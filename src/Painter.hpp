#pragma once
#include <Geode/Geode.hpp>
#include <set>
#include <string>
#include <vector>

namespace r387 {

// Shape of the selected blocks, one char per 30x30 cell, rows top -> bottom.
struct Mask {
    int x0 = 0, y0 = 0, w = 0, h = 0, cells = 0;
    std::vector<std::string> rows;
    std::string err;
};

// Result from the server. Rows use '.' for empty, else a base-32 palette index.
struct Art {
    std::string id;
    std::vector<cocos2d::ccColor3B> palette;
    std::vector<std::string> rows;
};

inline Mask maskFrom(cocos2d::CCArray* objs) {
    using namespace geode::prelude;
    Mask m;
    if (!objs || objs->count() == 0) { m.err = "Select some blocks in the editor first."; return m; }
    std::set<std::pair<int, int>> cells;
    for (auto obj : CCArrayExt<GameObject*>(objs)) {
        auto r = obj->getObjectRect();
        int cx0 = (int)std::floor((r.getMinX() + 1.f) / 30.f);
        int cx1 = (int)std::floor((r.getMaxX() - 1.f) / 30.f);
        int cy0 = (int)std::floor((r.getMinY() + 1.f) / 30.f);
        int cy1 = (int)std::floor((r.getMaxY() - 1.f) / 30.f);
        cx1 = std::max(cx0, cx1); cy1 = std::max(cy0, cy1);
        for (int x = cx0; x <= cx1; x++) for (int y = cy0; y <= cy1; y++) cells.insert({x, y});
    }
    int minX = 1 << 30, minY = 1 << 30, maxX = -(1 << 30), maxY = -(1 << 30);
    for (auto& [x, y] : cells) {
        minX = std::min(minX, x); maxX = std::max(maxX, x);
        minY = std::min(minY, y); maxY = std::max(maxY, y);
    }
    m.x0 = minX; m.y0 = minY; m.w = maxX - minX + 1; m.h = maxY - minY + 1; m.cells = (int)cells.size();
    if (m.w > 64 || m.h > 64) { m.err = "Selection is too big (max 64x64 blocks)."; return m; }
    m.rows.assign(m.h, std::string(m.w, '0'));
    for (auto& [x, y] : cells) m.rows[maxY - y][x - minX] = '1';
    return m;
}

// Mask enlarged to `res` pixels per block side.
inline std::vector<std::string> expand(Mask const& m, int res) {
    std::vector<std::string> out;
    for (auto& row : m.rows) {
        std::string line;
        for (char c : row) line.append(res, c);
        for (int i = 0; i < res; i++) out.push_back(line);
    }
    return out;
}

inline int paletteIndex(char ch, size_t n) {
    if (ch == '.') return -1;
    int v = (ch >= '0' && ch <= '9') ? ch - '0' : (ch >= 'a' && ch <= 'v') ? ch - 'a' + 10 : -1;
    return (v >= 0 && (size_t)v < n) ? v : -1;
}

// Builds a GD object string. Equal-colored pixels are merged into rectangles to save objects.
inline std::string buildObjectString(Mask const& m, Art const& a, int res, int blockId,
                                     int chanBase, int maxObjs, int& objCount, bool& truncated) {
    int W = m.w * res, H = m.h * res;
    auto ex = expand(m, res);
    auto at = [&](int r, int c) -> int {
        if (r >= (int)a.rows.size() || c >= (int)a.rows[r].size()) return -1;
        int v = paletteIndex(a.rows[r][c], a.palette.size());
        return (v >= 0 && ex[r][c] == '1') ? v : -1;
    };
    std::vector<std::vector<char>> used(H, std::vector<char>(W, 0));
    std::string s;
    float px = 30.f / res;
    objCount = 0; truncated = false;
    for (int r = 0; r < H; r++) for (int c = 0; c < W; c++) {
        int v = at(r, c);
        if (v < 0 || used[r][c]) continue;
        int cw = 1;
        while (c + cw < W && !used[r][c + cw] && at(r, c + cw) == v) cw++;
        int rh = 1;
        for (bool ok = true; r + rh < H && ok;) {
            for (int k = 0; k < cw; k++)
                if (used[r + rh][c + k] || at(r + rh, c + k) != v) { ok = false; break; }
            if (ok) rh++;
        }
        for (int i = 0; i < rh; i++) for (int k = 0; k < cw; k++) used[r + i][c + k] = 1;
        if (objCount >= maxObjs) { truncated = true; continue; }
        float x = m.x0 * 30.f + (c + cw / 2.f) * px;
        float y = (m.y0 + m.h) * 30.f - (r + rh / 2.f) * px;
        s += fmt::format("1,{},2,{:.2f},3,{:.2f},128,{:.3f},129,{:.3f},21,{},24,9;",
                         blockId, x, y, cw / (float)res, rh / (float)res, chanBase + v);
        objCount++;
    }
    // One color trigger per palette entry, parked just left of the art.
    for (size_t i = 0; i < a.palette.size(); i++) {
        auto c = a.palette[i];
        s += fmt::format("1,899,2,{:.1f},3,{:.1f},7,{},8,{},9,{},10,0,35,1,23,{};",
                         m.x0 * 30.f - 60.f, m.y0 * 30.f + 15.f + i * 30.f, c.r, c.g, c.b, chanBase + (int)i);
    }
    return s;
}

} // namespace r387
