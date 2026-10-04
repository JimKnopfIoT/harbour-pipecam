/*
 * YUYV variant: completed bulk transfers -> frames. No Qt, no libusb.
 *   Frame boundary: transfer ending in a short packet that starts DD CC (header).
 *   Header precedes its frame. Frame size from byte count, see frameSizeForBytes().
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef YUYVASSEMBLER_H
#define YUYVASSEMBLER_H

#include <cstddef>
#include <vector>

namespace upp {
namespace yuyv {

static const int PKT_SIZE = 512;
static const unsigned char HDR_MAGIC_0 = 0xDD;
static const unsigned char HDR_MAGIC_1 = 0xCC;
/* 1920x1080x2 < 4 MiB */
static const size_t MAX_FRAME_BYTES = 4u * 1024u * 1024u;

/* Distinct 2*w*h each. */
inline bool frameSizeForBytes(size_t n, int *w, int *h)
{
    static const int sizes[][2] = {
        { 640, 480 }, { 320, 240 }, { 1280, 720 }, { 640, 360 }, { 800, 600 },
        { 1024, 768 }, { 1920, 1080 }, { 352, 288 }, { 176, 144 }, { 160, 120 },
    };
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        if (n == size_t(sizes[i][0]) * size_t(sizes[i][1]) * 2u) {
            *w = sizes[i][0];
            *h = sizes[i][1];
            return true;
        }
    }
    return false;
}

class Assembler
{
public:
    enum Result {
        Nothing,
        FrameReady,     /* frame(), width(), height(), frameHeader() */
        FrameDropped,   /* droppedBytes(): no known size, or no header before */
        Overflow        /* no header within MAX_FRAME_BYTES */
    };

    Assembler() : m_haveHeader(false), m_lastHadHeader(false), m_w(0), m_h(0), m_dropped(0), m_strayShort(0) {}

    Result feed(const unsigned char *p, int n)
    {
        if (n <= 0)
            return Nothing;
        const int tail = n % PKT_SIZE;
        const bool hdr = tail >= 2 && p[n - tail] == HDR_MAGIC_0 && p[n - tail + 1] == HDR_MAGIC_1;
        const int data = hdr ? n - tail : n;
        m_lastHadHeader = hdr;
        if (tail && !hdr)
            ++m_strayShort;

        if (m_acc.size() + size_t(data) > MAX_FRAME_BYTES) {
            m_dropped = m_acc.size() + size_t(data);
            m_acc.clear();
            m_haveHeader = false;
            return Overflow;
        }
        m_acc.insert(m_acc.end(), p, p + data);
        if (!hdr)
            return Nothing;

        Result r = Nothing;
        if (!m_acc.empty()) {
            if (m_haveHeader && frameSizeForBytes(m_acc.size(), &m_w, &m_h)) {
                m_frame.swap(m_acc);
                m_frameHeader.swap(m_header);
                r = FrameReady;
            } else {
                m_dropped = m_acc.size();
                r = FrameDropped;
            }
        }
        m_acc.clear();
        m_header.assign(p + data, p + n);
        m_haveHeader = true;
        return r;
    }

    const std::vector<unsigned char> &frame() const { return m_frame; }
    /* Last feed() ended in a header; header() holds it. */
    bool lastHadHeader() const { return m_lastHadHeader; }
    const std::vector<unsigned char> &header() const { return m_header; }
    const std::vector<unsigned char> &frameHeader() const { return m_frameHeader; }
    /* Callers may take the buffer; next FrameReady refills it. */
    std::vector<unsigned char> &frame() { return m_frame; }
    int width() const { return m_w; }
    int height() const { return m_h; }
    size_t droppedBytes() const { return m_dropped; }
    /* Short packets without DD CC: appended as data. */
    int strayShortPackets() const { return m_strayShort; }

private:
    std::vector<unsigned char> m_acc;
    std::vector<unsigned char> m_frame;
    std::vector<unsigned char> m_header;
    std::vector<unsigned char> m_frameHeader;
    bool m_haveHeader;
    bool m_lastHadHeader;
    int m_w, m_h;
    size_t m_dropped;
    int m_strayShort;
};

} // namespace yuyv
} // namespace upp

#endif // YUYVASSEMBLER_H
