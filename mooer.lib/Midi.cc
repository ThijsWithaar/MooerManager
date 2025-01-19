#include "Midi.h"

#if defined(__linux__)
#include <Jack.h>
#include <Pipewire.h>
#endif


namespace MIDI
{


Interface::Interface(Callback* callback)
	: m_callback(callback)
{
}


std::unique_ptr<Interface> Interface::Create(std::string_view name, MIDI::Callback* callback)
{
	std::unique_ptr<Interface> r;
#if defined(__linux__)
	auto jack_client = jack_client_open(name.data(), JackNullOption, NULL);
	if(jack_client != nullptr)
		r = std::make_unique<Jack::MooerMidiControl>(jack_client, callback);
	else
		r = std::make_unique<PipeWire::MooerMidiControl>(name, callback);
#endif
	return r;
}


} // namespace MIDI
