#include "Note.h"
namespace MusicCodes {
	Note::Note(uint8_t pitch, int8_t duration) : pitch(pitch), duration(duration) {}
	Note::Note(const Note& rhs) : pitch(rhs.pitch), duration(rhs.duration) {}
	Note::Note(Note&& rhs) : pitch(rhs.pitch), duration(rhs.duration) {}
	uint8_t Note::getPitch() const {
		return pitch;
	}
	int8_t Note::getDuration() const {
		return duration;
	}
}
