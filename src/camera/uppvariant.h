#ifndef UPPVARIANT_H
#define UPPVARIANT_H

#include "uppprotocol.h"

#include <libusb-1.0/libusb.h>

#include <QString>

namespace upp {

/* Variants share VID:PID and strings; layout decides. iface 1 -> MJPEG;
 * single iface with bulk 0x82 IN + 0x02 OUT -> YUYV; else Unknown (refuse). */
inline Variant variantOf(const libusb_config_descriptor *cfg)
{
    for (int i = 0; i < cfg->bNumInterfaces; ++i) {
        const libusb_interface &itf = cfg->interface[i];
        if (itf.num_altsetting > 0 && itf.altsetting[0].bInterfaceNumber == IFACE_STREAM)
            return VariantMjpeg;
    }
    if (cfg->bNumInterfaces != 1 || cfg->interface[0].num_altsetting < 1)
        return VariantUnknown;

    const libusb_interface_descriptor &alt = cfg->interface[0].altsetting[0];
    bool in = false, out = false;
    for (int e = 0; e < alt.bNumEndpoints; ++e) {
        const libusb_endpoint_descriptor &ep = alt.endpoint[e];
        if ((ep.bmAttributes & LIBUSB_TRANSFER_TYPE_MASK) != LIBUSB_TRANSFER_TYPE_BULK)
            continue;
        if (ep.bEndpointAddress == yuyv::EP_IN)
            in = true;
        else if (ep.bEndpointAddress == yuyv::EP_OUT)
            out = true;
    }
    return alt.bInterfaceNumber == yuyv::IFACE && in && out ? VariantYuyv : VariantUnknown;
}

inline QString variantName(int v)
{
    switch (v) {
    case VariantMjpeg: return QStringLiteral("mjpeg (2 interfaces)");
    case VariantYuyv:  return QStringLiteral("yuyv (1 interface)");
    }
    return QStringLiteral("unknown");
}

} // namespace upp

#endif // UPPVARIANT_H
