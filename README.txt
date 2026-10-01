Dokumentation Teil I von II: 

Kurze Einführung in die Ambiophonie

1. Konzept und Zielsetzung
Ambiophonie ist ein reproduktives Audioverfahren, das darauf abzielt, ein dreidimensionales Schallfeld mit höchster psychoakustischer Präzision über lediglich zwei Lautsprecher abzubilden. Im Gegensatz zur klassischen Stereofonie, die ein gleichseitiges Dreieck (60°-Winkel) voraussetzt, erfordert die Ambiophonie einen sehr engen Lautsprecherabstand (typischerweise 20° bis 24°) in Kombination mit digitaler Signalverarbeitung (DSP) zur Eliminierung des akustischen Übersprechens.

2. Das physikalische Problem: Akustisches Übersprechen (Crosstalk)
Bei der herkömmlichen Stereowiedergabe erreicht das Signal des linken Lautsprechers nicht nur das linke Ohr, sondern zeitverzögert und durch den Kopfschatten abgeschwächt auch das rechte Ohr des Hörers (und umgekehrt). Dieses kontralaterale Übersprechen verursacht fundamentale Fehler bei der Schallreproduktion:

Kammfiltereffekte: Phasenauslöschungen im Mittel- und Hochtonbereich verfälschen die tonale Balance und den Frequenzgang.

Pinna-Fehler: Die Ohrmuschel (Pinna) wird aus den falschen Winkeln (±30°) angeregt. Das Gehirn interpretiert die HRTF-Hinweise (Head-Related Transfer Function) inkorrekt.

Limitierte Räumlichkeit: Die wahrgenommene Klangbühne bleibt künstlich auf den Raum strikt zwischen den Lautsprechern limitiert.

3. Die Lösung: Crosstalk Cancellation (XTC) und RACE
Um das natürliche binaurale Hören zu emulieren, müssen die Übersprechsignale an den Ohren des Hörers ausgelöscht werden. Ralph Glasgal entwickelte hierfür gemeinsam mit anderen Forschern praxisnahe Algorithmen, insbesondere das "Recursive Ambiophonic Crosstalk Elimination" (RACE) Verfahren.

Der RACE-Algorithmus generiert ein invertiertes, exakt zeitverzögertes und im Pegel angepasstes Korrektursignal. Trifft der parasitäre Schall (z. B. vom linken Lautsprecher) am rechten Ohr ein, wird er dort exakt in diesem Moment durch das vom rechten Lautsprecher ausgesendete RACE-Korrektursignal physikalisch ausgelöscht.

4. Psychoakustische Wirkung
Durch die erfolgreiche Übersprechdämpfung erhalten beide Ohren hochgradig isolierte Signale – vergleichbar mit dem Hören über Kopfhörer, jedoch unter Beibehaltung der natürlichen Körperschallwahrnehmung und ohne die unnatürliche "Im-Kopf-Lokalisation".
Die essenziellen Lokalisationsparameter des menschlichen Gehirns – die Interaurale Laufzeitdifferenz (ITD) und die Interaurale Pegeldifferenz (ILD) – bleiben unkorrumpiert. Das Resultat ist eine holografische, tiefe und extrem breite Klangbühne, die den Aufnahmewinkel der originalen Mikrofone (bis zu 150° und mehr) physikalisch korrekt am Hörplatz rekonstruiert.

Ausgewählte Literatur & Referenzen
Glasgal, R. (2001). "Ambiophonics: Achieving Physiological Realism in Sound Reproduction". Audio Engineering Society Convention 111. (Einführung in die physiologischen Grundlagen der Ambiophonie und die Limitationen der klassischen Stereofonie).

Glasgal, R. (2007). "Ambiophonics: Beyond Surround Sound to Virtual Sonic Reality". Ambiophonics Institute. (Detaillierte Beschreibung der RACE-Algorithmen und der praktischen Anwendung von Crosstalk Cancellation-Filtern).

Glasgal, R. (2000). "The Ambiophonic Audio System: Using Frontal Crosstalk Cancellation and Surround Convolvers to Render 3D Sound for Two or More Loudspeakers". Ambiophonics Institute / AES.

Macpherson, E. A. (1998). "A Computer Model of Binaural Localization for Stereo Imaging Measurement". Journal of the Audio Engineering Society. (Grundlagen zur ITD- und ILD-Auswertung im menschlichen Gehirn, auf denen das ambiophone Modell aufbaut).=========================================================
OAPW - System-Administration & Audio-Routing (Version 11)
=========================================================

Dokumentation Teil II von II:

Technische Implementierung und Limitationen

Dieses Dokument beschreibt die systemseitige Konfiguration und den laufenden Betrieb der OAPW-Engine auf dem Raspberry Pi 5 unter Linux.

1. DER HINTERGRUNDDIENST (oapw.service)
---------------------------------------------------------
Die C++ OAPW-Player Engine v11 (Psychoakustik-Update) läuft permanent als autarker Hintergrunddienst. Der Dienst horcht kontinuierlich auf die Named Pipe (/tmp/oapw_stream), verarbeitet das Signal durch die phasenlineare Biquad-Filterkette und gibt es exklusiv und echtzeitnah über ALSA an den DAC aus. Das integrierte Web-Interface zur Live-Steuerung und Auto-Kalibrierung ist dauerhaft über Port 8080 erreichbar.

Wichtige Terminal-Befehle für die Dienst-Verwaltung:
* Player manuell stoppen:   sudo systemctl stop oapw
* Player manuell starten:   sudo systemctl start oapw
* Player neu starten:       sudo systemctl restart oapw
* Status/Fehler anzeigen:   sudo systemctl status oapw
* Live-Logbuch ansehen:     journalctl -u oapw -f


2. AIRPLAY-INTEGRATION (Shairport Sync)
---------------------------------------------------------
Das systemweite AirPlay-Routing wird über den separaten Dienst "shairport-sync" abgewickelt. Dieser ist in der Konfigurationsdatei (/etc/shairport-sync.conf) so eingerichtet, dass er den decodierten Audiostream nicht direkt an die Hardware-Soundkarte sendet, sondern ihn als Raw-Audio nahtlos in die Named Pipe (/tmp/oapw_stream) schiebt, von wo die OAPW-Engine ihn abgreift.


3. EXKLUSIVER HARDWARE-ZUGRIFF & TEST-SZENARIEN
---------------------------------------------------------
Da die C++ Engine für minimale Latenzen extrem nah an der Hardware arbeitet (miniaudio ALSA Push-Mode), sperrt sie den Audio-DAC exklusiv für sich.

* Unkritische Dienste (z. B. Navidrome): Kein Konflikt. Navidrome streamt lediglich Musik ins Netzwerk und greift nicht auf den lokalen ALSA-Treiber zu.
* Potenzielle Konflikte mit anderen Audio-Programmen: Wenn Programme gestartet werden, die die lokale ALSA-Schnittstelle ansprechen, kollidieren diese mit dem blockierten ALSA-Treiber des C++ Players. Konflikte können vermieden werden, wenn nach dem entweder/oder Prinzip gearbeitet wird, das heißt: Entweder oapw oder das andere Programm, das auf ALSA zugreift laufen lassen.

Lösungsansatz:
Vor dem Ausführen der anderen Programme muss der C++ Hintergrunddienst zwingend freigegeben werden:
1. Terminal-Befehl ausführen: sudo systemctl stop oapw
2. Tests auf der nun freien Soundkarte durchführen.
3. Terminal-Befehl ausführen: sudo systemctl start oapw (AirPlay und das C++ Web-Interface sind danach sofort wieder aktiv).

<EOF>
