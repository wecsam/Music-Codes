#include <cmath>
#include "Note.h"
namespace MusicCodes {
	Note::Note(uint8_t pitch, int duration, int dots, double start) : pitch(pitch), duration(duration), dots(dots), start(start) {}
	uint8_t Note::getPitch() const {
		return pitch;
	}
	int Note::getDuration() const {
		return duration;
	}
	int Note::getDots() const {
		return dots;
	}
	double Note::getStart() const {
		return start;
	}
	Note::operator bool() const {
		return pitch <= 127 && dots >= 0;
	}
	Note Note::InvalidNote(){
		return {255, 0, -1};
	}
	const std::string Note::NOTE_NAMES[] = {
		"C",
		"C#",
		"D",
		"D#",
		"E",
		"F",
		"F#",
		"G",
		"G#",
		"A",
		"A#",
		"B"
	};
	std::ostream& operator<<(std::ostream& os, const Note& rhs){
		if(!rhs){
			os << "invalid note";
		}else{
			os << Note::NOTE_NAMES[rhs.pitch % 12] << rhs.pitch / 12 << ' ';
			switch(rhs.dots){
				case 0:
					break;
				case 1:
					os << "dotted ";
					break;
				case 2:
					os << "double-dotted ";
					break;
				case -1:
					os << "negative-dotted ";
					break;
				default:
					os << rhs.dots << "-times-dotted ";
			}
			switch(-rhs.duration){
				case 0:
					os << "whole";
					break;
				case 1:
					os << "half";
					break;
				case 2:
					os << "quarter";
					break;
				case 3:
					os << "eighth";
					break;
				case 4:
					os << "sixteenth";
					break;
				case 5:
					os << "thirty-second";
					break;
				default:
					os << pow(2, -rhs.duration) << "th";
			}
			os << " note";
		}
		return os;
	}
}
