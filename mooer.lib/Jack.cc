// https://github.com/harryhaaren/JACK-MIDI-Examples/blob/master/printData/jack.cpp

#include <stdexcept>

#include <jack/jack.h>
#include <jack/midiport.h>

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


class MooerMidiControl
{
public:
	MooerMidiControl();

	~MooerMidiControl();

private:
	/// Helper function to convert jack-callback to member-function-call
	static int staticProcess(jack_nframes_t nframes, void* arg);

	void process(jack_nframes_t nframes);

	jack_client_t* m_client;
	jack_port_t *m_InputPort, *port_out;
};


MooerMidiControl::MooerMidiControl()
{
	m_client = jack_client_open("PrintMidi", JackNullOption, NULL);
	if(m_client == nullptr)
		throw std::runtime_error("Cannot open Jack server");

	m_InputPort = jack_port_register(m_client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

	jack_set_process_callback(m_client, staticProcess, static_cast<void*>(this));

	if(jack_activate(m_client) != 0)
		throw std::runtime_error("Cannot activate Jack client");
}


MooerMidiControl::~MooerMidiControl()
{
	jack_deactivate(m_client);
	jack_port_unregister(m_client, m_InputPort);
	jack_client_close(m_client);
}


int MooerMidiControl::staticProcess(jack_nframes_t nframes, void* arg)
{
	static_cast<MooerMidiControl*>(arg)->process(nframes);
	return 0;
}


void MooerMidiControl::process(jack_nframes_t nframes)
{
	void* port_buf = jack_port_get_buffer(m_InputPort, nframes);

	jack_position_t position;
	jack_transport_state_t transport = jack_transport_query(m_client, &position);

	jack_midi_event_t in_event;
	const jack_nframes_t event_count = jack_midi_get_event_count(port_buf);
	for(int i = 0; i < event_count; i++)
	{
		jack_midi_event_get(&in_event, port_buf, i);
		// ToDo: Parse in_event.buffer
		// Convert to patch changes
	}
}


} // namespace Jack
