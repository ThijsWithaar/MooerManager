#pragma once

#include <array>
#include <span>
#include <string>

#define NOMINMAX
#include <windows.h>


// This is the API for teVirtualMIDI64.dll, installed by LoopMidi
typedef struct _VM_MIDI_PORT VM_MIDI_PORT, *LPVM_MIDI_PORT;


namespace TeVirtualMidi
{


class MooerMidiControl
{
public:
	MooerMidiControl(std::wstring portName);

	~MooerMidiControl();

private:
	static void on_read(LPVM_MIDI_PORT midiPort, LPBYTE midiDataBytes, DWORD length, DWORD_PTR dwCallbackInstance);

	void OnRead(LPVM_MIDI_PORT midiPort, std::span<std::byte> midiData);

	VM_MIDI_PORT* m_port;
	std::array<std::byte, 512> m_sysex_buffer;
};


} // namespace TeVirtualMidi
