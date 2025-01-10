#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <vector>

/**
Definition of the MIDI protocol

https://www.songstuff.com/recording/article/midi-message-format/
*/
namespace MIDI
{

enum class ControlChange : std::uint8_t
{
	BackSelect = 0,
	ModulationWheel = 1,
	BreathController = 2,
	// Undef1 = 3,
	FootPedal = 4,
	PortamentoTime = 5,
	DataEntry = 6,
	Volume = 7,
	Balance = 8,
	// Undef2 = 9,
	Pan = 10,
	Expression = 11,
	FxController1MSB = 12,
	FxController2MSB = 13,
	DamperPedalOnOff = 64,
	PortamentoOnOff = 65,
	SostenutoPedalOnOff = 66,
	SoundController1 = 70,
	Resonance = 71,
	ReleaseTime = 72,
	PortamentoAmount = 84,
	Reverb = 91,
	Tremolo = 92,
	Chorus = 93,
	Detune = 04,
	Phaser = 95,
	// ChannelMode messages:
	Mute = 120,
	Reset = 121,
};

enum class Message : std::uint8_t
{
	NoteOff = 0x80,
	NoteOn = 0x90,
	AfterTouch = 0xA0,
	ControlChange = 0xB0,
	ProgramChange = 0xC0,
	ChannelAfterTouch = 0xD0,
	Pitch = 0xE0,

	SysexStart = 0xF0,
	TimeCode = 0xF1,
	SongPosition = 0xF2,
	SongSelect = 0xF3,
	TuneRequest = 0xF6,
	SysexEnd = 0xF7,
	TimingClock = 0xF8,
	Start = 0xFA,
	Continue = 0xFB,
	Stop = 0xFC,
	ActiveSensing = 0xFE,
	SystemReset = 0xFF
};

enum class Manufacturer : std::uint8_t
{
	Moog = 0x04,
	Fender = 0x08,
	NonCommercial = 0x7D,
	NonRealtime = 0x7E,
	Realtime = 0x7F,
};

auto operator|(Message m, std::uint8_t n)
{
	return std::bit_cast<std::uint8_t>(m) | n;
}

auto CreateNoteOn(std::uint8_t channel, std::uint8_t note, std::uint8_t velocity)
{
	return std::array<std::uint8_t, 3>{Message::NoteOn | channel, note, velocity};
}

auto CreateControlChange(std::uint8_t channel, ControlChange controller, std::uint8_t value)
{
	return std::array<std::uint8_t, 3>{
		Message::ControlChange | channel, std::bit_cast<std::uint8_t>(controller), value};
}

auto CreateProgramChange(std::uint8_t channel, std::uint8_t value)
{
	return std::array<std::uint8_t, 3>{Message::ProgramChange | channel, value, 0};
}

auto CreateSysex(std::uint8_t channel, Manufacturer manufacturer, std::span<std::uint8_t> data)
{
	std::vector<std::uint8_t> r(data.size() + 3);
	r[0] = Message::SysexStart | channel;
	r[1] = std::bit_cast<std::uint8_t>(manufacturer);
	std::copy(std::begin(data), std::end(data), std::begin(r) + 2);
	r.back() = Message::SysexEnd | 0;
	return r;
}

/// The default implementation for each message is to do nothing
class Callback
{
public:
	virtual void OnControlChange(std::uint8_t channel, ControlChange controller, std::uint8_t value) {};
	virtual void OnProgramChange(std::uint8_t channel, std::uint8_t value) {};
	virtual void OnSysex(std::uint8_t channel, Manufacturer manufacturer, std::span<std::uint8_t> data) {};
};

class Parser
{
public:
	void Parse(std::span<std::uint8_t> data, Callback* cb);

private:
	std::vector<std::uint8_t> m_buffer;
};

} // namespace MIDI