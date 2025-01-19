#include <cstdint>

#include <bit>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

#include <MooerParser.h>
#include <WaveFile.h>


// #define PARSER_DEBUG_LVL 3
#ifdef PARSER_DEBUG_LVL
#include <format>
#endif


namespace Mooer
{


const std::uint16_t g_hash_lut[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad,
	0xe1ce, 0xf1ef, 0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a,
	0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
	0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861,
	0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
	0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 0x6ca6, 0x7c87,
	0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
	0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3,
	0x5004, 0x4025, 0x7046, 0x6067, 0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290,
	0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e,
	0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f,
	0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
	0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83,
	0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
	0x2e93, 0x3eb2, 0x0ed1, 0x1ef0};


std::uint16_t calculateChecksum(std::span<std::uint8_t> m)
{
	std::uint16_t chk = 0;
	for(std::uint8_t v : m)
		chk = g_hash_lut[(chk >> 8) ^ v] ^ (chk << 8);
	return ~chk;
}


File::Mbf LoadBackup(std::span<const std::uint8_t> mbfData)
{
	using namespace std::literals;
	if(mbfData.size() <= sizeof(File::Mbf))
	{
		std::stringstream ss;
		ss << "MBF must be at least " << sizeof(File::Mbf) << " bytes\n";
		throw std::runtime_error(ss.str());
	}

	auto mbf = reinterpret_cast<const File::Mbf*>(mbfData.data());
	if(std::string_view(mbf->manufacturer) != "MOOER"sv)
		throw std::runtime_error("MBF has an invalid heade");

	return *mbf;
}


//-- Parser --

enum class AmpKind : std::uint8_t
{
	amp = 0x05,
	gnr = 0x15
};

void Parser::LoadAmplifier(std::span<std::uint8_t> amp, std::string_view name, int slot)
{
	const int szData = 0x204;
	std::vector<std::uint8_t> packetData(szData + 6);

	packetData[0] = 0xAA;
	packetData[1] = 0x55;
	packetData[2] = szData & 0xFF;
	packetData[3] = szData >> 8;

	packetData[4] = RxFrame::Group::AmpUpload; // 0xE2;
	packetData[5] = slot;
	// packetData[6] = index;
	packetData[7] = std::bit_cast<std::uint8_t>(AmpKind::amp);

	const int iData = 8;
	const int iChecksum = iData + 512;
	constexpr int nBytesPerWord = 4;
	for(int i = 0; i < nBytesPerWord; i++)
	{
		packetData[6] = i + 1;
		for(int n = 0; n < 512; n++)
			packetData.at(iData + n) = amp[nBytesPerWord * n + (nBytesPerWord - 1 - i)];
		int cc = calculateChecksum(std::span(packetData).subspan(2, iChecksum - 2));
#if PARSER_DEBUG_LVL > 2
		std::cout << std::format("LoadAmplifier: checksum 0x{:04x}\n", cc);
#endif
		packetData[iChecksum + 0] = cc >> 8;
		packetData[iChecksum + 1] = cc & 0xFF;
		SendSplitPacket(packetData);
	}

	packetData[6] = nBytesPerWord + 1;
	std::fill(begin(packetData) + iData, end(packetData), 0);
	std::copy(begin(name), begin(name) + std::min<int>(name.size(), 15), begin(packetData) + iData);
	SendWithHeaderAndChecksum(std::span(packetData).subspan(4, 0x13));
}


void Parser::LoadGNR(std::span<std::uint8_t> gnrData, std::string_view name, int slot)
{
	if(gnrData.size() < sizeof(File::GNR))
		throw std::runtime_error("GNR too small in size");
	auto gnr = reinterpret_cast<const File::GNR*>(gnrData.data());

	assert(std::string_view(&gnr->manufacturer[0]) == "mooerge");
	assert(std::string_view(&gnr->infoId[0], 4) == "info");
	assert(std::string_view(&gnr->dataId[0], 4) == "data");

	auto ampData = gnr->dataSpan();
	/* LoadAmplifier(), with nBytesPerWord = 20, AmpKind::gnr
	ControlOut bmRequestType 0x21, data fragment 00
	ControlOut bmRequestType 0x21, data fragment 000c
	ControlOut bmRequestType 0x21, data fragment 000c
	ControlOut bmRequestType 0x21, data fragment 00
	ControlOut bmRequestType 0x21, data fragment 00ec
	ControlOut bmRequestType 0x21, data fragment 00ec
	bmRequestType:00, SetInterface(0x0B), alternate 1: itf 2, len 0
	ISO Out: data 0xa56/2646, 10 packets, all zeros
	bmRequestType:00, SetInterface(0x0B), alternate 0, itf 2, len 0
	*/
	const std::array<std::uint8_t, 1> m0{0x00};
	const std::array<std::uint8_t, 2> mc{0x00, 0x0C};
	const std::array<std::uint8_t, 2> mec{0x00, 0xEC};
	m_connection->control_transfer(0x21, 1, 1, 2, m0);
	m_connection->control_transfer(0x21, 1, 0x102, 2, mc);
	m_connection->control_transfer(0x21, 1, 0x202, 2, mc);

	m_connection->control_transfer(0x21, 1, 1, 5, m0);
	m_connection->control_transfer(0x21, 1, 0x102, 5, mec);
	m_connection->control_transfer(0x21, 1, 0x202, 5, mec);

	m_connection->set_interface(2, 1);
	std::vector<std::uint8_t> bulk(2646, 0);
	m_connection->bulk_transfer(m_tx_bulk_endpoint, bulk);
	m_connection->set_interface(2, 0);
}


void Parser::LoadWav(std::span<std::uint8_t> wav, std::string_view name, int slot)
{
	auto optionalWavData = RIFF::FindChunk(wav, RIFF::dataId);
	if(!optionalWavData.has_value())
		throw std::runtime_error("Input is not a RIFF wave file");
	auto wavData = optionalWavData.value();
	// auto wavData = wav.subspan(0x3E0);

	const int szData = 0x203;
	std::vector<std::uint8_t> packetData(szData + 6);

	packetData[0] = 0xAA;
	packetData[1] = 0x55;
	packetData[2] = szData & 0xFF;
	packetData[3] = szData >> 8;

	packetData[4] = RxFrame::Group::CabinetUpload; // 0xE1;
	packetData[5] = slot;

	const int iData = 8;
	const int iChecksum = iData + 512;
	constexpr int nBytesPerWord = 3;
	for(int i = 0; i < nBytesPerWord; i++)
	{
		packetData[6] = i + 1;
		for(int n = 0; n < 512; n++)
			packetData.at(iData + n) = wavData[nBytesPerWord * n + (nBytesPerWord - 1 - i)];
		int cc = calculateChecksum(std::span(packetData).subspan(2, iChecksum - 2));
#if PARSER_DEBUG_LVL > 2
		std::cout << std::format("LoadWav: checksum 0x{:04x}\n", cc);
#endif
		packetData[iChecksum + 0] = cc >> 8;
		packetData[iChecksum + 1] = cc & 0xFF;
		SendSplitPacket(packetData);
	}

	packetData[6] = nBytesPerWord + 1;
	std::fill(begin(packetData) + iData, end(packetData), 0);
	std::copy(begin(name), begin(name) + std::min<int>(name.size(), 15), begin(packetData) + iData);
	SendWithHeaderAndChecksum(std::span(packetData).subspan(4, 0x13));
}


void Parser::LoadMoPreset(std::span<const std::uint8_t> mo)
{
	const int szData = 0x200;

	assert(mo.size() == 2048);
	auto moData = mo.subspan(0x200, szData);

	const int iData = 5;
	std::vector<std::uint8_t> packetData(iData + szData + 2);

	packetData[0] = 0xAA;
	packetData[1] = 0x55;
	// Size includes 'RxFrame::Group'
	packetData[2] = (szData + 1) & 0xFF;
	packetData[3] = (szData + 1) >> 8;
	packetData[4] = RxFrame::Group::Preset; // 0x83;

	std::copy(begin(moData), end(moData), begin(packetData) + iData);

	const int iChecksum = iData + moData.size();
	int cc = calculateChecksum(std::span(packetData).subspan(2, iChecksum - 2));
	packetData[iChecksum + 0] = cc >> 8;
	packetData[iChecksum + 1] = cc & 0xFF;

	SendSplitPacket(packetData);
}


void Parser::SendSplitPacket(std::span<const std::uint8_t> m)
{
	while(m.size() > 0)
	{
		m_usb_tx[0] = std::min<int>(m.size(), 63);
		std::copy(begin(m), begin(m) + m_usb_tx[0], begin(m_usb_tx) + 1);
		assert(m_connection != nullptr);
		// std::cout << std::format("Parser::SendSplitPacket 0x{:02x}\n", m_usb_tx[0]);
		m_connection->interrupt_transfer(m_tx_endpoint, m_usb_tx);
		m = m.subspan(m_usb_tx[0]);
	}
}


void Parser::SendWithHeaderAndChecksum(std::span<const std::uint8_t> m)
{
	const int N = m.size();
	assert(N < m_usb_tx.size() - 7);
	m_usb_tx[0] = N + 6;
	m_usb_tx[1] = 0xAA;
	m_usb_tx[2] = 0x55;
	m_usb_tx[3] = N & 0xFF;
	m_usb_tx[4] = N >> 8;
	std::copy(begin(m), end(m), begin(m_usb_tx) + 5);
	int cc = calculateChecksum(std::span(m_usb_tx).subspan(3, N + 2));
	m_usb_tx[N + 7 - 2] = cc >> 8;
	m_usb_tx[N + 7 - 1] = cc & 0xFF;

	assert(m_connection != nullptr);
	m_connection->interrupt_transfer(m_tx_endpoint, m_usb_tx);
}


bool Parser::OnUsbInterruptData(std::span<std::uint8_t> data)
{
	auto frame = m_frame_rx.process(data);
	if(frame.has_value() && m_listener != nullptr)
	{
		m_listener->OnMooerFrame(*frame);
		if(frame->group() == RxFrame::Identify)
		{
			auto version = std::span(data).subspan(7, 5);
			auto name = std::span(data).subspan(12, 11);
			Listener::Identity id{as_string(version), as_string(name)};
			m_listener->OnMooerIdentify(id);
		}
	}
	return true;
}


}; // namespace Mooer
