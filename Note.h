/*
	This class shall represent a single note in music.
*/
#ifndef INCLUDE_MUSIC_CODES_NOTE
#define INCLUDE_MUSIC_CODES_NOTE 1
#include <cstdint>
#include <iostream>
#include <string>
namespace MusicCodes {
	class Note {
		friend std::ostream& operator<<(std::ostream&, const Note&);
	public:
		Note(uint8_t pitch, int duration, int dots, double start = 0);
		uint8_t getPitch() const;
		int getDuration() const;
		int getDots() const;
		double getStart() const;
		// Whether this is a valid note
		operator bool() const;
		// Returns an invalid note
		static Note InvalidNote();
		// An array of note names
		static const std::string NOTE_NAMES[];
	private:
		// The MIDI pitch number of this note
		uint8_t pitch;
		// The duration of this note, expressed as an exponent of 2.
		// The duration is multiplied by 1.5 for every dot.
		// For example, for a quarter note, duration=-2 because 2^(-2)=1/4.
		// For example, for a dotted quarter note, duration=-2 and dots=1.
		// For example, for a double-dotted whole note, duration=0 and dots=2.
		int duration;
		int dots;
		// The number of seconds since the beginning of the MIDI track that this note was started.
		double start;
	};
}
#endif
