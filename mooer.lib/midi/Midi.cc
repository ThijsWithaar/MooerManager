#include "Midi.h"

#if defined(__linux__)
#include <midi/Alsa.h>
#include <midi/Jack.h>
#include <midi/Pipewire.h>
#elif defined(_WIN32)
#include "TeVirtualMidi.h"
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
	r = std::make_unique<Alsa::MidiControl>(name, callback);
	if(r != nullptr)
		return r;
	auto jack_client = jack_client_open(name.data(), JackNullOption, NULL);
	if(jack_client != nullptr)
		r = std::make_unique<Jack::MooerMidiControl>(jack_client, callback);
	else
		r = std::make_unique<PipeWire::MooerMidiControl>(name, callback);
#elif defined(_WIN32)
	std::wstring wname(begin(name), end(name));
	r = std::make_unique<TeVirtualMidi::MooerMidiControl>(wname);
#endif
	return r;
}


} // namespace MIDI
