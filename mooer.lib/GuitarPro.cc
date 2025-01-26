#include <GuitarPro.h>

#include <array>
#include <charconv>
#include <iostream>
#include <map>
#include <ranges>
#include <sstream>

#include <zip.h>


void print(pugi::xml_node node, int level = 0, int indent = 0)
{
	std::string id(" ", indent);
	for(auto& child : node.children())
	{
		std::cout << id << child.name() << "\n";
		if(level > 0)
			print(child, level - 1, indent + 2);
	}
}


std::string as_string(const pugi::xml_node& node)
{
	return node.text().as_string();
}


auto split(const pugi::xml_node& node, char sep = ' ')
{
	std::istringstream in(node.text().as_string());
	std::vector<std::string> r;
	std::string el;
	while(std::getline(in, el, sep))
		r.push_back(el);
	return r;
}


template<typename T>
auto as_vector(const pugi::xml_node& node)
{
	std::istringstream in(node.text().as_string());
	std::vector<T> vec;
	std::copy(std::istream_iterator<T>(in), std::istream_iterator<T>(), std::back_inserter(vec));
	return vec;
}


namespace GuitarPro
{

// https://stackoverflow.com/questions/10440113/simple-way-to-unzip-a-zip-file-using-zlib
class ZipFile
{
public:
	ZipFile(std::span<const char> data)
	{
		zip_error_t error = {ZIP_ER_OK, 0};
		m_zs = zip_source_buffer_create(data.data(), data.size(), 0, &error);
		if(error.zip_err != ZIP_ER_OK)
			throw std::runtime_error(zip_error_strerror(&error));
		m_zip = zip_open_from_source(m_zs, ZIP_RDONLY, &error);
		if(error.zip_err != ZIP_ER_OK)
			throw std::runtime_error(zip_error_strerror(&error));
	}

	ZipFile(std::filesystem::path fn)
		: m_zs(nullptr)
	{
		int err = 0;
		m_zip = zip_open(fn.string().c_str(), 0, &err);
	}

	~ZipFile()
	{
		zip_close(m_zip);
		if(m_zs != nullptr)
			zip_source_close(m_zs);
	}

	bool exists(std::string_view filename) const
	{
		zip_flags_t flags = ZIP_FL_NOCASE;
		return zip_name_locate(m_zip, filename.data(), flags) >= 0;
	}

	struct zip_stat Stat(std::string_view filename) const
	{
		struct zip_stat st;
		zip_stat_init(&st);
		zip_stat(m_zip, filename.data(), 0, &st);
		return st;
	}

	std::vector<char> ExtractFile(std::string_view filename)
	{
		struct zip_stat st;
		zip_stat_init(&st);
		zip_stat(m_zip, filename.data(), 0, &st);

		std::vector<char> data(st.size);

		zip_file* f = zip_fopen(m_zip, filename.data(), 0);
		zip_fread(f, data.data(), st.size);
		zip_fclose(f);
		return data;
	}

	zip* m_zip;
	zip_source_t* m_zs;
};

//-- GPIF --

int GPIF::nrTracks()
{
	return tracks.size();
}


int GPIF::nrBars()
{
	return masterBars.size();
}


//-- AsciiRenderer --


void AsciiRenderer::RenderBars(std::ostream& dst, GPIF& score, int track, Range<int> bars)
{
	struct beat_t
	{
		GPIF::Rhythm duration;
		std::array<int, 6> frets;
	};
	std::vector<beat_t> beats;

	for(int bar = bars.begin; bar < bars.end; bar++)
	{
		score.visitNotes(
			track,
			bar,
			[](Clef key) {},
			[&beats](Clef key, GPIF::Rhythm r, std::span<GPIF::Note> notes)
			{
				beat_t b;
				b.duration = r;
				b.frets.fill(-1);
				for(auto n : notes)
					b.frets[n.string] = n.fret;
				beats.push_back(b);
			});
		beat_t b;
		b.frets.fill(-2);
		beats.push_back(b);
	}

	// beat_t pbeat;pbeat.frets.fill(-3);
	for(int f = beats.front().frets.size() - 1; f >= 0; f--)
	{
		for(auto beat : beats)
		{
			int i = beat.frets.at(f);
			// if(i == pbeat.frets.at(f))i = -1; // Skip continued notes
			if(i == -2)
				dst << "-|";
			else if(i < 0)
				dst << "--";
			else if(i < 10)
				dst << "-" << i;
			else
				dst << i;
			// pbeat = beat;
		}
		dst << "\n";
	}
};


//-- ReaderV78 --


bool ReaderV78::isValid(std::span<const char> data)
{
	const std::string_view pk("PK");
	auto d_pk = data.subspan(0, 2);
	bool is_pkzip = std::equal(begin(d_pk), end(d_pk), begin(pk), end(pk));
	if(!is_pkzip)
		return false;
	ZipFile zf(data);

	if(zf.exists("score.gpif"))
		return false;
	return true;
}


GPIF ReaderV78::Read(std::span<const char> data)
{
	if(!isValid(data))
		return {};
	ZipFile zf(data);
	auto gpifData = zf.ExtractFile(m_fnGPIF);
	pugi::xml_document mGpifXml;
	mGpifXml.load_buffer(gpifData.data(), gpifData.size());
	auto gpifNode = mGpifXml.child("GPIF");
	return Parse(gpifNode);
}


GPIF ReaderV78::Read(std::filesystem::path fn)
{
	ZipFile zf(fn);
	auto gpifData = zf.ExtractFile(m_fnGPIF);
	pugi::xml_document mGpifXml;
	mGpifXml.load_buffer(gpifData.data(), gpifData.size());
	auto gpifNode = mGpifXml.child("GPIF");
	return Parse(gpifNode);
}


GPIF ReaderV78::Parse(pugi::xml_node gpif)
{
	GPIF r;
	// print(gpif, 0);
	r.version = gpif.child("GPVersion").text().as_string();
	r.revision = gpif.child("GPRevision").text().as_int();
	r.tracks = ParseTracks(gpif.child("Tracks"));
	r.masterBars = ParseMasterBars(gpif.child("MasterBars"));
	r.bars = ParseBars(gpif.child("Bars"));
	r.voices = ParseVoices(gpif.child("Voices"));
	r.beats = ParseBeats(gpif.child("Beats"));
	r.notes = ParseNotes(gpif.child("Notes"));
	r.rhythms = ParseRhythms(gpif.child("Rhythms"));
	// ToDo: ScoreViews

	return r;
}


std::vector<GPIF::Track> ReaderV78::ParseTracks(node_t tracks)
{
	std::vector<GPIF::Track> r;
	for(auto& track : tracks.children())
	{
		/* [Sounds/Sound/RSE/EffectChain] has:
			<Effect id="E03_OverdriveScreamer">
			<Parameters>0.84 0.5 0.84</Parameters>
			</Effect>
			<Effect id="E26_CompressorOrange">
			<Parameters>0.6 0.5</Parameters>
			</Effect>
			<Effect id="E07_DistortionGrunge">
			<Parameters>0.8 0.2 0.8 0.6</Parameters>
			</Effect>
			<Effect id="A05_StackBritishVintage">
			<Parameters>0.85 0.67 0.36 0.66 0.52</Parameters>
			</Effect>
			<Effect id="E30_EqGEq">
			<Parameters>0.5 0.5 0.5 0.5 0.5 0.5 0.5 0.342857</Parameters>
		*/
		for(auto sound : track.child("Sounds").children())
		{
			if(auto rse = sound.child("RSE"))
			{
				for(auto fx : rse.child("EffectChain").child("Effect"))
				{
					// get id, parameters
				}
			}
		}

		r.push_back(GPIF::Track{
			as_string(track.child("Name")),
			track.child("PalmMute").text().as_bool(),
		});
	}
	return r;
}


std::vector<GPIF::MasterBar> ReaderV78::ParseMasterBars(node_t mbars)
{
	std::vector<GPIF::MasterBar> r;
	for(auto& mbar : mbars.children())
	{
		auto time = split(mbar.child("Time"), '/');
		r.push_back(GPIF::MasterBar{{std::stoi(time[0]), std::stoi(time[1])}, as_vector<int>(mbar.child("Bars"))});
	}
	return r;
}


std::vector<GPIF::Bar> ReaderV78::ParseBars(node_t bars)
{
	std::vector<GPIF::Bar> r;
	for(auto& bar : bars.children())
	{
		r.push_back(GPIF::Bar{Clef::A, as_vector<int>(bar.child("Voices"))});
	}
	return r;
}


std::vector<GPIF::Voice> ReaderV78::ParseVoices(node_t voices)
{
	std::vector<GPIF::Voice> r;
	for(auto& voice : voices.children())
	{
		r.push_back(GPIF::Voice{as_vector<int>(voice.child("Beats"))});
	}
	return r;
}


std::vector<GPIF::Beat> ReaderV78::ParseBeats(node_t beats)
{
	std::vector<GPIF::Beat> r;
	for(auto& beat : beats.children())
	{
		int idx = beat.attribute("id").as_int();
		r.resize(idx + 1);
		r[idx] = {Clef::A, beat.child("Rhythm").attribute("ref").as_int(), as_vector<int>(beat.child("Notes"))};
	}
	return r;
}


std::vector<GPIF::Note> ReaderV78::ParseNotes(node_t notes)
{
	std::vector<GPIF::Note> r;
	for(auto& note : notes.children())
	{
		GPIF::Note n;
		for(auto prop : note.child("Properties").children())
		{
			if(std::string_view(prop.attribute("name").as_string()) == "Fret")
				n.fret = prop.child("Fret").text().as_int();
			if(std::string_view(prop.attribute("name").as_string()) == "String")
				n.string = prop.child("String").text().as_int();
		}
		if(auto lr = note.child("LetRing"))
			n.letRing = true;
		else
			n.letRing = false;
		r.push_back(n);
	}
	return r;
}


std::vector<GPIF::Rhythm> ReaderV78::ParseRhythms(node_t rhythms)
{
	std::map<std::string_view, NoteValue> dmap = {
		{"Whole", NoteValue::Whole},
		{"Half", NoteValue::Half},
		{"Quarter", NoteValue::Quarter},
		{"Eighth", NoteValue::Eighth},
		{"16th", NoteValue::_16th},
		{"32nd", NoteValue::_32nd},
	};

	std::vector<GPIF::Rhythm> r;
	for(auto& rhythm : rhythms.children())
	{
		GPIF::Rhythm rh;
		auto duration = rhythm.child("NoteValue");
		rh.value = dmap.at(duration.text().as_string());
		if(auto aug = rhythm.child("AugmentationDot"))
			rh.augmentation = aug.attribute("count").as_int();
		r.push_back(rh);
	}
	return r;
}



} // namespace GuitarPro
