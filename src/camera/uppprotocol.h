/*
 * uppprotocol.h — wire format of the "com.useeplus.protocol" USB endoscope
 * (Geek szitman / "supercamera", VID:PID 2ce3:3828, also sold as 0329:2022).
 *
 * This header is the single source of truth for the protocol. It deliberately
 * carries the full description rather than a bare list of constants, because
 * the device is undocumented and this is the only place the knowledge lives.
 * The reference implementation that validated every value below is
 * the reference probe described in README.md, verified against the real camera on 2026-08-10:
 * 10/10 frames captured, all decoding as clean 640x480 JPEG.
 *
 * WHY WE TALK USB DIRECTLY
 * ------------------------
 * The device's *device* descriptor is class ef/02/01 (Interface Association),
 * which looks like UVC — but both of its interfaces are vendor-specific
 * (class 0xff, subclass 0xf0). Linux's uvcvideo (built into the Xperia 10 III
 * kernel) therefore never binds it and no /dev/videoN appears. The payload is
 * ordinary MJPEG; only the framing around it is proprietary. So we unwrap it
 * ourselves from userspace with libusb-1.0.
 *
 * USB LAYOUT
 * ----------
 *   Interface 0  "iAP" / control    bulk IN 0x82, bulk OUT 0x02
 *   Interface 1  video stream       bulk IN 0x81, bulk OUT 0x01, alt-setting 1
 * Both interfaces must be claimed; claiming only interface 1 does not stream.
 *
 * HANDSHAKE (order matters)
 * -------------------------
 *   1. claim interface 0 and interface 1
 *   2. drain EP 0x82 — the device queues an unsolicited heartbeat, and a stale
 *      one desynchronises everything that follows
 *   3. set interface 1 to alt-setting 1 (alt 0 has no endpoints)
 *   4. clear_halt on EP 0x01
 *   5. write MAGIC_INIT  to EP 0x02
 *   6. write CONNECT_CMD to EP 0x01
 *   7. read EP 0x81; the first one or two frames are partial — discard them
 *
 * PACKET FORMAT — exactly one logical packet per 1024-byte bulk read
 * -----------------------------------------------------------------
 *   off  size  field
 *   ---  ----  --------------------------------------------------------------
 *    0     2   magic uint16 LE = 0xBBAA (on the wire: AA BB)
 *    2     1   cid — camera id, observed as 7 and 11. BOTH belong to the same
 *              frame; the device alternates them across a frame's chunks.
 *    3     2   length uint16 LE — camera header + payload, counted from off 5
 *              (it does NOT include these five USB-header bytes)
 *    5     1   fid — frame id. THE frame delimiter.
 *    6     1   cam_num
 *    7     1   flags — bit 1 (0x02) = inline push-button on the cable
 *    8     4   g_sensor uint32 LE
 *   12   ...   JPEG payload, up to offset 5+length
 *
 * FRAME REASSEMBLY — the trap everybody falls into
 * ------------------------------------------------
 * Do NOT hunt for FFD8/FFD9 in the byte stream. The device coalesces many ~1 KB
 * packets into one large bulk transfer and EVERY packet carries its own 12-byte
 * header. Stripping only the leading header leaves the interior headers inside
 * the JPEG entropy data: corrupt image, premature FFD9, truncated frame. (This
 * is exactly the "half-grey picture" bug in the PyPI `supercamera` package.)
 *
 * The correct algorithm: request exactly PKT_SIZE per bulk transfer, strip the
 * 12-byte header, append the payload to an accumulator, and close the frame the
 * moment `fid` changes. Then verify SOI/EOI before handing the frame on.
 *
 * STREAM CHARACTERISTICS
 *   640 x 480, ~15.5 fps, ~17-30 KB per JPEG (scene dependent).
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef UPPPROTOCOL_H
#define UPPPROTOCOL_H

#include <stdint.h>

namespace upp {

/* The same hardware ships under two ID pairs. */
struct DeviceId { uint16_t vid; uint16_t pid; };
static const DeviceId KNOWN_DEVICES[] = {
    { 0x2ce3, 0x3828 },
    { 0x0329, 0x2022 },
};
static const int KNOWN_DEVICE_COUNT =
        int(sizeof(KNOWN_DEVICES) / sizeof(KNOWN_DEVICES[0]));

/* Interfaces and endpoints. */
static const int IFACE_IAP      = 0;
static const int IFACE_STREAM   = 1;
static const int STREAM_ALTSETTING = 1;

static const unsigned char EP_IAP_IN     = 0x82;
static const unsigned char EP_IAP_OUT    = 0x02;
static const unsigned char EP_STREAM_IN  = 0x81;
static const unsigned char EP_STREAM_OUT = 0x01;

/* Handshake payloads. */
static const unsigned char MAGIC_INIT[]  = { 0xFF, 0x55, 0xFF, 0x55, 0xEE, 0x10 };
static const int MAGIC_INIT_LEN = 6;
static const unsigned char CONNECT_CMD[] = { 0xBB, 0xAA, 0x05, 0x00, 0x00 };
static const int CONNECT_CMD_LEN = 5;

/* Framing. */
static const int PKT_SIZE       = 0x400;  /* one logical packet per bulk read  */
static const int USB_HDR_LEN    = 5;      /* magic(2) + cid(1) + length(2)     */
static const int PAYLOAD_OFFSET = 12;     /* USB header(5) + camera header(7)  */
static const unsigned char MAGIC_0 = 0xAA;
static const unsigned char MAGIC_1 = 0xBB;
static const unsigned char FLAG_BUTTON = 0x02;

/* Sanity bound: a 640x480 JPEG is ~30 KB; 512 KB is a generous ceiling that
 * still catches a runaway accumulator caused by a desynchronised stream. */
static const int MAX_FRAME_BYTES = 512 * 1024;

/* Frames to discard after CONNECT_CMD — the first one or two are partial. */
static const int WARMUP_FRAMES = 2;

/* Nominal stream geometry (the device has no way to report or change it). */
static const int FRAME_WIDTH  = 640;
static const int FRAME_HEIGHT = 480;

/* Only these camera ids belong to the video stream. */
inline bool cidIsValid(unsigned char cid) { return cid == 7 || cid == 11; }

} // namespace upp

#endif // UPPPROTOCOL_H
