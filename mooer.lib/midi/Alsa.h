#pragma once

#include <thread>

#include <alsa/asoundlib.h>

#include <midi/Midi.h>

namespace Alsa
{

class MidiControl : public MIDI::Interface
{
public:
	MidiControl(std::string_view name, MIDI::Callback* callback = nullptr);

	~MidiControl();

	void ControlChange(std::uint8_t channel, MIDI::ControlChange controller, std::uint8_t value) override;
	void ProgramChange(std::uint8_t channel, std::uint8_t value) override;
	void Sysex(std::uint8_t channel, MIDI::Manufacturer manufacturer, std::span<std::uint8_t> value) override;

private:
	void BackgroundWork();

	struct VirtualMidi
	{
		VirtualMidi();
		~VirtualMidi();
		snd_rawmidi_t *read, *write;
	};

	VirtualMidi m_port;
	std::vector<std::uint8_t> m_sysexBuffer;
	std::jthread m_worker;
};

} // namespace Alsa