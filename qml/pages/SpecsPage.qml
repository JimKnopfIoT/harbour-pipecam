/*
 * SpecsPage.qml — everything known about the attached camera.
 *
 * Two kinds of information, deliberately kept apart:
 *
 *   * what the DEVICE says about itself — read live from its USB descriptors
 *     every time it is opened, so these are facts about the thing on the end of
 *     the cable, not constants baked into the app;
 *   * what WE worked out — the protocol, the frame rate, the fields whose
 *     meaning is known and the ones that are not. This camera family is
 *     undocumented, so this page is the only place that knowledge is visible
 *     without reading the source.
 *
 * Worth having in the app rather than only in the repo: these cameras ship
 * under a dozen names with no data sheet, and "does my cable work with this?"
 * is answered by the VID:PID line at the top.
 *
 * Copyright (C) 2026  JimKnopfIoT — GPLv3 or later.
 */
import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page
    allowedOrientations: Orientation.Landscape

    readonly property var info: app.camera.deviceInfo
    readonly property bool haveDevice: info && info.product !== undefined

    function val(key, fallback) {
        if (!info || info[key] === undefined || info[key] === "")
            return fallback !== undefined ? fallback : qsTr("unknown")
        return info[key]
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader {
                title: qsTr("Camera specs")
                description: page.haveDevice ? page.val("product", "")
                                             : qsTr("no camera connected")
            }

            Label {
                visible: !page.haveDevice
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                text: qsTr("Connect the camera and start it; the device details "
                           + "below are read from the camera itself when it is "
                           + "opened.")
            }

            SectionHeader { text: qsTr("Device") }

            DetailItem { label: qsTr("Manufacturer"); value: page.val("manufacturer") }
            DetailItem { label: qsTr("Product");      value: page.val("product") }
            DetailItem { label: qsTr("Serial");       value: page.val("serial") }
            DetailItem {
                label: qsTr("USB ID")
                value: page.haveDevice
                       ? page.val("vendorId") + ":" + page.val("productId")
                       : qsTr("unknown")
            }
            DetailItem { label: qsTr("Hardware revision"); value: page.val("deviceVersion") }
            DetailItem { label: qsTr("USB version");   value: page.val("usbVersion") }
            DetailItem { label: qsTr("Link speed");    value: page.val("speed") }
            DetailItem { label: qsTr("Power draw");    value: page.val("maxPower") }
            DetailItem {
                label: qsTr("Bus / address")
                value: page.haveDevice
                       ? page.val("busNumber") + " / " + page.val("deviceAddress")
                       : qsTr("unknown")
            }

            SectionHeader { text: qsTr("USB interfaces") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.family: Theme.fontFamilyHeading
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.highlightColor
                text: page.val("interfaces", qsTr("unknown"))
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("Class ff is “vendor specific”. That is why Linux's "
                           + "built-in UVC driver never binds this camera and no "
                           + "/dev/video node appears — the device only looks "
                           + "like a webcam from the outside.")
            }

            SectionHeader { text: qsTr("Video stream") }

            DetailItem { label: qsTr("Resolution"); value: app.camera.frameWidth
                                                          + " × " + app.camera.frameHeight }
            DetailItem { label: qsTr("Format");     value: "MJPEG" }
            DetailItem {
                label: qsTr("Frame rate now")
                value: app.camera.streaming ? app.camera.fps.toFixed(1) + " fps"
                                            : qsTr("not streaming")
            }
            DetailItem { label: qsTr("Typical rate");  value: qsTr("about 11–15 fps") }
            DetailItem { label: qsTr("Frame size");    value: qsTr("about 8–30 kB per frame") }
            DetailItem { label: qsTr("Frames so far"); value: app.camera.frameCount }

            SectionHeader { text: qsTr("Protocol") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("Undocumented vendor protocol, reverse engineered. "
                           + "Two commands are known:\n\n"
                           + "    FF 55 FF 55 EE 10   initialise (control out)\n"
                           + "    BB AA 05 00 00      connect (stream out)\n\n"
                           + "Each 1024-byte packet is a 5-byte USB header, a "
                           + "7-byte camera header and a slice of JPEG. Frames "
                           + "are delimited by the frame-id byte changing — not "
                           + "by searching for JPEG markers, which is the "
                           + "mistake that produces half-grey pictures.")
            }

            SectionHeader { text: qsTr("What the header fields mean") }

            DetailItem { label: qsTr("Camera id"); value: qsTr("always 7 here (11 never seen)") }
            DetailItem { label: qsTr("cam_num");   value: qsTr("toggles 0/1 per frame") }
            DetailItem { label: qsTr("flags bit 1"); value: qsTr("push-button on the cable") }
            DetailItem { label: qsTr("32-bit field"); value: qsTr("cycles through 4 fixed values — unknown") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeExtraSmall
                color: Theme.secondaryColor
                text: qsTr("The reference implementation this was ported from "
                           + "describes camera id 7 and 11 as two halves of one "
                           + "frame, and the 32-bit field as a g-sensor. Neither "
                           + "matches what this device does: id 7 alone yields "
                           + "complete images, and the 32-bit field cycles "
                           + "through the same four values on a motionless "
                           + "cable.")
            }

            SectionHeader { text: qsTr("Lighting") }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryColor
                text: qsTr("The LED ring cannot be controlled from the phone. "
                           + "Measured: 60 seconds of capture while the cable's "
                           + "dimmer wheel was turned produced no change in any "
                           + "header field and not one byte on the control "
                           + "endpoint — while a button press in the same run "
                           + "showed up immediately. The wheel is an analogue "
                           + "potentiometer in the LED supply and the firmware "
                           + "never sees it.\n\n"
                           + "The brightness bar in the viewfinder therefore "
                           + "brightens the picture, not the lamp.")
            }

            Item { width: 1; height: Theme.paddingLarge }
        }

        VerticalScrollDecorator {}
    }
}
