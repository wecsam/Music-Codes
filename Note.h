/*
	This class shall represent a single note in music.
*/
#ifndef INCLUDE_MUSIC_CODES_NOTE
#define INCLUDE_MUSIC_CODES_NOTE 1
#include <cstdint>
namespace MusicCodes {
	class Note {
	public:
		Note(uint8_t pitch, int8_t duration);
		Note(const Note& rhs);
		Note(Note&& rhs);
		uint8_t getPitch() const;
		int8_t getDuration() const;
	private:
		// The MIDI pitch number of this note
		uint8_t pitch;
		// The duration of this note, expressed as an exponent of 2.
		// For example, for a quarter note, duration=-2 because 2^(-2)=1/4.
		int8_t duration;
	};
}
#endif
