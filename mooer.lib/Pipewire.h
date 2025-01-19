#pragma once

#include <string_view>

#include <spa/pod/builder.h>

#include <Midi.h>


namespace PipeWire
{


class MooerMidiControl : public MIDI::Interface
{
public:
	MooerMidiControl(std::string_view name, MIDI::Callback* callback = nullptr);

	~MooerMidiControl();

	// MIDI::Sink

	void ControlChange(std::uint8_t channel, MIDI::ControlChange controller, std::uint8_t value) override;

	void ProgramChange(std::uint8_t channel, std::uint8_t value) override;

	void Sysex(std::uint8_t channel, MIDI::Manufacturer manufacturer, std::span<std::uint8_t> values) override;

public:
	void OnProcess(struct spa_io_position* position);

	void OnSignal(int signal_number);

	static void on_process(void* userdata, struct spa_io_position* position);

private:
	static void do_quit(void* userdata, int signal_number);

	void SendMidi(uint64_t sample_offset, std::span<const std::uint8_t> data);

	struct pw_main_loop* m_loop;
	struct pw_filter* m_filter;
	struct port* m_InputPort;
	struct spa_pod_builder m_builder;
	std::vector<std::uint8_t> m_sysexBuffer;
};

} // namespace PipeWire
