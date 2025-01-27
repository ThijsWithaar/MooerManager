/**

Connect to Tobias Erichsen's VirtualMidi driver
	https://www.tobias-erichsen.de/software/virtualmidi/virtualmidi-sdk.html

IOCTL codes:
	0x80012004 - Get process ID's over other processes.

Data-structure in teVirtualMIDI64.dll:

struct VM_MIDI_PORT
{
	PHANDLE driver;
	PHANDLE thread;
	DWORD thread_id;
	int nr_bytes_to_read;
	OVERLAPPED write;
	OVERLAPPED read;
	OVERLAPPED ioctl;
	HGLOBAL rx_buf;
	HGLOBAL tx_buf;
	PVOID callback;
	PVOID callback_self;
	PVOID rx_sysex_parser;
	PVOID tx_sysex_parser;
	PVOID portname;
	...
};
*/

#include <array>
#include <cassert>
#include <span>
#include <string>

#define NOMINMAX
#include <windows.h>

// This is the API for teVirtualMIDI64.dll, installed by LoopMidi

extern "C"
{

/* TE_VM_FLAGS_PARSE_RX tells the driver to always provide valid preparsed MIDI-commands either via Callback or via
 * virtualMIDIGetData */
#define TE_VM_FLAGS_PARSE_RX (1)
/* TE_VM_FLAGS_PARSE_TX tells the driver to parse all data received via virtualMIDISendData */
#define TE_VM_FLAGS_PARSE_TX (2)
/* TE_VM_FLAGS_INSTANTIATE_RX_ONLY - Only the "midi-out" part of the port is created */
#define TE_VM_FLAGS_INSTANTIATE_RX_ONLY (4)
/* TE_VM_FLAGS_INSTANTIATE_TX_ONLY - Only the "midi-in" part of the port is created */
#define TE_VM_FLAGS_INSTANTIATE_TX_ONLY (8)
/* TE_VM_FLAGS_INSTANTIATE_BOTH - a bidirectional port is created */
#define TE_VM_FLAGS_INSTANTIATE_BOTH (12)

typedef struct _VM_MIDI_PORT VM_MIDI_PORT, *LPVM_MIDI_PORT;

typedef void(CALLBACK* LPVM_MIDI_DATA_CB)(LPVM_MIDI_PORT midiPort,
										  LPBYTE midiDataBytes,
										  DWORD length,
										  DWORD_PTR dwCallbackInstance);

LPVM_MIDI_PORT CALLBACK virtualMIDICreatePortEx3(LPCWSTR portName,
												 LPVM_MIDI_DATA_CB callback,
												 DWORD_PTR dwCallbackInstance,
												 DWORD maxSysexLength,
												 DWORD flags,
												 GUID* manufacturer,
												 GUID* product);

void CALLBACK virtualMIDIClosePort(LPVM_MIDI_PORT midiPort);

BOOL CALLBACK virtualMIDISendData(LPVM_MIDI_PORT midiPort, LPBYTE midiDataBytes, DWORD length);
}


#include <midi/TeVirtualMidi.h>


namespace TeVirtualMidi
{


MooerMidiControl::MooerMidiControl(std::wstring portName, MIDI::Callback* callback)
	: MIDI::Interface(callback)
{
	DWORD flags = TE_VM_FLAGS_PARSE_RX;
	m_port = virtualMIDICreatePortEx3(
		portName.c_str(), &MooerMidiControl::on_read, (DWORD_PTR)this, m_sysex_buffer.size(), flags, nullptr, nullptr);
}


MooerMidiControl::~MooerMidiControl()
{
	virtualMIDIClosePort(m_port);
}


void MooerMidiControl::ControlChange(std::uint8_t channel, MIDI::ControlChange controller, std::uint8_t value)
{
}


void MooerMidiControl::ProgramChange(std::uint8_t channel, std::uint8_t value)
{
}


void MooerMidiControl::Sysex(std::uint8_t channel, MIDI::Manufacturer manufacturer, std::span<std::uint8_t> values)
{
}


void MooerMidiControl::on_read(LPVM_MIDI_PORT midiPort,
							   LPBYTE midiDataBytes,
							   DWORD length,
							   DWORD_PTR dwCallbackInstance)
{
	std::span<std::byte> data{(std::byte*)midiDataBytes, length};
	return reinterpret_cast<MooerMidiControl*>(dwCallbackInstance)->OnRead(midiPort, data);
}


void MooerMidiControl::OnRead(LPVM_MIDI_PORT midiPort, std::span<std::byte> midiData)
{
	assert(midiPort == m_port);
}


} // namespace TeVirtualMidi
