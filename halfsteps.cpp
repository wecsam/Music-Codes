#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include "Note.h"
#include "MidiReader.h"
using namespace std;
using namespace MusicCodes;
int main(int argc, char** argv){
	// This program expects file paths to be passed in as arguments.
	// Check whether any arguments were passed in.
	if(argc < 2){
		cout << "Half Steps by David Tsai\n"
			<< "This program reads a MIDI file and then prints out the number of half steps\n"
			<< "between every note. If the MIDI file has N notes, then N-1 numbers will be\n"
			<< "printed.\n"
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
		cout << "MIDI: " << midiread << endl;
		// Handle the first note.
		Note n = midiread.getNextNote();
		int count = 1;
		cout << setw(4) << count << '.' << ' ' << n << endl;
		// Create a vector to hold the intervals between notes.
		auto lastPitch = n.getPitch();
		vector<int> intervals;
		// Iterate over the other notes.
		while((n = midiread.getNextNote())){
			// Print the note.
			cout << setw(4) << ++count << '.' << ' ' << n << endl;
			// Save the interval from the last pitch.
			intervals.push_back(n.getPitch() - lastPitch);
			lastPitch = n.getPitch();
		}
		// Print out the intervals as numbers of halfs steps.
		cout << "Sequence of half steps:";
		for(int s : intervals){
			cout << ' ' << s;
		}
		cout << endl;
	}
	return numFailures;
}
