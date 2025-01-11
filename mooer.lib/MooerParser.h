#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <optional>
#include <type_traits>

#include <UsbConnection.h>

// #define PARSER_DEBUG_LVL 3


namespace Mooer
{


template<typename T>
concept PODType = std::is_pod<T>::value;

template<typename T>
concept StandardLayoutType = std::is_standard_layout_v<T>;

template<StandardLayoutType Struct>
std::span<std::uint8_t, sizeof(Struct)> as_span(Struct& f)
{
	return std::span<std::uint8_t, sizeof(Struct)>{reinterpret_cast<std::uint8_t*>(&f), sizeof(Struct)};
}

template<StandardLayoutType Struct>
std::span<const std::uint8_t, sizeof(Struct)> as_const_span(const Struct* f)
{
	return std::span<const std::uint8_t, sizeof(Struct)>{reinterpret_cast<const std::uint8_t*>(f), sizeof(Struct)};
}

template<StandardLayoutType Struct>
void copy(std::span<std::uint8_t> dst, const Struct& src)
{
	auto* pSrc = &src;
	assert(pSrc != nullptr);
	auto ss = as_const_span(pSrc);
	assert(dst.size() >= ss.size());
	std::copy(begin(ss), end(ss), begin(dst));
}

template<StandardLayoutType Struct>
Struct& copy(Struct& dst, std::span<const std::uint8_t> src)
{
	std::span<std::uint8_t> ds = as_span(dst);
	assert(ds.size() == src.size());
	std::copy(begin(src), end(src), begin(ds));
	return dst;
}


using u8 = std::uint8_t;

constexpr int vendor_id = 0x0483;
constexpr int product_id = 0x5703;

// clang-format off
constexpr std::array<std::string_view, 65> amp_model_names = {
	"65 US DS", "65 US TW", "59 US BASS", "US SONIC", "US BLUES CL",
	"US BLUES OD", "J800", "J900", "PLX100", "E650 CL", "E650 DS", "POWERBELL CL",
	"POWERBELL DS", "BLACKNIGHT CL", "BLACKNIGHT DS", "MARK III CL", "MARK III DS",
	"MARK V CL", "MARK V DS", "TRI REC CL", "TRI REC DS", "ROCK VRB CL",
	"ROCK VRB DS", "CITRUS 30", "CITRUS 50", "SLOW 100 CR",
	"SLOW 100 DS", "DR. ZEE 18 JR", "DR. ZEE RECK", 
	// 30
	"JET 100H CL", "JET 100H OD", "JAZZ 120", "UK30 CL", "UK 30 OD", "HWT 103",
	"PV 5050 CL", "PV 5050 DS", "REGAL TONE CL", "REGAL TONE OD1",
	"REGAL TONE OD2", "CAROL CL", "CAROL OD", "CARDEFF",
	"EV 5050 CL", "EV 5050 DS", "HT CLUB CL", "HT CLUB DS", "HUGEN CL", "HUGEN OD",
	// 50
	"HUGEN DS", "KOCHE OD", "KOCHE DS", "ACOUSTIC 1", "ACOUSTIC 2", "ACOUSTIC 3",
	"USER 1", "USER 2", "USER 3", "USER 4", "USER 5", "USER 6", "USER 7", "USER 8",
	"USER 9", "USER 10"
};

constexpr std::array<std::string_view, 36> cab_model_names = {
	"US DLX 112", "US TWN 212", "US BASS 410", "Sonic 112", "Blues 112",
	"1960 412", "Eagle P412", "Eagle S412", "Mark 112", "Rec 412", "Citrus 412",
	"Citrus 212", "Slow 412", "Dr. Zee 112", "Dr. Zee 212", "Jazz 212", "UK 212",
	"HWT 412", "PV 5050 412", "Regal Tone 110", "Two Stones 212", "Cardeff 112",
	"EV 5050 412", "HT 412", "Gas Station 412", "Acoustic 112",
	"User 1", "User 2", "User 3", "User 4", "User 5", "User 6", "User 7", "User 8",
	"User 9", "User 10"
};

// clang-format on

/// CRC of the message, excluding the package length and the AA55 pre-amble.
std::uint16_t calculateChecksum(std::span<std::uint8_t> m);

/// Big-endian 16-bit integer
struct u16be
{
	u16be() = default;

	u16be(std::uint16_t v)
	{
		s.hi = v >> 8;
		s.lo = v & 0xFF;
	}

	operator std::uint16_t() const
	{
		return (((std::uint16_t)s.hi) << 8) | s.lo;
	}

	struct split
	{
		std::uint8_t hi, lo;
	};

	union
	{
		std::uint16_t native;
		split s;
	};
};
static_assert(sizeof(u16be) == 2);


/// Structures to define the file-formats
namespace File
{

struct FX
{
	u8 type;
	u8 enabled;
	u8 attack, thresh, ratio, level;
	u8 unused[2];
};
static_assert(sizeof(FX) == 8);

struct DS
{
	u8 type;
	u8 enabled;
	u8 volume, tone, gain;
	u8 unused[3];
};
static_assert(sizeof(DS) == 8);

struct AMP
{
	u8 type, enabled;
	u8 gain, bass, mid, treble, pres, mst;
};
static_assert(sizeof(AMP) == 8);

struct Cab
{
	u8 type, enabled;
	u8 p[6];
};
static_assert(sizeof(Cab) == 8);

struct Preset
{
	std::string_view getName() const
	{
		return {&name[0], strnlen(name, sizeof(name))};
	}

	std::array<u8, 10> fxOrder;
	u16be size;
	char name[16];
	FX fx;					  // 541 - 548
	DS ds;					  // 549 - 556
	AMP amp;				  // 557 - 564
	Cab cab;				  // 565 - 572
	std::array<u8, 8> ns;	  // 573 - 580
	std::array<u8, 8> eq;	  // 581
	std::array<u8, 8> mod;	  // 589 - 596
	std::array<u8, 8> delay;  // 597 - 604
	std::array<u8, 8> reverb; // 605
};

struct PresetPadded : Preset
{
	PresetPadded() = default;

	PresetPadded(std::span<const std::uint8_t> src)
	{
		std::span<std::uint8_t> ds{reinterpret_cast<std::uint8_t*>(this), 0x200};
		assert(ds.size() == src.size());
		std::copy(begin(src), end(src), begin(ds));
	}

	std::array<u8, 0x200 - sizeof(Preset)> padding;
};
static_assert(sizeof(PresetPadded) == 0x200);


/// Format of the .MO file
/// https://github.com/sidekickDan/mooerMoConvert/blob/main/mooer.php
struct MO
{
	u8 junk[0x200]; ///< Data seems unused
	PresetPadded preset;
};
static_assert(offsetof(Preset, fx) == 540 - 0x200, "FX offset incorrect");
static_assert(offsetof(Preset, ds) == 548 - 0x200, "DS offset incorrect");
static_assert(offsetof(Preset, reverb) == 604 - 0x200, "DS reverb incorrect");
static_assert(sizeof(MO) == 0x400);


struct MbfPreset
{
	u16be index;
	PresetPadded preset;
	std::uint8_t pad[0x20];
};
static_assert(sizeof(MbfPreset) == 0x222);

struct Mbf
{
	char manufacturer[8]; ///< "MOOER "
	char model[32];
	char version[7];  ///< "V1.0.0"
	char version2[7]; ///< "V1.1.0"
	char version3[7]; ///< "V1.0.0"
	char zero[24];
	char buff[4]; ///< "BUFF"
	// Followed by 2 absolute file-offsets, system header at 0x400 and presets at 0x531
	char unknown[0x3a7];
	char system[0x131];
	char presetHeader[0x11e];
	std::array<MbfPreset, 199> presets;
};
static_assert(offsetof(Mbf, system) == 0x400);
static_assert(offsetof(Mbf, presetHeader) == 0x531);
static_assert(offsetof(Mbf, presets) == 0x650);

/// Load a backup from an .mbf file
Mbf LoadBackup(std::span<const std::uint8_t> mbf);


} // namespace File


namespace DeviceFormat
{
constexpr int sh = 1; // Size of the packet header

struct FX
{
	FX() = default;

	FX(const File::FX& fx)
	{
		enabled = fx.enabled;
		type = fx.type;
		q = fx.attack;
		position = fx.thresh;
		peak = fx.ratio;
		level = fx.level;
	}

	auto& operator=(std::span<const std::uint8_t> s)
	{
		return copy(*this, s);
	}

	u16be enabled, type;
	u16be q, position, peak, level;
};
static_assert(sizeof(FX) == 0x0d - sh);

struct OD
{
	OD() = default;

	OD(const File::DS& ds)
	{
		enabled = ds.enabled;
		type = ds.type;
		volume = ds.volume;
		tone = ds.tone;
		gain = ds.gain;
	}

	auto& operator=(std::span<const std::uint8_t> s)
	{
		return copy(*this, s);
	}

	u16be enabled, type;
	u16be volume, tone, gain;
};
static_assert(sizeof(OD) == 0x0b - sh);

struct Amp
{
	auto& operator=(std::span<const std::uint8_t> s)
	{
		return copy(*this, s);
	}

	u16be enabled, type;
	u16be gain, bass, mid, treble, pres, mst;
};
static_assert(sizeof(Amp) == 0x11 - sh);

struct Cab
{
	auto& operator=(std::span<const std::uint8_t> s)
	{
		return copy(*this, s);
	}

	u16be enabled, type;
	u16be mic, center, distance;
	u16be tube;
};
static_assert(sizeof(Cab) == 0x0d - sh);

struct NS
{
	auto& operator=(std::span<const std::uint8_t> s)
	{
		return copy(*this, s);
	}

	u16be enabled, type;
	u16be attack, release, thresh;
};
static_assert(sizeof(NS) == 0x0b - sh);

struct Equalizer
{
	auto& operator=(std::span<const std::uint8_t> s)
	{
		return copy(*this, s);
	}

	u16be enabled, type;
	std::array<u16be, 6> band;
	std::array<u8, 6> unknown;
};
static_assert(sizeof(Equalizer) == 0x17 - sh);

struct Mod
{
	auto& operator=(std::span<const std::uint8_t> s)
	{
		return copy(*this, s);
	}

	u16be enabled, type;
	u16be rate, level, depth;
	u16be p4, p5;
};
static_assert(sizeof(Mod) == 0x0f - sh);

struct Delay
{
	u16be enabled, type;
	u16be level, fback, time, subd;
	u16be p5, p6;
};
static_assert(sizeof(Delay) == 0x11 - sh);

struct Reverb
{
	u16be enabled, type;
	std::array<u16be, 4> p;
};
static_assert(sizeof(Reverb) == 0x0d - sh);

struct Rhythm
{
	auto& operator=(std::span<const std::uint8_t> s)
	{
		copy(*this, s);
		return *this;
	}

	u16be bpm;
	// All these are not sent over USB:
	u16be enabled;
	u16be type;
	u16be kind;
	u16be volume;
	u16be unknown;
};

struct Pedal
{
	u8 module1, param1, module2, param2;
	u8 unk = 5;
	u8 vol_min, vol_max;
};
static_assert(sizeof(Pedal) == 7);

/// Message AmpModels(E3) from the device
struct AmpModelNames
{
	AmpModelNames()
	{
		std::fill(std::begin(m_data), std::end(m_data), 0);
	}

	AmpModelNames(std::span<const std::uint8_t> d)
	{
		assert(d.size() >= m_data.size());
		std::copy(std::begin(d), std::begin(d) + m_data.size(), std::begin(m_data));
	}

	constexpr static int size()
	{
		return 10;
	}

	auto operator[](int i) const
	{
		return std::string_view(m_data.data() + 15 * i, 15);
	}

	void set(int i, std::string_view sv)
	{
		auto d = this->span(i);
		std::fill(begin(d), end(d), ' ');
		std::copy(begin(sv), begin(sv) + std::min(d.size(), sv.size()), begin(d));
	}

	std::span<char> span(int i)
	{
		return std::span(m_data).subspan(15 * i, 15);
	}

private:
	std::array<char, 15 * 10> m_data;
};

struct Preset
{
	Preset() = default;

	Preset(std::span<const std::uint8_t> s)
	{
		copy(*this, s);
	}

	auto& operator=(std::span<const std::uint8_t> s)
	{
		copy(*this, s);
		return *this;
	}

	std::array<std::uint8_t, 10> fxOrder; ///< Ordering of the effects, excluding rhythm
	u16be size;
	char name[14];
	FX fx;
	OD distortion;
	Amp amp;
	Cab cab;
	NS noiseGate;
	Equalizer equalizer;
	Mod modulation;
	Delay delay;
	Reverb reverb;
	Rhythm rhythm;
	u8 unknown[0x15e];
};
static_assert(sizeof(Preset) == 0x200);

/// All state that is sent via USB
struct State
{
	int activePresetIndex;
	int activeMenu;
	AmpModelNames ampModelNames;
	Preset activePreset;
	std::array<Mooer::File::PresetPadded, 200> savedPresets;
};

} // namespace DeviceFormat



class RxFrame
{
public:
	RxFrame()
		: m_len(EMPTY_BUFFER)
	{
	}

	enum Group
	{
		Identify = 0x10,
		PedalAssignment = 0xA3,
		PatchAlternate = 0xA4, ///< Send when pressing CTRL TAP
		PatchSetting = 0xA5,
		ActivePatch = 0xA6,
		CabinetUpload = 0xE1,
		AmpUpload = 0xE2, ///< Upload index received/request?
		AmpModels = 0xE3, ///< Names of the custom amp models (bank 56 and up)
		Menu = 0x82,	  ///< The currently active menu on the display
		Preset = 0x83,
		PedalAssignment_Maybe = 0x84,
		FX = 0x90,
		DS_OD = 0x91,
		AMP = 0x93,
		CAB = 0x94,
		NS_GATE = 0x95,
		EQ = 0x96,
		MOD = 0x97,
		DELAY = 0x98,
		REVERB = 0x99,
		RHYTHM = 0xA4,
	};

	struct Frame
	{
		Group group() const
		{
			return static_cast<Group>(key);
			// return static_cast<Group>(key & 0xFF);
		}

		int index() const
		{
			return data.empty() ? -1 : data[0]; // key >> 8;
		}

		auto noidx_data() const
		{
			return std::span(data).subspan(1);
		}

		auto nochecksum_data() const
		{
			return std::span(data).subspan(0, data.size() - 2);
		}

		std::uint16_t key;
		std::vector<std::uint8_t> data;
	};

	std::optional<Frame> process(std::span<std::uint8_t> chunk)
	{
		if(chunk.size() < 2)
			return std::nullopt;
		auto packet_size = read<std::uint8_t>(chunk);
		chunk = chunk.subspan(0, packet_size);
		if(m_len == EMPTY_BUFFER)
		{
			auto hdr = read<std::uint16_t>(chunk);
			// std::cout << std::format("MooerRxFrame: New frame {:04x} packet, hdr {:04x}\n", packet_size, hdr);
			if(hdr != 0x55AA)
				return std::nullopt; // Not the start of a buffer, drop it
			m_frame.data.clear();
			m_len = read<std::uint16_t>(chunk);
			m_frame.key = read<std::uint8_t>(chunk); // The length seems to exclude this
		}
		std::copy(begin(chunk), end(chunk), std::back_inserter(m_frame.data));
#if PARSER_DEBUG_LVL > 4
		std::cout << std::format("RxFrame: buffer {:04x}/{:04x}, psize {}\n", m_frame.data.size(), m_len, packet_size);
#endif
		if(m_frame.data.size() >= m_len)
		{
			m_len = EMPTY_BUFFER;
			return m_frame;
		}
		return {};
	}

private:
	template<StandardLayoutType T>
	T read(std::span<std::uint8_t>& chunk)
	{
		assert(chunk.size() >= sizeof(T));
		T r;
		std::memcpy(&r, chunk.data(), sizeof(T));
		chunk = chunk.subspan(sizeof(T));
		return r;
	}

	constexpr static int EMPTY_BUFFER = -1;

	int m_len;
	Frame m_frame;
};

class Listener
{
public:
	struct Identity
	{
		std::string version;
		std::string name;
	};

	virtual ~Listener() = default;

	virtual void OnMooerFrame(const RxFrame::Frame& frame) {};
	virtual void OnMooerIdentify(const Identity&) {};
};

/**
Commands for the request, send on host->1.5.1, received on 1.5.1->host.
1.5.2 has ack traffic
*/
class Parser : public USB::TransferListener
{
public:
	Parser(USB::Connection* connection = nullptr, Listener* listener = nullptr)
		: m_connection(connection), m_listener(listener)
	{
		m_connection->Connect(this, m_rx_endpoint);
	}

	~Parser()
	{
		std::cout << "Mooer::Parser::~Parser" << std::endl;
	}

	/// Send an identification request. Should respond with "MOOER_GE200"
	void SendIdentifyRequest()
	{
		const std::array<std::uint8_t, 3> msg{0x84, 0x00, 0x00};
		SendWithHeaderAndChecksum(msg);
	}

	/// A flush, send after the ID-response
	void SendFlush()
	{
		std::array<std::uint8_t, 64> msg = {0, 1, 2, 3, 4, 0, 0, 0, 0};
		SendSplitPacket(msg);
	}

	/// Request names of all patches
	void SendPatchListRequest()
	{
		std::array<std::uint8_t, 8> msg({0xe0});
		SendWithHeaderAndChecksum(msg);
	}

	/** Change the currently active preset
	 */
	void SendPresetChange(std::uint8_t idx)
	{
		const std::array<std::uint8_t, 2> msg{RxFrame::Group::ActivePatch, idx};
		SendWithHeaderAndChecksum(msg);
		// std::array<std::uint8_t, 12> msg2{11, 0xAA, 0x55, 0x05, 0x00, 0x82, 0x00, 0x00, 0x00, 0x00, 0xE0, 0x0B};
	}

	/** Switch the menu that's shown on the display
	FX=1, OD=2, Amp=3, Cab=4
	*/
	void SwitchMenu(std::uint8_t menu)
	{
		const std::array<std::uint8_t, 2> msg{RxFrame::Group::Menu, menu};
		SendWithHeaderAndChecksum(msg);
	}

	/**
	Set the expression pedal assignment.
	/p module1, /p param1: Expression-Pedal 1 is assigned to this parameter
	/p module2, /p param2: Expression-Pedal 2 (External) is assigned to this parameter
	 */
	void SetExpressionPedal(DeviceFormat::Pedal pedal)
	{
		std::array<std::uint8_t, 8> msg{RxFrame::Group::PedalAssignment};
		copy(std::span(msg).subspan(1), pedal);
		SendWithHeaderAndChecksum(msg);
	}

	/**
	/p amp: The contents of an .amp file
	/p name: Name of the amplifier setting
	/p slot: The slot index into which to load
	 */
	void LoadAmplifier(std::span<std::uint8_t> amp, std::string_view name, int slot);

	/**
	Load a .wav file into a Cabinet slot
	*/
	void LoadWav(std::span<std::uint8_t> wav, std::string_view name, int slot);

	/**
	Load an .mo file into the active preset
	*/
	void LoadMoPreset(std::span<const std::uint8_t> mo);

	void SetFX(const DeviceFormat::FX& fx)
	{
		std::array<std::uint8_t, 0x0d> msg{RxFrame::Group::FX, 0};
		copy(std::span(msg).subspan(1), fx);
		SendWithHeaderAndChecksum(msg);
	}

	void SetDS(const DeviceFormat::OD& ds)
	{
		std::array<std::uint8_t, sizeof(DeviceFormat::OD) + 1> msg{RxFrame::Group::DS_OD, 0};
		copy(std::span(msg).subspan(1), ds);
		SendWithHeaderAndChecksum(msg);
	}

	void SetAmplifier(const DeviceFormat::Amp& s)
	{
		std::array<std::uint8_t, sizeof(DeviceFormat::Amp) + 1> msg{RxFrame::Group::AMP, 0};
		copy(std::span(msg).subspan(1), s);
		SendWithHeaderAndChecksum(msg);
	}

	void SetCabinet(const DeviceFormat::Cab& s)
	{
		std::array<std::uint8_t, sizeof(DeviceFormat::Cab) + 1> msg{RxFrame::Group::CAB, 0};
		copy(std::span(msg).subspan(1), s);
		SendWithHeaderAndChecksum(msg);
	}

	void SetNoiseGate(const DeviceFormat::NS& s)
	{
		std::array<std::uint8_t, sizeof(DeviceFormat::NS) + 1> msg{RxFrame::Group::NS_GATE, 0};
		copy(std::span(msg).subspan(1), s);
		SendWithHeaderAndChecksum(msg);
	}

	void SetEQ(const DeviceFormat::Equalizer& s)
	{
		std::array<std::uint8_t, sizeof(DeviceFormat::Equalizer) + 1> msg{RxFrame::Group::EQ, 0};
		copy(std::span(msg).subspan(1), s);
		SendWithHeaderAndChecksum(msg);
	}

	void SetModulator(const DeviceFormat::Mod& s)
	{
		std::array<std::uint8_t, sizeof(DeviceFormat::Mod) + 1> msg{RxFrame::Group::MOD, 0};
		std::uint16_t type = s.type;
		if(type >= 22)
			throw std::runtime_error(std::format("Modulator type {} must be < 21", type));
		copy(std::span(msg).subspan(1), s);
		// This makes the GE-200 crash
		SendWithHeaderAndChecksum(msg);
	}

private:
	void SendWithHeaderAndChecksum(std::span<const std::uint8_t> m);

	/// Split a packet into max 63-bytes chunks and send it
	void SendSplitPacket(std::span<const std::uint8_t> m);

	bool OnUsbInterruptData(std::span<std::uint8_t> data) override;

	std::string as_string(std::span<std::uint8_t> buf)
	{
		return {reinterpret_cast<const char*>(buf.data()), buf.size()};
	}

	constexpr static int m_tx_endpoint = 0x02;
	constexpr static int m_rx_endpoint = 0x81;

	USB::Connection* m_connection;
	std::array<std::uint8_t, 64> m_usb_tx;
	RxFrame m_frame_rx;
	Listener* m_listener;
};


} // namespace Mooer
