#pragma once

#include <jack/jack.h>
#include <jack/midiport.h>

#include <midi/Midi.h>


namespace Jack
{

class MooerMidiControl : public MIDI::Interface
{
public:
	MooerMidiControl(std::string_view name, MIDI::Callback* callback = nullptr);

	MooerMidiControl(jack_client_t* client, MIDI::Callback* callback);

	~MooerMidiControl() override;

	void ControlChange(std::uint8_t channel, MIDI::ControlChange controller, std::uint8_t value) override;

	void ProgramChange(std::uint8_t channel, std::uint8_t value) override;

	void Sysex(std::uint8_t channel, MIDI::Manufacturer manufacturer, std::span<std::uint8_t> values) override;

private:
	/// Helper function to convert jack-callback to member-function-call
	static int staticProcess(jack_nframes_t nframes, void* arg);

	void process(jack_nframes_t nframes);

	jack_client_t* m_client;
	jack_port_t* m_port;

	std::array<std::uint8_t, 32> m_txbuf;
};

} // namespace Jack