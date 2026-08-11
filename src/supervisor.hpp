/*
Definiert die Schnittstelle des Supervisors, damit andere Teile ihn starten oder Status abfragen können.
*/
#pragma once

// Startet das gesamte Audio-System: legt die Pipes an, startet Worker,
// Splitter und Reassembler und wartet, bis alles fertig ist.
// Rückgabewert ist wie üblich 0 = Erfolg, sonst Fehler.
int runAudioSystem();
