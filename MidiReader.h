/*
	This class will accept an istream and interpret its data as MIDI data.
	At this time, only musical notes will be read from the file.
	
	Some excellent MIDI references:
	http://www.ccarh.org/courses/253/handout/smf/
	http://cs.fit.edu/~ryan/cse4051/projects/midi/midi.html
	
	For MIDI note numbers:
	http://www.electronics.dit.ie/staff/tscarff/Music_technology/midi/midi_note_numbers_for_octaves.htm
*/
#ifndef INCLUDE_MUSIC_CODES_MIDIREADER
#define INCLUDE_MUSIC_CODES_MIDIREADER 1
#include <iostream>
#include <string>
#include "Note.h"
namespace MusicCodes {
	class MidiReader {
		friend std::ostream& operator<<(std::ostream&, const MidiReader&);
	public:
		MidiReader(std::istream& input);
		// TODO: copy and move constructors
		~MidiReader();
		MusicCodes::Note getNextNote();
		operator bool() const;
		enum FORMAT { SINGLE_TRACK, MULTI_TRACK, MULTI_SONG, NUM_FORMATS };
		class Track {
		public:
			Track(MidiReader*);
			~Track();
			operator bool() const;
			enum EVENT { MIDI_EVENT, META_EVENT, SYSEX_EVENT, NUM_EVENTS };
			// Returns the type of event that was handled. If the return value is NUM_EVENTS,
			// then the event type was unknown. It is assumed that trackValid is true.
			EVENT handleNextEvent();
			// Returns whether the end of the data for this track has been reached.
			// When this is true, do not call handleNextEvent().
			bool endOfData() const;
			class TimeSignature {
			public:
				// Construct a new time signature. Pass in a reference to the MIDI file.
				// The next four bytes will be read and interpreted as the time signature.
				TimeSignature(std::istream&);
			private:
				// The time signature's numerator. Example: the numerator of 6/8 is 6.
				uint8_t numerator;
				// The log base 2 of the signature's denominator.
				// Example: the log base 2 of the denominator of 6/8 is 3 because log_2(8)=3.
				uint8_t denominatorLog2;
				// The number of MIDI clocks per metronome tick.
				uint8_t clocksPerMetronomeTick;
				// The number of 32nd notes per 24 MIDI clocks
				uint8_t demisemiquaversPer24Clocks;
			};
			class KeySignature {
			public:
				// Construct a new key signature. Pass in a reference to the MIDI file.
				// The next two bytes will be read and interpreted as the key signature.
				KeySignature(std::istream&);
			private:
				// The number of sharps or flats in the key signature.
				// A negative number indicates flats. A positive number indicates sharps.
				int8_t numSharpsFlats;
				// Minor if true. Major if false.
				bool minor;
			};
		private:
			bool trackValid;
			// A pointer to the containing MidiReader
			MidiReader* file;
			// The length of the track data
			uint32_t lengthMTrk;
			// The stream position of the istream right after the length field
			std::streampos streamPositionStart;
			// The sequence number of the track
			uint16_t sequenceNumber;
			// The name of the track
			std::string name;
			// The tempo that was last seen
			// The tempo is expressed as the number of microseconds per quarter note.
			uint32_t lastSeenTempo;
			// The time signature that was last seen
			TimeSignature* lastSeenTimeSignature;
			// The key signature that was last seen
			KeySignature* lastSeenKeySignature;
		};
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
		// The MIDI track that is currently being read (NULL if no track is being read)
		Track* currentTrack;
		// Reads the next sizeof(T) bytes from the istream and returns them as type T
		template <class T> T getValue();
		// Reads a variable-length value from the istream and returns it
		unsigned int getVariableLengthValue();
		// Skips over n bytes in the istream
		void skipBytes(std::streamoff n);
	};
}
#endif
