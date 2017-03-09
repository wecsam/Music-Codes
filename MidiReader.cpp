#include <algorithm>
#include <cstring>
#include "MidiReader.h"
using namespace std;
namespace MusicCodes {
	MidiReader::MidiReader(istream& input) : input(input) {
		// Make sure that this is a MIDI file.
		midiValid = false;
		// Read the first four bytes and check for the MIDI header chunk.
		char buffer[5];
		input.read(buffer, 4);
		buffer[4] = 0;
		if(strcmp(buffer, "MThd") == 0){
			// Check that the header is exactly six bytes long.
			lengthMThd = getValue<uint32_t>();
			if(lengthMThd == 6){
				// Get the format of the MIDI file.
				uint16_t f = getValue<uint16_t>();
				// Check that it is one of the formats specified in MidiReader.h.
				if(f < NUM_FORMATS){
					midiFormat = static_cast<FORMAT>(f);
					// Get the number of tracks that follow the header.
					midiNumTracks = getValue<uint16_t>();
					// Get the timing division.
					midiDivision = getValue<int16_t>();
					// ...and we're finally done.
					midiValid = true;
				}
			}
		}
	}
	MidiReader::operator bool() const {
		return midiValid;
	}
	template <class T>
	T MidiReader::getValue(){
		char buffer[sizeof(T)];
		input.read(buffer, sizeof(T));
		reverse(buffer, buffer + sizeof(T));
		return *(T*)buffer;
	}
	ostream& operator<<(ostream& lhs, const MidiReader& rhs){
		lhs << "<MidiReader: valid=" << rhs.midiValid << ", format=";
		switch(rhs.midiFormat){
			case MidiReader::FORMAT::SINGLE_TRACK:
				lhs << "single track";
				break;
			case MidiReader::FORMAT::MULTI_TRACK:
				lhs << "multi-track";
				break;
			case MidiReader::FORMAT::MULTI_SONG:
				lhs << "multi-song";
				break;
			default:
				lhs << "unknown";
		}
		lhs << ", tracks=" << rhs.midiNumTracks << ", division=" << rhs.midiDivision << ">";
		return lhs;
	}
}
