/*
	This class will accept an istream and interpret its data as MIDI data.
	At this time, only musical notes will be read from the file.
	
	Some excellent MIDI references:
	http://www.ccarh.org/courses/253/handout/smf/
	http://cs.fit.edu/~ryan/cse4051/projects/midi/midi.html
*/
#ifndef INCLUDE_MUSIC_CODES_MIDIREADER
#define INCLUDE_MUSIC_CODES_MIDIREADER 1
#include <iostream>
#include "Note.h"
namespace MusicCodes {
	class MidiReader {
		friend std::ostream& operator<<(std::ostream&, const MidiReader&);
	public:
		MidiReader(std::istream& input);
		// TODO: copy and move constructors
		MusicCodes::Note getNextNote();
		operator bool() const;
		enum FORMAT { SINGLE_TRACK, MULTI_TRACK, MULTI_SONG, NUM_FORMATS };
	private:
		std::istream& input;
		// Whether the input istream contained a valid MIDI header
		bool midiValid;
		// The length of the MIDI header
		uint32_t lengthMThd;
		// The MIDI format (single track, multi track, or multi song)
		FORMAT midiFormat;
		// The number of track chunks that follow the header chunk
		uint16_t midiNumTracks;
		// The unit of time that this MIDI file uses, as ticks per beat
		int16_t midiDivision;
		// Reads the next sizeof(T) bytes from the istream and returns them as type T
		template <class T> T getValue();
	};
}
#endif
