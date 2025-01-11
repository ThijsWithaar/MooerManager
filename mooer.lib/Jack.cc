// https://github.com/harryhaaren/JACK-MIDI-Examples/blob/master/printData/jack.cpp

#include <stdexcept>

#include <jack/jack.h>
#include <jack/midiport.h>

#include <Jack.h>

namespace Jack
{

class Client
{
	Client(const char* name)
	{
		client = jack_client_open(name, JackNullOption, NULL);
	}

	~Client()
	{
	}

public:
	jack_client_t* client;
};


MooerMidiControl::MooerMidiControl(std::string_view name, MIDI::Callback* callback)
	: m_callback(callback)
{
	m_client = jack_client_open(name.data(), JackNullOption, NULL);
	if(m_client == nullptr)
		throw std::runtime_error("Cannot open Jack server");

	m_port = jack_port_register(m_client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsOutput, 0);

	jack_set_process_callback(m_client, staticProcess, static_cast<void*>(this));

	if(jack_activate(m_client) != 0)
		throw std::runtime_error("Cannot activate Jack client");
}


MooerMidiControl::~MooerMidiControl()
{
	jack_deactivate(m_client);
	jack_port_unregister(m_client, m_port);
	jack_client_close(m_client);
}


void MooerMidiControl::ControlChange(std::uint8_t channel, MIDI::ControlChange controller, std::uint8_t value)
{
	jack_nframes_t time = 0;
	auto buf = MIDI::CreateControlChange(channel, controller, value);
	jack_midi_event_write(m_port, time, buf.data(), buf.size());
}


void MooerMidiControl::ProgramChange(std::uint8_t channel, std::uint8_t value)
{
	jack_nframes_t time = 0;
	auto buf = MIDI::CreateProgramChange(channel, value);
	jack_midi_event_write(m_port, time, buf.data(), buf.size());
}


void MooerMidiControl::Sysex(std::uint8_t channel, MIDI::Manufacturer manufacturer, std::span<std::uint8_t> values)
{
}


int MooerMidiControl::staticProcess(jack_nframes_t nframes, void* arg)
{
	static_cast<MooerMidiControl*>(arg)->process(nframes);
	return 0;
}


void MooerMidiControl::process(jack_nframes_t nframes)
{
	if(m_callback == nullptr)
		return;

	void* port_buf = jack_port_get_buffer(m_port, nframes);

	jack_position_t position;
	jack_transport_state_t transport = jack_transport_query(m_client, &position);

	const jack_nframes_t event_count = jack_midi_get_event_count(port_buf);
	for(int i = 0; i < event_count; i++)
	{
		jack_midi_event_t in_event;
		jack_midi_event_get(&in_event, port_buf, i);
		std::span<std::uint8_t> b(in_event.buffer, in_event.size);
		if(b.size() < 2)
			continue;

		auto id = static_cast<MIDI::Message>(b[0] & 0xF0);
		auto channel = b[0] & 0xF;
		switch(id)
		{
		case MIDI::Message::ControlChange:
			if(b.size() >= 3)
				m_callback->OnControlChange(channel, (MIDI::ControlChange)b[1], b[2]);
			break;
		case MIDI::Message::ProgramChange:
			if(b.size() >= 2)
				m_callback->OnProgramChange(channel, b[1]);
			break;
		case MIDI::Message::SysexStart:
			if(b.size() >= 2)
				m_callback->OnSysex(channel, (MIDI::Manufacturer)b[1], b.subspan(2));
			break;
		default:
			break;
		}
	}
}


} // namespace Jack
