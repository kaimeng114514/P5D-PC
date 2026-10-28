// cpk_reader.h — CRIWARE CPK virtual filesystem + CRILAYLA decompressor.
// Header-only, ported byte-for-byte from the VERIFIED prototype
// tools/cpk_tools/cpk_extract_prototype.py (calibrated on P5D data.cpk).
//
// Reads the CPK by seeking on-disk (the 1.9 GB data.cpk is never loaded whole).
// Big-endian @UTF tables; storage flags decide which columns consume row bytes;
// physical offset = FileOffset + min(TocOffset, ContentOffset); compressed
// members start with "CRILAYLA" and are decoded by the backward LZ below.
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cwchar>
#endif

namespace p5d::fs {

// Open a binary file from a UTF-8 path. On Windows the ANSI fopen mangles
// non-ASCII (e.g. CJK user) paths, so convert to UTF-16 and use _wfopen.
inline std::FILE* fopen_utf8(const std::string& path) {
#if defined(_WIN32)
    int n = MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, nullptr, 0);
    if (n <= 0) return nullptr;
    std::wstring wp(size_t(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, path.c_str(), -1, &wp[0], n);
    return _wfopen(wp.c_str(), L"rb");
#else
    return std::fopen(path.c_str(), "rb");
#endif
}

inline uint16_t be16(const uint8_t* p) { return uint16_t((p[0] << 8) | p[1]); }
inline uint32_t be32(const uint8_t* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
inline uint64_t be64(const uint8_t* p) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v = (v << 8) | p[i];
    return v;
}
inline uint32_t le32(const uint8_t* p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

// 18.4248: 64-bit file seek. On Windows long is 32-bit and vita.cpk is 2.17 GB,
// so std::fseek(fp, long(off)) silently fails for offsets above 2^31.
inline int cpk_seek(std::FILE* f, uint64_t off) {
#if defined(_WIN32)
    return _fseeki64(f, (__int64)off, SEEK_SET);
#else
    return fseeko(f, (off_t)off, SEEK_SET);
#endif
}

struct CpkEntry {
    uint64_t offset = 0;     // TOC FileOffset (relative to add_offset)
    uint32_t size = 0;       // stored (compressed) size
    uint32_t extract = 0;    // uncompressed size (== uncompressed_size + 0x100)
    uint32_t id = 0;
    std::string dir, name;
};

struct UtfColumn { const char* name; uint8_t type; uint8_t storage; };

// CRILAYLA backward LZ. `in` begins at the "CRILAYLA" magic.
inline std::vector<uint8_t> crilayla_decompress(const std::vector<uint8_t>& in) {
    const uint8_t* ip = in.data();
    uint32_t usz = le32(ip + 8);
    uint32_t hoff = le32(ip + 0xC);
    std::vector<uint8_t> out(size_t(usz) + 0x100, 0);
    // verbatim 0x100 header
    std::memcpy(out.data(), in.data() + hoff + 0x10, 0x100);

    int in_off = int(in.size()) - 0x100 - 1;
    int out_end = 0x100 + int(usz) - 1;
    int produced = 0;
    uint8_t pool = 0;
    int bits_left = 0;
    static const int vle[4] = {2, 3, 5, 8};

    auto get_bits = [&](int n) -> int {
        int r = 0, made = 0;
        while (made < n) {
            if (bits_left == 0) { pool = ip[in_off]; bits_left = 8; --in_off; }
            int take = bits_left <= n - made ? bits_left : n - made;
            r = (r << take) |
                ((pool >> (bits_left - take)) & ((1 << take) - 1));
            bits_left -= take; made += take;
        }
        return r;
    };

    while (produced < int(usz)) {
        if (get_bits(1) > 0) {
            int src = out_end - produced + get_bits(13) + 3;
            int length = 3;
            bool allmax = true;
            for (int lvl = 0; lvl < 4; ++lvl) {
                int t = get_bits(vle[lvl]);
                length += t;
                if (t != (1 << vle[lvl]) - 1) { allmax = false; break; }
            }
            if (allmax) {
                int t;
                do { t = get_bits(8); length += t; } while (t == 255);
            }
            for (int i = 0; i < length; ++i) {
                out[out_end - produced] = out[src];
                --src; ++produced;
            }
        } else {
            out[out_end - produced] = uint8_t(get_bits(8) & 0xFF);
            ++produced;
        }
    }
    return out;
}

class CpkReader {
public:
    bool open(const std::string& path) {
        fp_ = fopen_utf8(path);
        if (!fp_) return false;
        path_ = path;
        if (!parse_header()) { close(); return false; }
        if (!parse_toc()) { close(); return false; }
        return true;
    }
    void close() { if (fp_) { std::fclose(fp_); fp_ = nullptr; } }
    ~CpkReader() { close(); }

    bool contains(const std::string& name) const {
        return index_.count(norm_key(name)) != 0;
    }
    const CpkEntry* locate(const std::string& name) const {
        auto it = index_.find(norm_key(name));
        return it == index_.end() ? nullptr : &it->second;
    }
    uint64_t physical(const CpkEntry& e) const {
        return e.offset + add_offset_;
    }

    // Read + (if needed) decompress one member into `out`.
    bool read_member(const std::string& name, std::vector<uint8_t>& out) {
        const CpkEntry* e = locate(name);
        if (!e) return false;
        return read_entry(*e, out);
    }
    bool read_entry(const CpkEntry& e, std::vector<uint8_t>& out) {
        uint64_t phys = physical(e);
        std::vector<uint8_t> raw(e.size);
        cpk_seek(fp_, phys);
        if (std::fread(raw.data(), 1, raw.size(), fp_) != raw.size()) return false;
        if (raw.size() >= 8 && std::memcmp(raw.data(), "CRILAYLA", 8) == 0) {
            out = crilayla_decompress(raw);
        } else {
            out = std::move(raw);
        }
        return true;
    }

    size_t file_count() const { return index_.size(); }
    const std::unordered_map<std::string, CpkEntry>& entries() const { return index_; }
    uint64_t content_offset() const { return content_offset_; }
    uint64_t toc_offset() const { return toc_offset_; }

    static std::string norm_key(const std::string& s) {
        std::string k = s;
        for (char& c : k) c = char(std::tolower(uint8_t(c)));
        // strip a device prefix such as "data0:" / "host0:"
        size_t colon = k.find(':');
        if (colon != std::string::npos) k.erase(0, colon + 1);
        std::replace(k.begin(), k.end(), '\\', '/');
        while (!k.empty() && k[0] == '/') k.erase(k.begin());
        return k;
    }

private:
    std::FILE* fp_ = nullptr;
    std::string path_;
    std::unordered_map<std::string, CpkEntry> index_;
    uint64_t content_offset_ = 0, toc_offset_ = 0, toc_size_ = 0;
    uint64_t add_offset_ = 0, align_ = 0x200;

    std::vector<uint8_t> read_at(uint64_t off, uint32_t n) {
        std::vector<uint8_t> b(n);
        cpk_seek(fp_, off);
        size_t got = std::fread(b.data(), 1, n, fp_);
        b.resize(got);
        return b;
    }

    // Parse one @UTF table in `blk` starting at `start`; returns rows of
    // column-name -> value. Integer values stored as uint64; strings as the
    // C-string text; blobs as a sentinel string. Storage rules honored.
    struct Cell {
        uint8_t type = 0;
        uint64_t num = 0;
        std::string str;
        bool is_str = false;
    };
    bool parse_utf(const std::vector<uint8_t>& blk, size_t start,
                   std::vector<std::unordered_map<std::string, Cell>>& rows) {
        if (start + 0x20 > blk.size() ||
            std::memcmp(&blk[start], "@UTF", 4) != 0) return false;
        const uint8_t* b = blk.data();
        uint32_t rows_f = be32(b + start + 0x08);
        uint32_t str_f = be32(b + start + 0x0C);
        uint32_t dat_f = be32(b + start + 0x10);
        uint16_t ncol = be16(b + start + 0x18);
        uint16_t rowlen = be16(b + start + 0x1A);
        uint32_t nrow = be32(b + start + 0x1C);
        size_t rows_a = size_t(rows_f) + start + 8;
        size_t str_a = size_t(str_f) + start + 8;
        size_t dat_a = size_t(dat_f) + start + 8;
        auto cstr = [&](size_t off) -> std::string {
            size_t a = str_a + off;
            size_t e = a;
            while (e < blk.size() && b[e]) ++e;
            return std::string(reinterpret_cast<const char*>(b + a), e - a);
        };
        struct Col { std::string name; uint8_t type, storage; };
        std::vector<Col> cols;
        size_t p = start + 0x20;
        for (uint32_t i = 0; i < ncol; ++i) {
            uint8_t fl = b[p++];
            if (fl == 0) { p += 3; fl = b[p++]; }
            uint32_t no = be32(b + p); p += 4;
            cols.push_back({cstr(no), uint8_t(fl & 0x0F), uint8_t(fl & 0xF0)});
        }
        auto width_of = [](uint8_t t) -> int {
            switch (t) {
                case 0: case 1: return 1;
                case 2: case 3: return 2;
                case 4: case 5: return 4;
                case 6: case 7: return 8;
                case 8: case 0xA: case 0xB: return 4;
                default: return 0;
            }
        };
        rows.clear();
        rows.reserve(nrow);
        for (uint32_t j = 0; j < nrow; ++j) {
            size_t rp = rows_a + size_t(j) * rowlen;
            std::unordered_map<std::string, Cell> row;
            for (const Col& c : cols) {
                Cell cell; cell.type = c.type;
                if (c.storage == 0x10) { cell.num = 0; row[c.name] = cell; continue; }
                if (c.storage == 0x30) { row[c.name] = cell; continue; }
                if (c.storage != 0x50) { row[c.name] = cell; continue; }
                int w = width_of(c.type);
                uint64_t v = 0;
                for (int k = 0; k < w && rp + k < blk.size(); ++k)
                    v = (v << 8) | b[rp + k];
                rp += w;
                if (c.type == 0xA) {
                    cell.is_str = true; cell.str = cstr(size_t(v));
                } else {
                    cell.num = v;
                }
                row[c.name] = cell;
            }
            rows.push_back(std::move(row));
        }
        return true;
    }

    bool parse_header() {
        auto blk = read_at(0, 0x200000);
        std::vector<std::unordered_map<std::string, Cell>> rows;
        if (!parse_utf(blk, 0x10, rows) || rows.empty()) return false;
        auto& h = rows[0];
        auto num = [&](const char* k) -> uint64_t {
            auto it = h.find(k); return it == h.end() ? 0 : it->second.num;
        };
        content_offset_ = num("ContentOffset");
        toc_offset_ = num("TocOffset");
        toc_size_ = num("TocSize");
        align_ = num("Align") ? num("Align") : 0x800;
        add_offset_ = std::min(toc_offset_, content_offset_);
        return toc_offset_ != 0;
    }

    bool parse_toc() {
        auto blk = read_at(toc_offset_, uint32_t(toc_size_ + 0x10000));
        size_t u = 0;
        for (; u + 4 <= blk.size(); ++u)
            if (!std::memcmp(&blk[u], "@UTF", 4)) break;
        if (u >= blk.size()) return false;
        std::vector<std::unordered_map<std::string, Cell>> rows;
        if (!parse_utf(blk, u, rows)) return false;
        for (auto& r : rows) {
            auto getn = [&](const char* k) -> uint64_t {
                auto it = r.find(k); return it == r.end() ? 0 : it->second.num;
            };
            auto gets = [&](const char* k) -> std::string {
                auto it = r.find(k);
                return (it == r.end() || !it->second.is_str) ? std::string() : it->second.str;
            };
            CpkEntry e;
            e.dir = gets("DirName");
            e.name = gets("FileName");
            if (e.name.empty()) continue;
            e.size = uint32_t(getn("FileSize"));
            e.extract = uint32_t(getn("ExtractSize"));
            e.offset = getn("FileOffset");
            e.id = uint32_t(getn("ID"));
            std::string key = norm_key(e.dir + "/" + e.name);
            index_[key] = std::move(e);
        }
        return !index_.empty();
    }
};

} // namespace p5d::fs
