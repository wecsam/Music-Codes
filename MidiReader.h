/*
	This class will accept an istream and interpret its data as MIDI data.
	At this time, only musical notes will be read from the file.
	
	Some excellent MIDI references:
	http://www.ccarh.org/courses/253/handout/smf/
	http://cs.fit.edu/~ryan/cse4051/projects/midi/midi.html
	https://www.nyu.edu/classes/bello/FMT_files/9_MIDI_code.pdf
	
	For MIDI note numbers:
	http://www.electronics.dit.ie/staff/tscarff/Music_technology/midi/midi_note_numbers_for_octaves.htm
	
	You can also take a look at the official specification, but the website
	wants you to create an account just to download the file.
	https://www.midi.org/specifications/item/the-midi-1-0-specification
*/
#ifndef INCLUDE_MUSIC_CODES_MIDIREADER
#define INCLUDE_MUSIC_CODES_MIDIREADER 1
#include <iostream>
#include <map>
#include <queue>
#include <string>
#include "Note.h"
namespace MusicCodes {
	class MidiReader {
		friend std::ostream& operator<<(std::ostream&, const MidiReader&);
	public:
		MidiReader(std::istream& input);
		// TODO: copy and move constructors
		~MidiReader();
		Note getNextNote();
		unsigned int getTicksPerQuarterNote(uint32_t microsecondsPerQuarterNote);
		operator bool() const;
		std::streampos tellg();
		enum FORMAT { SINGLE_TRACK, MULTI_TRACK, MULTI_SONG, NUM_FORMATS };
		class Track {
		public:
			Track(MidiReader*);
			~Track();
			// TODO: copy and move constructors
			operator bool() const;
			enum Event {
				NOTE_ON_EVENT, NOTE_OFF_EVENT,
				CHANNEL_EVENT, PROGRAM_CHANGE, META_EVENT, SYSEX_EVENT,
				NUM_EVENTS
			};
			// Returns the type of event that was handled. If the return value is NUM_EVENTS,
			// then the event type was unknown. It is assumed that trackValid is true.
			Event handleNextEvent();
			// Returns whether the end of the data for this track has been reached.
			bool endOfData() const;
			// Gets the next note in the music.
			Note getNextNote();
			// Classes to represent various meta information
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
				// The number of MIDI clock ticks per metronome tick.
				uint8_t clocksPerMetronomeTick;
				// The number of 32nd notes per quarter note, regardless of time signature.
				uint8_t demisemiquaversPerQuarterNote;
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
			using pitch_t = uint8_t;
			using time_delta_t = unsigned int;
			using channel_t = uint8_t;
		private:
			bool trackValid;
			bool sawTrackEnd;
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
			// The tempo is expressed as the number of microseconds per quarter note, regardless of time signature.
			uint32_t lastSeenTempo;
			// The time signature that was last seen
			TimeSignature* lastSeenTimeSignature;
			// The key signature that was last seen
			KeySignature* lastSeenKeySignature;
			// The last event code that was seen (useful for running status encoding)
			unsigned char lastSeenEventType;
			// Total number of MIDI deltas since the beginning of the track
			unsigned int runningTime;
			// This class keeps track of notes as we read the entire track.
			class NoteSequence {
			public:
				NoteSequence(Track*);
				void handleNoteOn(channel_t midiChannel, time_delta_t ticksSinceBeginningOfTrack, pitch_t p);
				void handleNoteOff(channel_t midiChannel, time_delta_t ticksSinceBeginningOfTrack, pitch_t p);
				Note getNextNote();
				std::size_t numNotesRemaining() const;
				struct NoteSequenceNote {
					NoteSequenceNote(channel_t, time_delta_t, Note&&);
					// The MIDI channel
					channel_t channel;
					// The number of MIDI deltas since the beginning of the track
					time_delta_t startTime;
					// The actual note
					Note theNote;
				};
				static constexpr double SHORTEST_NOTE = 0.125; // 32nd note is 1/8 of a quarter note
			private:
				Track* parent;
				// Keep track of notes that have not yet been turned off.
				std::map<channel_t, std::map<pitch_t, time_delta_t>> notesThatAreOn;
				struct NoteSequenceNoteCompare {
					bool operator()(const NoteSequenceNote& lhs, const NoteSequenceNote& rhs);
				};
				// When a note is turned off, we can calculate its duration. It then goes here.
				std::priority_queue<NoteSequenceNote, std::vector<NoteSequenceNote>, NoteSequenceNoteCompare> pastNotes;
			};
			NoteSequence ns;
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
