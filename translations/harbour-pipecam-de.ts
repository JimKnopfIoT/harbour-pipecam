<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE TS>
<TS version="2.1" language="de_DE">
<context>
    <name>AboutPage</name>
    <message>
        <location filename="../qml/pages/AboutPage.qml" line="27"/>
        <source>About PipeCam</source>
        <translation>Über PipeCam</translation>
    </message>
    <message>
        <location filename="../qml/pages/AboutPage.qml" line="41"/>
        <source>Live view, snapshots and recording for USB-C pipe inspection cameras.</source>
        <translation>Livebild, Fotos und Aufnahmen für USB-C-Rohrkameras.</translation>
    </message>
    <message>
        <location filename="../qml/pages/AboutPage.qml" line="47"/>
        <source>Supported hardware</source>
        <translation>Unterstützte Hardware</translation>
    </message>
    <message>
        <location filename="../qml/pages/AboutPage.qml" line="55"/>
        <source>Endoscope cameras that speak the “com.useeplus” protocol, sold as USeePlus, Geek szitman or supercamera:

    USB 2ce3:3828
    USB 0329:2022

These cameras are not UVC devices, so no kernel driver binds them and they never appear as a /dev/video node. PipeCam drives their USB endpoints directly.</source>
        <translation>Endoskopkameras, die das „com.useeplus“-Protokoll sprechen — verkauft als USeePlus, Geek szitman oder supercamera:

    USB 2ce3:3828
    USB 0329:2022

Diese Kameras sind keine UVC-Geräte. Deshalb bindet kein Kerneltreiber sie, und sie erscheinen nie als /dev/video-Knoten. PipeCam spricht ihre USB-Endpunkte direkt an.</translation>
    </message>
    <message>
        <location filename="../qml/pages/AboutPage.qml" line="66"/>
        <source>How captures are stored</source>
        <translation>Wie Aufnahmen gespeichert werden</translation>
    </message>
    <message>
        <location filename="../qml/pages/AboutPage.qml" line="74"/>
        <source>The camera streams MJPEG, so a snapshot is written as the camera&apos;s own untouched JPEG and a recording is muxed straight into MP4 — nothing is re-encoded. Switching on the timestamp or the brightness gain does force a re-encode, because both change the picture. No location or device information is added to any file.</source>
        <translation>Die Kamera liefert MJPEG. Ein Foto wird deshalb als das unveränderte JPEG der Kamera gespeichert, eine Aufnahme direkt in eine MP4-Datei gemuxt — ohne Neukodierung. Zeitstempel oder Helligkeitsanhebung erzwingen allerdings eine Neukodierung, weil beide das Bild verändern. Keiner Datei werden Standort- oder Geräteinformationen hinzugefügt.</translation>
    </message>
    <message>
        <location filename="../qml/pages/AboutPage.qml" line="83"/>
        <source>Privacy</source>
        <translation>Privatsphäre</translation>
    </message>
    <message>
        <location filename="../qml/pages/AboutPage.qml" line="91"/>
        <source>Everything runs on the device. PipeCam contains no network code at all: there is no telemetry, no cloud component and nothing to opt out of.</source>
        <translation>Alles läuft auf dem Gerät. PipeCam enthält überhaupt keinen Netzwerkcode: keine Telemetrie, keine Cloud, nichts zum Abschalten.</translation>
    </message>
    <message>
        <location filename="../qml/pages/AboutPage.qml" line="96"/>
        <source>Licence</source>
        <translation>Lizenz</translation>
    </message>
    <message>
        <location filename="../qml/pages/AboutPage.qml" line="104"/>
        <source>GNU General Public License v3.0 or later. Provided as is, with no warranty.</source>
        <translation>GNU General Public License v3.0 oder später. Ohne jede Gewährleistung.</translation>
    </message>
</context>
<context>
    <name>CaptureStore</name>
    <message>
        <location filename="../src/app/capturestore.cpp" line="77"/>
        <source>%1 B</source>
        <translation>%1 B</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="79"/>
        <source>%1 kB</source>
        <translation>%1 kB</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="80"/>
        <source>%1 MB</source>
        <translation>%1 MB</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="90"/>
        <source>Cannot create folder %1</source>
        <translation>Ordner %1 kann nicht angelegt werden</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="114"/>
        <source>No camera.</source>
        <translation>Keine Kamera.</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="120"/>
        <source>No frame to save yet.</source>
        <translation>Noch kein Bild zum Speichern.</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="142"/>
        <source>Cannot write %1: %2</source>
        <translation>%1 kann nicht geschrieben werden: %2</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="148"/>
        <source>Only wrote %1 of %2 bytes to %3</source>
        <translation>Nur %1 von %2 Bytes nach %3 geschrieben</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="161"/>
        <source>Could not decode the frame to add the timestamp.</source>
        <translation>Bild konnte zum Einfügen des Zeitstempels nicht dekodiert werden.</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="173"/>
        <source>Cannot write %1</source>
        <translation>%1 kann nicht geschrieben werden</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="234"/>
        <source>Cannot delete %1</source>
        <translation>%1 kann nicht gelöscht werden</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="264"/>
        <source>The name cannot be empty.</source>
        <translation>Der Name darf nicht leer sein.</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="270"/>
        <source>A name cannot contain “/” or start with a dot.</source>
        <translation>Ein Name darf kein „/“ enthalten und nicht mit einem Punkt beginnen.</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="282"/>
        <source>“%1” already exists.</source>
        <translation>„%1“ existiert bereits.</translation>
    </message>
    <message>
        <location filename="../src/app/capturestore.cpp" line="286"/>
        <source>Could not rename “%1”.</source>
        <translation>„%1“ konnte nicht umbenannt werden.</translation>
    </message>
</context>
<context>
    <name>CaptureViewPage</name>
    <message>
        <location filename="../qml/pages/CaptureViewPage.qml" line="47"/>
        <source>Delete</source>
        <translation>Löschen</translation>
    </message>
    <message>
        <location filename="../qml/pages/CaptureViewPage.qml" line="48"/>
        <source>Deleting</source>
        <translation>Wird gelöscht</translation>
    </message>
    <message>
        <location filename="../qml/pages/CaptureViewPage.qml" line="54"/>
        <source>Rename</source>
        <translation>Umbenennen</translation>
    </message>
    <message>
        <location filename="../qml/pages/CaptureViewPage.qml" line="75"/>
        <source>Share</source>
        <translation>Teilen</translation>
    </message>
</context>
<context>
    <name>CoverPage</name>
    <message numerus="yes">
        <location filename="../qml/cover/CoverPage.qml" line="105"/>
        <source>%n capture(s)</source>
        <translation>
            <numerusform>%n Aufnahme</numerusform>
            <numerusform>%n Aufnahmen</numerusform>
        </translation>
    </message>
</context>
<context>
    <name>GalleryPage</name>
    <message>
        <location filename="../qml/pages/GalleryPage.qml" line="33"/>
        <source>Captures</source>
        <translation>Aufnahmen</translation>
    </message>
    <message numerus="yes">
        <location filename="../qml/pages/GalleryPage.qml" line="35"/>
        <source>%n item(s)</source>
        <translation>
            <numerusform>%n Eintrag</numerusform>
            <numerusform>%n Einträge</numerusform>
        </translation>
    </message>
    <message>
        <location filename="../qml/pages/GalleryPage.qml" line="42"/>
        <source>Refresh</source>
        <translation>Aktualisieren</translation>
    </message>
    <message>
        <location filename="../qml/pages/GalleryPage.qml" line="72"/>
        <source>Rename</source>
        <translation>Umbenennen</translation>
    </message>
    <message>
        <location filename="../qml/pages/GalleryPage.qml" line="86"/>
        <source>Delete</source>
        <translation>Löschen</translation>
    </message>
    <message>
        <location filename="../qml/pages/GalleryPage.qml" line="102"/>
        <source>Deleting</source>
        <translation>Wird gelöscht</translation>
    </message>
    <message>
        <location filename="../qml/pages/GalleryPage.qml" line="158"/>
        <source>No captures yet</source>
        <translation>Noch keine Aufnahmen</translation>
    </message>
    <message>
        <location filename="../qml/pages/GalleryPage.qml" line="159"/>
        <source>Snapshots and recordings you make will appear here.</source>
        <translation>Fotos und Videoaufnahmen erscheinen hier.</translation>
    </message>
</context>
<context>
    <name>MjpegRecorder</name>
    <message>
        <location filename="../src/camera/mjpegrecorder.cpp" line="166"/>
        <source>Already recording.</source>
        <translation>Es läuft bereits eine Aufnahme.</translation>
    </message>
    <message>
        <location filename="../src/camera/mjpegrecorder.cpp" line="201"/>
        <source>Video recording is unavailable: a required GStreamer element is missing (appsrc/jpegparse/qtmux/filesink).</source>
        <translation>Videoaufnahme nicht verfügbar: ein benötigtes GStreamer-Element fehlt (appsrc/jpegparse/qtmux/filesink).</translation>
    </message>
    <message>
        <location filename="../src/camera/mjpegrecorder.cpp" line="234"/>
        <source>Could not build the recording pipeline.</source>
        <translation>Die Aufnahme-Pipeline konnte nicht aufgebaut werden.</translation>
    </message>
    <message>
        <location filename="../src/camera/mjpegrecorder.cpp" line="242"/>
        <source>Could not start recording to %1.</source>
        <translation>Aufnahme nach %1 konnte nicht gestartet werden.</translation>
    </message>
    <message>
        <location filename="../src/camera/mjpegrecorder.cpp" line="326"/>
        <location filename="../src/camera/mjpegrecorder.cpp" line="367"/>
        <source>Recording failed: %1</source>
        <translation>Aufnahme fehlgeschlagen: %1</translation>
    </message>
    <message>
        <location filename="../src/camera/mjpegrecorder.cpp" line="357"/>
        <source>Timed out finalising the video file — it may be incomplete.</source>
        <translation>Zeitüberschreitung beim Abschließen der Videodatei — sie ist womöglich unvollständig.</translation>
    </message>
</context>
<context>
    <name>QObject</name>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="135"/>
        <source>Camera found but not accessible. The USB permission rule is missing — reinstall the app and replug the camera.</source>
        <translation>Kamera gefunden, aber nicht zugreifbar. Die USB-Berechtigungsregel fehlt — App neu installieren und Kamera aus- und wieder einstecken.</translation>
    </message>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="139"/>
        <source>No camera detected. Check the USB-C connection.</source>
        <translation>Keine Kamera erkannt. USB-C-Verbindung prüfen.</translation>
    </message>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="236"/>
        <source>Not allowed to open the camera (USB permissions).</source>
        <translation>Keine Berechtigung, die Kamera zu öffnen (USB-Rechte).</translation>
    </message>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="237"/>
        <source>Camera is busy — unplug and replug it.</source>
        <translation>Kamera ist belegt — aus- und wieder einstecken.</translation>
    </message>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="255"/>
        <source>Could not start the camera&apos;s video interface.</source>
        <translation>Das Video-Interface der Kamera konnte nicht gestartet werden.</translation>
    </message>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="265"/>
        <source>Camera did not accept the initialisation command.</source>
        <translation>Die Kamera hat den Initialisierungsbefehl nicht angenommen.</translation>
    </message>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="272"/>
        <source>Camera did not accept the connect command.</source>
        <translation>Die Kamera hat den Verbindungsbefehl nicht angenommen.</translation>
    </message>
</context>
<context>
    <name>RenameDialog</name>
    <message>
        <location filename="../qml/pages/RenameDialog.qml" line="38"/>
        <source>Rename</source>
        <translation>Umbenennen</translation>
    </message>
    <message>
        <location filename="../qml/pages/RenameDialog.qml" line="39"/>
        <source>Cancel</source>
        <translation>Abbrechen</translation>
    </message>
    <message>
        <location filename="../qml/pages/RenameDialog.qml" line="45"/>
        <location filename="../qml/pages/RenameDialog.qml" line="46"/>
        <source>Name</source>
        <translation>Name</translation>
    </message>
    <message>
        <location filename="../qml/pages/RenameDialog.qml" line="68"/>
        <source>Saved as “%1”</source>
        <translation>Wird gespeichert als „%1“</translation>
    </message>
</context>
<context>
    <name>SettingsPage</name>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="30"/>
        <source>Settings</source>
        <translation>Einstellungen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="36"/>
        <source>Camera</source>
        <translation>Kamera</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="39"/>
        <source>Camera on</source>
        <translation>Kamera an</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="48"/>
        <source>Show date and time</source>
        <translation>Datum und Uhrzeit anzeigen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="49"/>
        <source>Shown in the corner of the picture and burnt into snapshots and recordings. Burning it in means each frame has to be re-encoded instead of being saved exactly as the camera sent it.</source>
        <translation>Wird in der Bildecke angezeigt und in Fotos und Aufnahmen eingebrannt. Durch das Einbrennen muss jedes Bild neu kodiert werden, statt genau so gespeichert zu werden, wie die Kamera es geliefert hat.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="58"/>
        <source>Image</source>
        <translation>Bild</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="61"/>
        <source>Mirror image</source>
        <translation>Bild spiegeln</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="62"/>
        <source>Flip left to right. Useful when the camera head is fed into the pipe rotated.</source>
        <translation>Links und rechts vertauschen. Nützlich, wenn der Kamerakopf verdreht ins Rohr geschoben wird.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="69"/>
        <source>Scaling</source>
        <translation>Skalierung</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="70"/>
        <source>How the 4:3 camera image fills the screen.</source>
        <translation>Wie das 4:3-Kamerabild den Bildschirm ausfüllt.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="73"/>
        <source>Fit — show the whole frame</source>
        <translation>Einpassen — ganzes Bild zeigen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="74"/>
        <source>Fill — crop the edges</source>
        <translation>Ausfüllen — Ränder beschneiden</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="75"/>
        <source>Stretch — distort to fit</source>
        <translation>Strecken — verzerrt einpassen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="81"/>
        <source>Save rotated</source>
        <translation>Gedreht speichern</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="82"/>
        <source>Apply the roll dial to snapshots and recordings, not only to the live view. The picture is scaled to fit, so nothing is cut off — turning it leaves black corners.</source>
        <translation>Die Drehung des Rings auch auf Fotos und Aufnahmen anwenden, nicht nur auf das Livebild. Das Bild wird passend verkleinert, es geht also nichts verloren — beim Drehen bleiben schwarze Ecken.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="91"/>
        <source>Show grid</source>
        <translation>Raster anzeigen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="92"/>
        <source>Thirds overlay — helps to judge whether the camera head is centred in the pipe.</source>
        <translation>Drittel-Raster — hilft zu beurteilen, ob der Kamerakopf mittig im Rohr läuft.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="98"/>
        <source>Camera cable</source>
        <translation>Kamerakabel</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="106"/>
        <source>The dimmer wheel is wired straight to the LEDs and cannot be read or driven from the phone (measured — see the specs page). The bar on the left of the viewfinder brightens the picture instead.</source>
        <translation>Das Dimm-Rad ist direkt mit den LEDs verdrahtet und kann vom Telefon aus weder gelesen noch gesteuert werden (gemessen — siehe Technische Daten). Die Leiste links im Sucher hellt stattdessen das Bild auf.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="113"/>
        <source>Button on the cable</source>
        <translation>Taster am Kabel</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="114"/>
        <source>What the push-button on the camera cable does. Turn it off if you keep catching it while using the dimmer wheel next to it.</source>
        <translation>Was der Taster am Kamerakabel auslöst. Abschalten, wenn du ihn beim Bedienen des Dimm-Rads daneben ständig erwischst.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="123"/>
        <source>Take a snapshot</source>
        <translation>Foto aufnehmen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="124"/>
        <source>Start / stop recording</source>
        <translation>Aufnahme starten / stoppen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="125"/>
        <source>Do nothing</source>
        <translation>Nichts tun</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="135"/>
        <source>Behaviour</source>
        <translation>Verhalten</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="138"/>
        <source>Keep the display on</source>
        <translation>Bildschirm anlassen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="139"/>
        <source>Prevents blanking while the camera is live. Only active when frames are arriving.</source>
        <translation>Verhindert das Abschalten, solange die Kamera Bilder liefert. Nur aktiv, wenn tatsächlich Bilder ankommen.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="145"/>
        <source>Storage</source>
        <translation>Speicherort</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="148"/>
        <source>Photos</source>
        <translation>Fotos</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="152"/>
        <source>Videos</source>
        <translation>Videos</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="156"/>
        <source>Captures</source>
        <translation>Aufnahmen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="160"/>
        <source>Open the gallery</source>
        <translation>Galerie öffnen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="168"/>
        <source>Camera specs</source>
        <translation>Technische Daten</translation>
    </message>
    <message>
        <location filename="../qml/pages/SettingsPage.qml" line="176"/>
        <source>About PipeCam</source>
        <translation>Über PipeCam</translation>
    </message>
</context>
<context>
    <name>SpecsPage</name>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="32"/>
        <location filename="../qml/pages/SpecsPage.qml" line="71"/>
        <location filename="../qml/pages/SpecsPage.qml" line="81"/>
        <location filename="../qml/pages/SpecsPage.qml" line="93"/>
        <source>unknown</source>
        <translation>unbekannt</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="45"/>
        <source>Camera specs</source>
        <translation>Technische Daten</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="47"/>
        <source>no camera connected</source>
        <translation>keine Kamera verbunden</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="57"/>
        <source>Connect the camera and start it; the device details below are read from the camera itself when it is opened.</source>
        <translation>Kamera anschließen und starten — die Gerätedaten unten werden beim Öffnen direkt aus der Kamera gelesen.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="62"/>
        <source>Device</source>
        <translation>Gerät</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="64"/>
        <source>Manufacturer</source>
        <translation>Hersteller</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="65"/>
        <source>Product</source>
        <translation>Produkt</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="66"/>
        <source>Serial</source>
        <translation>Seriennummer</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="68"/>
        <source>USB ID</source>
        <translation>USB-ID</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="73"/>
        <source>Hardware revision</source>
        <translation>Hardware-Revision</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="74"/>
        <source>USB version</source>
        <translation>USB-Version</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="75"/>
        <source>Link speed</source>
        <translation>Übertragungsrate</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="76"/>
        <source>Power draw</source>
        <translation>Stromaufnahme</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="78"/>
        <source>Bus / address</source>
        <translation>Bus / Adresse</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="84"/>
        <source>USB interfaces</source>
        <translation>USB-Schnittstellen</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="101"/>
        <source>Class ff is “vendor specific”. That is why Linux&apos;s built-in UVC driver never binds this camera and no /dev/video node appears — the device only looks like a webcam from the outside.</source>
        <translation>Klasse ff bedeutet „herstellerspezifisch“. Deshalb bindet der in Linux eingebaute UVC-Treiber diese Kamera nie und es entsteht kein /dev/video-Knoten — das Gerät sieht nur von außen wie eine Webcam aus.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="107"/>
        <source>Video stream</source>
        <translation>Videostrom</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="109"/>
        <source>Resolution</source>
        <translation>Auflösung</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="111"/>
        <source>Format</source>
        <translation>Format</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="113"/>
        <source>Frame rate now</source>
        <translation>Aktuelle Bildrate</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="115"/>
        <source>not streaming</source>
        <translation>kein Bildstrom</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="117"/>
        <source>Typical rate</source>
        <translation>Typische Bildrate</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="117"/>
        <source>about 11–15 fps</source>
        <translation>etwa 11–15 fps</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="118"/>
        <source>Frame size</source>
        <translation>Bildgröße</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="118"/>
        <source>about 8–30 kB per frame</source>
        <translation>etwa 8–30 kB pro Bild</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="119"/>
        <source>Frames so far</source>
        <translation>Bilder bisher</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="121"/>
        <source>Protocol</source>
        <translation>Protokoll</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="129"/>
        <source>Undocumented vendor protocol, reverse engineered. Two commands are known:

    FF 55 FF 55 EE 10   initialise (control out)
    BB AA 05 00 00      connect (stream out)

Each 1024-byte packet is a 5-byte USB header, a 7-byte camera header and a slice of JPEG. Frames are delimited by the frame-id byte changing — not by searching for JPEG markers, which is the mistake that produces half-grey pictures.</source>
        <translation>Undokumentiertes Herstellerprotokoll, per Reverse Engineering erschlossen. Zwei Befehle sind bekannt:

    FF 55 FF 55 EE 10   Initialisierung (Steuer-Ausgang)
    BB AA 05 00 00      Verbinden (Strom-Ausgang)

Jedes 1024-Byte-Paket besteht aus 5 Byte USB-Kopf, 7 Byte Kamera-Kopf und einem Stück JPEG. Bilder werden durch den Wechsel des Frame-ID-Bytes abgegrenzt — nicht durch die Suche nach JPEG-Markern, was der Fehler ist, der halb graue Bilder erzeugt.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="140"/>
        <source>What the header fields mean</source>
        <translation>Bedeutung der Kopf-Felder</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="142"/>
        <source>Camera id</source>
        <translation>Kamera-ID</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="142"/>
        <source>always 7 here (11 never seen)</source>
        <translation>hier immer 7 (11 nie gesehen)</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="143"/>
        <source>cam_num</source>
        <translation>cam_num</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="143"/>
        <source>toggles 0/1 per frame</source>
        <translation>wechselt 0/1 pro Bild</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="144"/>
        <source>flags bit 1</source>
        <translation>flags Bit 1</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="144"/>
        <source>push-button on the cable</source>
        <translation>Taster am Kabel</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="145"/>
        <source>32-bit field</source>
        <translation>32-Bit-Feld</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="145"/>
        <source>cycles through 4 fixed values — unknown</source>
        <translation>zykliert durch 4 feste Werte — unbekannt</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="153"/>
        <source>The reference implementation this was ported from describes camera id 7 and 11 as two halves of one frame, and the 32-bit field as a g-sensor. Neither matches what this device does: id 7 alone yields complete images, and the 32-bit field cycles through the same four values on a motionless cable.</source>
        <translation>Die Referenz-Implementierung, von der hier portiert wurde, beschreibt Kamera-ID 7 und 11 als zwei Hälften eines Bildes und das 32-Bit-Feld als Lagesensor. Beides passt nicht zu diesem Gerät: ID 7 allein liefert vollständige Bilder, und das 32-Bit-Feld zykliert bei unbewegtem Kabel durch immer dieselben vier Werte.</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="162"/>
        <source>Lighting</source>
        <translation>Beleuchtung</translation>
    </message>
    <message>
        <location filename="../qml/pages/SpecsPage.qml" line="170"/>
        <source>The LED ring cannot be controlled from the phone. Measured: 60 seconds of capture while the cable&apos;s dimmer wheel was turned produced no change in any header field and not one byte on the control endpoint — while a button press in the same run showed up immediately. The wheel is an analogue potentiometer in the LED supply and the firmware never sees it.

The brightness bar in the viewfinder therefore brightens the picture, not the lamp.</source>
        <translation>Die LED-Ringbeleuchtung lässt sich vom Telefon aus nicht steuern. Gemessen: 60 Sekunden Aufzeichnung, während das Dimm-Rad am Kabel gedreht wurde, ergaben keine Änderung in irgendeinem Kopf-Feld und kein einziges Byte am Steuer-Endpunkt — während ein Tastendruck im selben Lauf sofort sichtbar war. Das Rad ist ein analoges Potentiometer im LED-Zweig, das die Firmware nie sieht.

Die Helligkeitsleiste im Sucher hellt deshalb das Bild auf, nicht die Lampe.</translation>
    </message>
</context>
<context>
    <name>StatusOverlay</name>
    <message>
        <location filename="../qml/components/StatusOverlay.qml" line="59"/>
        <source>Open settings with the gear button and switch the camera on.</source>
        <translation>Einstellungen über das Zahnrad öffnen und die Kamera einschalten.</translation>
    </message>
</context>
<context>
    <name>UppCamera</name>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="455"/>
        <source>Off</source>
        <translation>Aus</translation>
    </message>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="456"/>
        <source>Looking for camera…</source>
        <translation>Suche Kamera…</translation>
    </message>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="457"/>
        <source>Connecting…</source>
        <translation>Verbinde…</translation>
    </message>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="458"/>
        <source>Live</source>
        <translation>Live</translation>
    </message>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="459"/>
        <source>Problem</source>
        <translation>Problem</translation>
    </message>
</context>
<context>
    <name>UppCameraWorker</name>
    <message>
        <location filename="../src/camera/uppcamera.cpp" line="356"/>
        <source>USB subsystem unavailable.</source>
        <translation>USB-Subsystem nicht verfügbar.</translation>
    </message>
</context>
<context>
    <name>ViewfinderPage</name>
    <message>
        <location filename="../qml/pages/ViewfinderPage.qml" line="241"/>
        <source>%1 fps</source>
        <translation>%1 fps</translation>
    </message>
</context>
</TS>
