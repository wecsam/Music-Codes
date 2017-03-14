#include <algorithm>
#include <cmath>
#include <cstring>
#include "MidiReader.h"
using namespace std;
namespace MusicCodes {
	// MidiReader
	MidiReader::MidiReader(istream& input) : input(input), currentTrack(NULL) {
		// Make sure that this is a MIDI file.
		midiValid = false;
		// Read the first four bytes and check for the beginning of a MIDI header chunk.
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
	MidiReader::~MidiReader(){
		delete currentTrack;
	}
	Note MidiReader::getNextNote(){
		// If we are not in a track or at the end of the current track, move to the next track.
		if(!currentTrack || currentTrack->endOfData()){
			delete currentTrack;
			while(true){
				if(!input){
					// One of the internal state flags of the istream is set. We probably have
					// reached the end of the file.
					return Note::InvalidNote();
				}
				currentTrack = new Track(this);
				if(currentTrack){
					break;
				}else{
					// It's an alien chunk. Skip it by reading the next four bytes, interpreting
					// them as the length of this chunk, and then skipping that number of bytes.
					skipBytes(getValue<uint32_t>());
				}
			}
		}
		return currentTrack->getNextNote();
	}
	unsigned int MidiReader::getTicksPerQuarterNote(uint32_t microsecondsPerQuarterNote){
		// If midiDivision is negative, it is in SMPTE format.
		if(midiDivision < 0){
			// The upper byte is the negative SMPTE format in two's-complement form.
			int8_t framesPerSecond = *((int8_t*)&midiDivision);
			// The lower byte is the number of ticks per frame.
			uint8_t ticksPerFrame = *((uint8_t*)&midiDivision + 1);
			// [T/F] * [F/S] = [T/S]
			// [T/S] / [uS/S] = [T/uS] (this step was moved to the end to improve precision)
			// [T/uS] * [uS/B] = [T/B]
			return ticksPerFrame * -framesPerSecond * microsecondsPerQuarterNote / 1000000;
		}
		// It's already stored as ticks per quarter note! That makes things easy.
		return midiDivision;
	}
	MidiReader::operator bool() const {
		return midiValid;
	}
	streampos MidiReader::tellg(){
		return input.tellg();
	}
	template <class T>
	T MidiReader::getValue(){
		char buffer[sizeof(T)];
		input.read(buffer, sizeof(T));
		reverse(buffer, buffer + sizeof(T));
		return *(T*)buffer;
	}
	unsigned int MidiReader::getVariableLengthValue(){
		unsigned int result = 0;
		unsigned char nextByte;
		do {
			if(!input){
				break;
			}
			nextByte = input.get();
			// Copy the lower 7 bits into result.
			// MIDI stores the values in the lower 7 bits of each byte.
			// This bitshift essentially concatenates groups of seven bits.
			result = (result << 7) | (nextByte & 0x7F);
			// If the most significant bit is 0, then this is the least significant byte.
			// if this is the last significant byte, then stop reading more bytes; they
			// are not a part of this variable-length value.
		} while(nextByte & 0x80);
		return result;
	}
	void MidiReader::skipBytes(streamoff n){
		input.seekg(n, ios_base::cur);
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
	// MidiReader::Track
	MidiReader::Track::Track(MidiReader* file) : file(file), ns(this) {
		trackValid = false;
		sawTrackEnd = false;
		sequenceNumber = 0;
		lastSeenTempo = 500000;
		lastSeenTimeSignature = NULL;
		lastSeenKeySignature = NULL;
		lastSeenEventType = 0;
		runningTime = 0;
		// Make sure that this is a MIDI track.
		// Read the first four bytes and check for the beginning of a MIDI track.
		char buffer[5];
		file->input.read(buffer, 4);
		buffer[4] = 0;
		if(strcmp(buffer, "MTrk") == 0){
			// Read the length of the MIDI track.
			lengthMTrk = file->getValue<uint32_t>();
			// Remember the current position of the istream.
			// When (file->input.tellg() - streamPositionStart) == lengthMTrk,
			// we have reached the end of the data for this track.
			streamPositionStart = file->input.tellg();
			// Read through the entire track.
			do {
				auto e = handleNextEvent();
				if(e == NUM_EVENTS){
					// An unknown event was seen.
					break;
				}
			} while(!sawTrackEnd);
			// Check whether the end of the track was seen.
			trackValid = sawTrackEnd;
		}
	}
	MidiReader::Track::~Track(){
		delete lastSeenTimeSignature;
		delete lastSeenKeySignature;
	}
	MidiReader::Track::operator bool() const {
		return trackValid;
	}
	MidiReader::Track::Event MidiReader::Track::handleNextEvent(){
		if(endOfData()){
			return NUM_EVENTS;
		}
		// Get the amount of time since the last event. Remember, MIDI uses a unit
		// of time called a delta, and the actual duration of a delta is defined in
		// the MIDI header. It can be referenced as file->midiDivision.
		unsigned int timeSinceLastEvent = file->getVariableLengthValue();
		runningTime += timeSinceLastEvent;
		// Get the next byte, which usually indicates the type of the event, without extracting it.
		unsigned char eventType = file->input.peek();
		if(!file->input){
			return NUM_EVENTS;
		}
		// If we're in running status, then this byte will not be a valid status message.
		// We should use the last received status.
		if(eventType < 0x80){
			// This was not a valid status message.
			// Use the last status message.
			eventType = lastSeenEventType;
		}else{
			// This was a valid status message.
			// Save this status message.
			lastSeenEventType = eventType;
			// Go ahead and extract the character from the stream.
			file->skipBytes(1);
		}
		switch(eventType){
			case 0xFF: {
				// Read the next byte, which indicates the type of meta.
				unsigned char metaType = file->input.get();
				// The next data is the length of the meta event.
				unsigned int metaLength = file->getVariableLengthValue();
				// Although there are many possible types of meta events,
				// only some are handled below.
				switch(metaType){
					case 0x00:
						// Sequence Number
						// The length should be two bytes.
						if(metaLength == sizeof(uint16_t)){
							// The length is correct.
							sequenceNumber = file->getValue<uint16_t>();
						}else{
							// The length is incorrect.
							file->skipBytes(metaLength);
						}
						break;
					case 0x03: {
						// Sequence/Track Name
						// Read next metaLength bytes as a string.
						char buffer[metaLength + 1];
						file->input.read(buffer, metaLength);
						buffer[metaLength] = 0;
						// Invoke string& operator= (const char* s).
						name = buffer;
						break;
					}
					case 0x2F:
						// End of Track
						sawTrackEnd = true;
						file->skipBytes(metaLength);
						break;
					case 0x51:
						// Tempo
						// The length should be three bytes.
						if(metaLength == 3){
							// The length is correct.
							// Copy each byte into lastSeenTempo in Big Endian order.
							*((char*)&lastSeenTempo) = 0;
							*((char*)&lastSeenTempo + 1) = file->input.get();
							*((char*)&lastSeenTempo + 2) = file->input.get();
							*((char*)&lastSeenTempo + 3) = file->input.get();
						}else{
							// The length is incorrect.
							file->skipBytes(metaLength);
						}
						break;
					case 0x58:
						// Time Signature
						// The length should be four bytes.
						if(metaLength == 4){
							// The length is correct.
							// Destroy the last time signature.
							delete lastSeenTimeSignature;
							// Save the new time signature. The constructor reads four bytes.
							lastSeenTimeSignature = new TimeSignature(file->input);
						}else{
							// The length is incorrect.
							file->skipBytes(metaLength);
						}
						break;
					case 0x59:
						// Key Signature
						// The length should be two bytes.
						if(metaLength == 2){
							// The length is correct.
							// Destroy the last time signature.
							delete lastSeenKeySignature;
							// Save the new time signature. The constructor reads two bytes.
							lastSeenKeySignature = new KeySignature(file->input);
						}else{
							// The length is incorrect.
							file->skipBytes(metaLength);
						}
						break;
					default:
						// We are not interested in this type of meta event.
						// Just scan to the end of the message.
						file->skipBytes(metaLength);
				}
				return META_EVENT;
			}
			case 0xF7:
			case 0xF0:
				// We're not doing anything with system exclusive events for now.
				// Scan until the End-of-Exclusive event.
				while(file->input.get() != 0xF7);
				return SYSEX_EVENT;
			default:
				// For some events, the lower 4 bits represent the channel number.
				// The higher 4 bits represent the type of event.
				// Check for these events.
				switch(eventType & 0xF0){
					case 0x80: {
						// Note Off
						// Read the pitch value from the next byte.
						uint8_t pitch = file->input.get();
						// Ignore velocity.
						file->skipBytes(1);
						// Save this note.
						ns.handleNoteOff(eventType & 0x0F, runningTime, pitch);
						return NOTE_OFF_EVENT;
					}
					case 0x90: {
						// Note On
						// Read the pitch and velocity values from the next two bytes.
						uint8_t pitch = file->input.get();
						uint8_t velocity = file->input.get();
						// If the velocity is zero, then this should be considered as the end of a note.
						if(velocity == 0){
							ns.handleNoteOff(eventType & 0x0F, runningTime, pitch);
							return NOTE_OFF_EVENT;
						}
						// Otherwise, just treat this as the start of a note.
						ns.handleNoteOn(eventType & 0x0F, runningTime, pitch);
						return NOTE_ON_EVENT;
					}
					//case 0xA0:
						// Poly Key Pressure
					case 0xB0:
						// This is a channel event.
						// Just skip over the next two bytes.
						file->skipBytes(2);
						return CHANNEL_EVENT;
					case 0xC0:
						// This is a program change. 
						// Skip one byte.
						file->skipBytes(1);
						return PROGRAM_CHANGE;
					//case 0xD0:
						// Channel Pressure
					//case 0xF0:
						// Pitch Bend
				}
		}
		// Return NUM_EVENTS to indicate that the event type was unknown.
		return NUM_EVENTS;
	}
	bool MidiReader::Track::endOfData() const {
		return sawTrackEnd && ns.numNotesRemaining() == 0;
	}
	Note MidiReader::Track::getNextNote(){
		return ns.getNextNote();
	}
	// MidiReader::Track::TimeSignature
	MidiReader::Track::TimeSignature::TimeSignature(istream& is){
		numerator = is.get();
		denominatorLog2 = is.get();
		clocksPerMetronomeTick = is.get();
		demisemiquaversPerQuarterNote = is.get();
	}
	// MidiReader::Track::KeySignature
	MidiReader::Track::KeySignature::KeySignature(istream& is){
		numSharpsFlats = is.get();
		minor = is.get();
	}
	// MidiReader::Track::NoteSequence
	MidiReader::Track::NoteSequence::NoteSequence(MidiReader::Track* parent) : parent(parent) {}
	void MidiReader::Track::NoteSequence::handleNoteOn(channel_t midiChannel, time_delta_t ticksSinceBeginningOfTrack, pitch_t p){
		// First, check whether a note of the same pitch is currently on. If there is, end it first.
		handleNoteOff(midiChannel, ticksSinceBeginningOfTrack, p);
		// Add this note to the map of notes that are on.
		// The emplace() method requires C++ 2011.
		notesThatAreOn[midiChannel][p] = ticksSinceBeginningOfTrack;
	}
	void MidiReader::Track::NoteSequence::handleNoteOff(channel_t midiChannel, time_delta_t ticksSinceBeginningOfTrack, pitch_t p){
		// Find the time that this note was turned on. Ignore this note if it is not on.
		// If notesThatAreOn[midiChannel] hasn't been created, it'll be empty when created.
		auto on = notesThatAreOn[midiChannel].find(p);
		if(on != notesThatAreOn[midiChannel].end()){
			// The note is currently turned on.
			// Get ratio of this note length to a quarter note. For example, an eighth note gets a ratio
			// of 0.5 because it is half of a quarter note. Round to the nearest multiple of the shortest note.
			double fractionOfQuarterNote = round(
				(double)(ticksSinceBeginningOfTrack - on->second) /            // Number of ticks since beginning of note
				parent->file->getTicksPerQuarterNote(parent->lastSeenTempo) /  // Number of ticks per quarter note
				SHORTEST_NOTE
			) * SHORTEST_NOTE;
			// Before running this through the logarithm function, we have to make sure that it isn't zero or negative.
			if(fractionOfQuarterNote > 0){
				// Figure out how many dots the note should have. Recall that a dot in music increases the note length by 50%.
				int dots = 0;
				double wholePart;
				while(true){
					// Find the logarithm base 2 of fractionOfQuarterNote and split it into its integer and fraction parts.
					double fractionPart = modf(log2(fractionOfQuarterNote), &wholePart);
					// If the result was close enough to a whole number, accept it.
					if(abs(fractionPart) < 0.1){
						break;
					}
					// Musically remove a dot.
					fractionOfQuarterNote /= 1.5;
					++dots;
				}
				// Subtract two from the log to convert from quarter=1 to whole=1.
				wholePart -= 2.0;
				// Add this to the priority_queue of notes.
				// If another note that started after this one but finished before this one is already in the priority_queue,
				// this note will move up ahead of it (because that's how a priority_queue works).
				// The emplace() method requires C++ 2011.
				pastNotes.emplace(midiChannel, ticksSinceBeginningOfTrack, Note(p, wholePart, dots));
			}
			// Erase the note from the map of notes that are on.
			notesThatAreOn[midiChannel].erase(on);
		}
	}
	Note MidiReader::Track::NoteSequence::getNextNote(){
		if(pastNotes.size()){
			auto result = pastNotes.top();
			pastNotes.pop();
			return result.theNote;
		}
		return Note::InvalidNote();
	}
	size_t MidiReader::Track::NoteSequence::numNotesRemaining() const {
		return pastNotes.size();
	}
	MidiReader::Track::NoteSequence::NoteSequenceNote::NoteSequenceNote(channel_t channel, time_delta_t startTime, Note&& theNote)
	: channel(channel), startTime(startTime), theNote(move(theNote)) {}
	bool MidiReader::Track::NoteSequence::NoteSequenceNoteCompare::operator()(const NoteSequenceNote& lhs, const NoteSequenceNote& rhs){
		// We want the priority_queue to bring notes with earlier start times to the top of the heap.
		return lhs.startTime > rhs.startTime;
	}
}
