#pragma once

#include <string_view>

#include <pipewire/filter.h>
#include <pipewire/port.h>
#include <pipewire/thread-loop.h>
#include <spa/pod/builder.h>

#include <midi/Midi.h>


namespace PipeWire
{

class PipeWire
{
public:
	PipeWire(int argc, char* argv[]);
	~PipeWire();
};

/**
Event handler that starts it's own background thread
 */
class ThreadLoop
{
public:
	class Lock
	{
	public:
		Lock(pw_thread_loop* parent);
		~Lock();

		Lock(const Lock&) = delete;
		Lock& operator=(const Lock&) = delete;

	private:
		pw_thread_loop* m_loop;
	};

	ThreadLoop(const char* name, const struct spa_dict* props);

	~ThreadLoop();

	void Quit();

	Lock GetLock();

	operator pw_loop*();

private:
	pw_thread_loop* m_loop;
};


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
	struct Port
	{
	};

	static void do_quit(void* userdata, int signal_number);

	void SendMidi(uint64_t sample_offset, std::span<const std::uint8_t> data);

	PipeWire m_pw;
	ThreadLoop m_loop;
	pw_filter* m_filter;
	Port *m_portIn, *m_portOut;
	spa_pod_builder m_builder;
	std::vector<std::uint8_t> m_sysexBuffer;
};

} // namespace PipeWire
