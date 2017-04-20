/*
	MIDI to AHK for Revel Piano Time
	
	Copyright (c) 2017 by David Tsai. All rights reserved.
	
	This program reads a MIDI file and generates an AutoHotkey script that
	presses keys in Piano Time by Revel Software to play the music. You're
	asking why we wouldn't just play the MIDI file directly. I don't know.
	I'm just making this for fun. Maybe I will record my screen while some
	music is playing and put it on YouTube.
*/
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "MidiReader.h"
#include "Note.h"
using namespace std;
using namespace MusicCodes;

// In Piano Time (and in most music applications), the octave starts on a C.
// Three octaves are playable by pressing keys on the keyboard, and the 
// octaves can be adjusted by changing the Keyboard Start Octave option on the
// toolbar. The keys for the 36 playable notes (3 octaves) are in MIDI_TO_KEY.
// MIDI_TO_KEY[0] is a C. All of the notes in the music must be in the next
// 36 half steps.
const char MIDI_TO_KEY[] = "z1x2cv3b4n5ma6s7df8g9h0jqiwoerptkylu";

int main(int argc, char** argv){
	// This program expects file paths to be passed in as arguments.
	// Check whether any arguments were passed in.
	if(argc < 2){
		cout << "MIDI to AHK for Revel Piano Time by David Tsai\n\n"
			<< "This program reads a MIDI file and generates an AutoHotkey script that\n"
			<< "presses keys in Piano Time by Revel Software to play the music. The\n"
			<< "generated AutoHotkey script will be placed in the same directory as\n"
			<< "the input MIDI file.\n\n"
			<< "Pass in one or more paths to MIDI files." << endl;
		return 0;
	}
	// Loop through the arguments.
	int numFailures = 0;
	for(int i = 1; i < argc; ++i){
		// Print out the argument so that the user knows which one is being processed.
		if(argc > 2){
			if(i > 1){
				cout << '\n';
			}
			cout << "File: " << argv[i] << endl;
		}
		// Open the file.
		ifstream midifile(argv[i]);
		if(!midifile){
			cerr << "This file could not be opened.\n";
			++numFailures;
			continue;
		}
		// Read the MIDI data.
		MidiReader midiread(midifile);
		if(!midiread){
			cerr << "This is not a supported MIDI file.\n";
			++numFailures;
			continue;
		}
		// Print out a summary of the MIDI file.
		cout << "MIDI: " << midiread << '\n';
		// Read the notes into a vector so that we can iterate over them multiple times.
		// During the first iteration, also find the highest and lowest notes.
		vector<Note> notes;
		Note n = Note::InvalidNote();
		uint8_t lowestNote = 0xff, highestNote = 0;
		while((n = midiread.getNextNote())){
			cout << (unsigned int)n.getPitch() << '\t' << n.getStart() << '\n';
			if(n.getPitch() < lowestNote){
				lowestNote = n.getPitch();
			}
			if(n.getPitch() > highestNote){
				highestNote = n.getPitch();
			}
			notes.push_back(move(n));
		}
		// Get the C below the lowest note and the C above the highest note.
		// If the lowest note is a C, then it does not need to be adjusted.
		// if the highest note is a C, however, the range has to be extended up an octave.
		// That's because 36 half steps above a C is a B.
		lowestNote -= lowestNote % 12;
		highestNote += 12 - (highestNote % 12) - 1;
		if(highestNote - lowestNote >= 36){
			cerr << "The range of this track is more than three octaves (from "
				<< (unsigned int)lowestNote << " to " << (unsigned int)highestNote << ").\n";
			continue;
		}
		// Create a new AHK file.
		ofstream ahk(string(argv[i]) + ".ahk");
		if(!ahk){
			cerr << "The output file could not be opened.\n";
			++numFailures;
			continue;
		}
		ahk << "; Open Piano Time by Revel Software, wait for it to load, and switch to it.\n"
			<< "Run, C:\\Windows\\System32\\cmd.exe /c \"C:\\Windows\\explorer.exe shell:appsFolder\\RevelSoftware.PianoTimePro_rm1v733ay04k0!App\"\n"
			<< "WinWait, Piano Time Pro\n"
			<< "WinActivate\n"
			<< "; Make sure that this thread will not be interrupted.\n"
			<< "Critical, 50\n"
			<< "; Store the current number of milliseconds since the computer booted.\n"
			<< "DllCall(\"QueryPerformanceFrequency\", \"Int64*\", PerformanceFrequency)\n"
			<< "DllCall(\"QueryPerformanceCounter\", \"Int64*\", StartTime)\n"
			<< "; For every note, wait until its start time and then send the keystroke.\n";
		for(Note& n : notes){
			ahk << "; " << n.getStart() << " seconds: " << n << '\n'
				<< "Loop\n"
				<< "{\n"
				<< "\tDllCall(\"QueryPerformanceCounter\", \"Int64*\", CurrentTime)\n"
				<< "\tIf (CurrentTime - StartTime) / PerformanceFrequency >= " << n.getStart() << '\n'
				<< "\t\tbreak\n"
				<< "}\n"
				<< "SendInput, " << MIDI_TO_KEY[n.getPitch() - lowestNote] << '\n';
		}
		ahk << "; Let the user know that this script has completed.\n"
			<< "Sleep, 1000\n"
			<< "MsgBox, Done.\n";
		cout << "Script generation complete. The start octave should be set to " << lowestNote / 12 - 1 << '.' << endl;
	}
	return numFailures;
}
