#include <midi/Alsa.h>

#include <alsa/asoundlib.h>
#include <fmt/format.h>

namespace Alsa
{

struct ReturnCode
{
	ReturnCode(int rc)
	{
		validate(rc);
	}

	ReturnCode& operator=(int rc)
	{
		validate(rc);
		return *this;
	}

	static void validate(int rc)
	{
		if(rc < 0)
			throw std::runtime_error(fmt::format("ALSA error '{}'", snd_strerror(rc)));
	}
};


MidiControl::VirtualMidi::VirtualMidi()
{
	ReturnCode rc = snd_rawmidi_open(&read, &write, "virtual", SND_RAWMIDI_READ_STANDARD);
}


MidiControl::VirtualMidi::~VirtualMidi()
{
	snd_rawmidi_close(read);
	snd_rawmidi_close(write);
}


MidiControl::MidiControl(std::string_view /*name*/, MIDI::Callback* callback)
	: MIDI::Interface(callback), m_worker(&MidiControl::BackgroundWork, this)
{
}


MidiControl::~MidiControl()
{
	m_worker.request_stop();
}


void MidiControl::BackgroundWork()
{
	auto stopToken = m_worker.get_stop_token();

	std::vector<struct pollfd> fds(snd_rawmidi_poll_descriptors_count(m_port.read));
	snd_rawmidi_poll_descriptors(m_port.read, fds.data(), fds.size());

	std::array<std::uint8_t, 256> buf;

	const int timeout_ms = 1000;
	while(!stopToken.stop_requested())
	{
		// Wait for input
		int err = poll(fds.data(), fds.size(), timeout_ms);
		if(err < 0 && errno == EINTR)
			break;

		// Check if the event is for reading data and re-initialize fds
		unsigned short revents = 0;
		if((err = snd_rawmidi_poll_descriptors_revents(m_port.read, fds.data(), fds.size(), &revents)) < 0)
			throw std::runtime_error("ALSA: Can't get descriptor events");
		if(revents & (POLLERR | POLLHUP))
			break;
		if(!(revents & POLLIN))
			continue;

		// Read the data
		err = snd_rawmidi_read(m_port.read, buf.data(), buf.size());
		if(err == -EAGAIN || err <= 0)
			continue;
		std::span<std::uint8_t> data(buf.data(), err);

		// Handle the data
		auto id = static_cast<MIDI::Message>(data[0] & 0xF0);
		auto channel = data[0] & 0xF;
		switch(id)
		{
		case MIDI::Message::ControlChange:
			if(data.size() >= 3)
				m_callback->OnControlChange(channel, (MIDI::ControlChange)data[1], data[2]);
			break;
		case MIDI::Message::ProgramChange:
			if(data.size() >= 2)
				m_callback->OnProgramChange(channel, data[1]);
			break;
		case MIDI::Message::SysexStart:
			if(data.size() >= 2)
				m_callback->OnSysex(channel, (MIDI::Manufacturer)data[1], data.subspan(2));
			break;
		default:
			break;
		}
	}
}


void MidiControl::ControlChange(std::uint8_t channel, MIDI::ControlChange controller, std::uint8_t value)
{
	auto buf = MIDI::CreateControlChange(channel, controller, value);
	snd_rawmidi_write(m_port.write, buf.data(), buf.size());
}


void MidiControl::ProgramChange(std::uint8_t channel, std::uint8_t value)
{
	auto buf = MIDI::CreateProgramChange(channel, value);
	snd_rawmidi_write(m_port.write, buf.data(), buf.size());
}


void MidiControl::Sysex(std::uint8_t channel, MIDI::Manufacturer manufacturer, std::span<std::uint8_t> values)
{
	MIDI::CreateSysex(m_sysexBuffer, channel, manufacturer, values);
	snd_rawmidi_write(m_port.write, m_sysexBuffer.data(), m_sysexBuffer.size());
}


} // namespace Alsa
