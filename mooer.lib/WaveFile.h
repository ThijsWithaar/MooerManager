#pragma once

#include <array>
#include <cstdint>


namespace RIFF
{

using id_t = std::array<char, 4>;
using u16 = std::uint16_t;
using u32 = std::uint32_t;

constexpr id_t fileTypeId = {'R', 'I', 'F', 'F'};
constexpr id_t fileFormatId = {'W', 'A', 'V', 'E'};
constexpr id_t fmtChunkId = {'f', 'm', 't', ' '};
constexpr id_t dataId = {'d', 'a', 't', 'a'};


enum AudioFormat : std::uint16_t
{
	PCM = 1,
	IEEE754 = 3
};

struct Chunk
{
	id_t dataId; // "data"
	u32 dataSize;
	std::uint8_t data[];
};

struct Master
{
	id_t fileTypeId;   // RIFF
	u32 fileSize;	   // Overall file size minus 8 bytes
	id_t fileFormatId; // WAVE

	id_t formatId; // "fmt "
	u32 blockSize;
	AudioFormat audioFormat;
	u16 nChannels;
	u32 frequency;
	u32 bytesPerSec;
	u16 bytesPerBlock;
	u16 bitsPerSample;

	// Followed by chunks
};
static_assert(sizeof(Master) + sizeof(Chunk) == 44);

bool isValidRiff(std::span<const std::uint8_t> riff)
{
	if(riff.size() < sizeof(Master))
		return false;
	auto hdr = reinterpret_cast<const Master*>(riff.data());
	return (hdr->fileTypeId == fileTypeId) && (hdr->fileFormatId == fileFormatId);
}

std::optional<std::span<const std::uint8_t>> FindChunk(std::span<const std::uint8_t> riff, id_t id)
{
	if(!isValidRiff(riff))
		return std::nullopt;
	u32 idx = sizeof(Master);
	while(riff.size() > sizeof(Chunk))
	{
		auto hdr = reinterpret_cast<const Chunk*>(riff.data());
		if(hdr->dataId == id)
			return std::span<const std::uint8_t>{&hdr->data[0], hdr->dataSize};
		riff = riff.subspan(sizeof(Chunk) + hdr->dataSize);
	}
	return std::nullopt;
};


} // namespace RIFF
