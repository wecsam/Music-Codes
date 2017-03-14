#include <algorithm>
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
	unsigned int MidiReader::getVariableLengthValue(){
		unsigned int result = 0;
		unsigned char nextByte;
		do {
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
	MidiReader::Track::Track(MidiReader* file)
	: file(file), sequenceNumber(0), lastSeenTempo(500000), lastSeenTimeSignature(NULL)
	{
		// Make sure that this is a MIDI track.
		// Read the first four bytes and check for the beginning of a MIDI track.
		char buffer[5];
		file->input.read(buffer, 4);
		buffer[4] = 0;
		trackValid = (strcmp(buffer, "MTrk") == 0);
		if(trackValid){
			// Read the length of the MIDI track.
			lengthMTrk = file->getValue<uint32_t>();
			// Remember the current position of the istream.
			// When (file->input.tellg() - streamPositionStart) == lengthMTrk,
			// we have reached the end of the data for this track.
			streamPositionStart = file->input.tellg();
		}
	}
	MidiReader::Track::~Track(){
		delete lastSeenTimeSignature;
		delete lastSeenKeySignature;
	}
	MidiReader::Track::operator bool() const {
		return trackValid;
	}
	MidiReader::Track::EVENT MidiReader::Track::handleNextEvent(){
		// Get the amount of time since the last event. Remember, MIDI uses a unit
		// of time called a delta, and the actual duration of a delta is defined in
		// the MIDI header. It can be referenced as file->midiDivision.
		unsigned int timeSinceLastEvent = file->getVariableLengthValue();
		// Read the next byte, which indicates the type of the event.
		switch(file->input.get()){
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
			case 0xF0:
			case 0xF7:
				// We're not doing anything with system exclusive events for now.
				// Scan until the End-of-Exclusive event.
				while(file->input.get() != 0xF7);
				return SYSEX_EVENT;
		}
		if(timeSinceLastEvent == 0){
			
		}
		// Return NUM_EVENTS to indicate that the event type was unknown.
		return NUM_EVENTS;
	}
	bool MidiReader::Track::endOfData() const {
		return (file->input.tellg() - streamPositionStart) == lengthMTrk;
	}
	// MidiReader::Track::TimeSignature
	MidiReader::Track::TimeSignature::TimeSignature(istream& is){
		numerator = is.get();
		denominatorLog2 = is.get();
		clocksPerMetronomeTick = is.get();
		demisemiquaversPer24Clocks = is.get();
	}
	// MidiReader::Track::KeySignature
	MidiReader::Track::KeySignature::KeySignature(istream& is){
		numSharpsFlats = is.get();
		minor = is.get();
	}
}
