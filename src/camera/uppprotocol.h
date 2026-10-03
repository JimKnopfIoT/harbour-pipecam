/*
 * "com.useeplus.protocol" USB endoscope (Geek szitman / "supercamera"),
 * VID:PID 2ce3:3828 or 0329:2022. Undocumented device; this file is the spec.
 *
 * Interfaces vendor-specific (ff/f0) despite IAD device class ef/02/01:
 * uvcvideo does not bind, no /dev/videoN. Payload MJPEG, framing proprietary.
 *
 * USB layout (MJPEG variant)
 *   iface 0  iAP/control  bulk IN 0x82, OUT 0x02
 *   iface 1  stream       bulk IN 0x81, OUT 0x01, alt 1 (alt 0: no endpoints)
 *   Both interfaces must be claimed, else no stream.
 *
 * Handshake (order matters)
 *   1. claim iface 0 + 1
 *   2. drain EP 0x82 (stale unsolicited heartbeat desyncs the rest)
 *   3. iface 1 -> alt 1
 *   4. clear_halt EP 0x01
 *   5. MAGIC_INIT  -> EP 0x02
 *   6. CONNECT_CMD -> EP 0x01
 *   7. read EP 0x81; first 1-2 frames partial, discard
 *
 * Packet: one per 1024-byte bulk read
 *   off size field
 *    0   2   magic u16 LE 0xBBAA (wire: AA BB)
 *    2   1   cid: always 7 (11 never seen)
 *    3   2   length u16 LE, from off 5 (excludes 5-byte USB header)
 *    5   1   fid: frame id, sole frame delimiter
 *    6   1   cam_num: toggles 0/1 per frame
 *    7   1   flags: 0x02 = cable push-button
 *    8   4   u32 LE, cycles through 4 fixed values; not a g-sensor
 *   12   ..  JPEG payload, up to off 5+length
 *
 * Reassembly: frames delimited by fid change, not FFD8/FFD9. Every packet
 * carries its own 12-byte header, also inside coalesced transfers. Read exactly
 * PKT_SIZE, strip header, append, close frame on fid change, then check SOI/EOI.
 *
 * Stream: 640x480, ~15.5 fps, ~17-30 KB per JPEG.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
#ifndef UPPPROTOCOL_H
#define UPPPROTOCOL_H

#include <stdint.h>

namespace upp {

struct DeviceId { uint16_t vid; uint16_t pid; };
static const DeviceId KNOWN_DEVICES[] = {
    { 0x2ce3, 0x3828 },
    { 0x0329, 0x2022 },
};
static const int KNOWN_DEVICE_COUNT =
        int(sizeof(KNOWN_DEVICES) / sizeof(KNOWN_DEVICES[0]));

/* Firmware variant, from the config descriptor per open. */
enum Variant {
    VariantUnknown,
    VariantMjpeg,   /* two interfaces */
    VariantYuyv     /* one interface, raw 320x240 YUYV */
};

static const int IFACE_IAP      = 0;
static const int IFACE_STREAM   = 1;
static const int STREAM_ALTSETTING = 1;

static const unsigned char EP_IAP_IN     = 0x82;
static const unsigned char EP_IAP_OUT    = 0x02;
static const unsigned char EP_STREAM_IN  = 0x81;
static const unsigned char EP_STREAM_OUT = 0x01;

static const unsigned char MAGIC_INIT[]  = { 0xFF, 0x55, 0xFF, 0x55, 0xEE, 0x10 };
static const int MAGIC_INIT_LEN = 6;
static const unsigned char CONNECT_CMD[] = { 0xBB, 0xAA, 0x05, 0x00, 0x00 };
static const int CONNECT_CMD_LEN = 5;

static const int PKT_SIZE       = 0x400;  /* one packet per bulk read */
static const int USB_HDR_LEN    = 5;      /* magic(2) + cid(1) + length(2) */
static const int PAYLOAD_OFFSET = 12;     /* USB header(5) + camera header(7) */
static const unsigned char MAGIC_0 = 0xAA;
static const unsigned char MAGIC_1 = 0xBB;
static const unsigned char FLAG_BUTTON = 0x02;

/* Desync guard; real frames ~30 KB. */
static const int MAX_FRAME_BYTES = 512 * 1024;

/* Partial frames after CONNECT_CMD. */
static const int WARMUP_FRAMES = 2;

/* Fixed; device cannot report or change it. */
static const int FRAME_WIDTH  = 640;
static const int FRAME_HEIGHT = 480;

inline bool cidIsValid(unsigned char cid) { return cid == 7 || cid == 11; }

/*
 * YUYV variant: same VID:PID and strings (seen: bcdDevice 1.11), other firmware.
 *   iface 0  alt 0, ff/f0/01  bulk IN 0x82, OUT 0x02; no iface 1
 * Claiming iface 1 here fails with LIBUSB_ERROR_INVALID_PARAM (EINVAL from
 * USBDEVFS_DISCONNECT_CLAIM). Detect by descriptors only, never by IDs.
 *
 * Unverified locally; per two public implementations:
 *   https://github.com/Bognabon/Endoscope_Viewer          (Python, MIT)
 *   https://github.com/KlumpRasmus/supercamera-yuyv-linux (V4L2, GPL-2.0+)
 *
 * Handshake: class requests on control pipe, not bulk
 *   1. claim iface 0, clear_halt 0x82 + 0x02
 *   2. getinfo     bmRequestType 0xA0, bRequest 0x00, wValue 0x0005, wIndex 0,
 *                  IN 512 bytes; failure harmless
 *   3. camera_up   0x20 / 0x01 / 0x0005 / 0, OUT 64 zero bytes; wait 200 ms
 *   teardown: camera_down 0x20 / 0x02 / 0x0005 / 0, no data
 *
 * Stream: 320x240 YUY2 (Y0 U Y1 V), one frame per bulk read of FRAME+512 on
 * 0x82. First block after camera_up: 512-byte preamble, then pixels.
 * No header, frame id or button flag.
 */
namespace yuyv {
static const int IFACE   = 0;
static const unsigned char EP_IN  = 0x82;
static const unsigned char EP_OUT = 0x02;

static const uint8_t  REQ_TYPE_IN   = 0xA0;
static const uint8_t  REQ_TYPE_OUT  = 0x20;
static const uint8_t  REQ_GETINFO   = 0x00;
static const uint8_t  REQ_CAMERA_UP = 0x01;
static const uint8_t  REQ_CAMERA_DOWN = 0x02;
static const uint16_t REQ_VALUE     = 0x0005;
static const int GETINFO_LEN   = 512;
static const int CAMERA_UP_LEN = 64;
static const int CAMERA_UP_SETTLE_MS = 200;

static const int FRAME_WIDTH  = 320;
static const int FRAME_HEIGHT = 240;
static const int FRAME_BYTES  = FRAME_WIDTH * FRAME_HEIGHT * 2;
static const int PREAMBLE_LEN = 512;
static const int READ_SIZE    = FRAME_BYTES + PREAMBLE_LEN;
} // namespace yuyv

} // namespace upp

#endif // UPPPROTOCOL_H
