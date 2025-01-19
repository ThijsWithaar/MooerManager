#pragma once

#include <filesystem>
#include <span>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <pugixml.hpp>


namespace GuitarPro
{

enum class Clef : std::uint8_t
{
	A,
	B,
	C,
	D,
	E,
	F
};

enum class NoteValue : std::uint8_t
{
	Whole,
	Half,
	Quarter,
	Eighth,
	_16th,
	_32nd
};

class BarVisitor
{
public:
	virtual void OnBar(Clef clef) {};
	virtual void OnVoice() {};
	virtual void OnBeat(NoteValue duration) {};
	virtual void OnNote(int string, int fret) {};
};


struct GPIF
{
	struct Track
	{
		std::string name;
		bool palmMute;
	};

	struct Time
	{
		int num, den;
	};

	struct MasterBar
	{
		Time time;
		std::vector<int> bars; ///< 1 bar per track
	};

	struct Bar
	{
		Clef clef;
		std::vector<int> voices;
	};

	struct Voice
	{
		std::vector<int> beats;
	};

	struct Beat
	{
		Clef dynamic;
		int rhythm;
		std::vector<int> notes;
	};

	struct Note
	{
		std::string pitch_step;
		int pitch_octave;
		int fret;	  ///< 0-base?
		int string;	  ///< 0-based
		bool letRing; ///< This makes it repeat for all succesive notes
	};

	struct Rhythm
	{
		NoteValue value;
		std::uint8_t augmentation;
	};

	int nrTracks();

	int nrBars();

	template<std::invocable<Clef> CbBar, std::invocable<Clef, Rhythm, std::span<Note>> CbBeat>
	void visitNotes(int idx_track, int idx_bar, CbBar&& cbBar, CbBeat&& cbBeat)
	{
		const Bar& vbar = bars[masterBars[idx_bar].bars[idx_track]];
		cbBar(vbar.clef);
		for(int idx_voice : vbar.voices)
		{
			if(idx_voice < 0)
				continue;
			for(int idx_beat : voices[idx_voice].beats)
			{
				std::vector<Note> notes;
				Beat& beat = beats.at(idx_beat);
				for(int idx_note : beat.notes)
					notes.push_back(this->notes.at(idx_note));
				Rhythm& r = rhythms.at(beat.rhythm);
				cbBeat(beat.dynamic, r, notes);
			}
		}
	}

	std::string version;
	int revision;

private:
	friend class ReaderV78;

	std::vector<Track> tracks;
	std::vector<MasterBar> masterBars; ///< Entry point for tab rendering
	std::vector<Bar> bars;
	std::vector<Voice> voices;
	std::vector<Beat> beats;
	std::vector<Note> notes;
	std::vector<Rhythm> rhythms;
};

/*
ToDo: Unicode music symbols https://www.unicode.org/charts/PDF/U1D100.pdf
	Vibrato: 1D19D
	Bend up: 1D149

debian: fonts-noto-core
*/
class AsciiRenderer
{
public:
	template<typename T>
	struct Range
	{
		T begin, end; ///< end is exclusive
	};

	///< Number of bars in the track, given a max. column width
	int NrBars(GPIF& score, int track, int colwidth);

	static void RenderBars(std::ostream& dst, GPIF& score, int track, Range<int> bars);

private:
	static int BarWidth(GPIF& score, int track, int bar);
};


/// Version 7,8: Zipped .xml
class ReaderV78
{
public:
	/// Returns true if this is a valid/parseable file
	static bool isValid(std::span<const char> data);

	static GPIF Read(std::span<const char> data);
	static GPIF Read(std::filesystem::path fname);

private:
	using node_t = pugi::xml_node;

	static GPIF Parse(node_t gpif);
	static std::vector<GPIF::Track> ParseTracks(node_t tracks);
	static std::vector<GPIF::MasterBar> ParseMasterBars(node_t mbars);
	static std::vector<GPIF::Bar> ParseBars(node_t bars);
	static std::vector<GPIF::Voice> ParseVoices(node_t voices);
	static std::vector<GPIF::Beat> ParseBeats(node_t beats);
	static std::vector<GPIF::Note> ParseNotes(node_t notes);
	static std::vector<GPIF::Rhythm> ParseRhythms(node_t rhythms);

	constexpr static std::string_view m_fnGPIF = "Content/score.gpif";
	// pugi::xml_document mGpifXml;
};


} // namespace GuitarPro
