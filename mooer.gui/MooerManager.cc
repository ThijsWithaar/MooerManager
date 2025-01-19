#include "MooerManager.h"

#include <filesystem>
#include <fstream>

#include <QFileDialog>


//#define DEBUG_LVL 3


const QString ptFileOpen = "";

bool ichar_equals(char a, char b)
{
	return std::tolower(a) == std::tolower(b);
}


bool iequals(const std::string& a, const std::string& b)
{
	return std::equal(a.begin(), a.end(), b.begin(), b.end(), ichar_equals);
}


template<class Object>
	requires(std::derived_from<Object, QObject>)
class WBS
{
public:
	WBS(Object* object)
		: m_object(object)
	{
		assert(m_object != nullptr);
		m_object->blockSignals(true);
	}

	~WBS()
	{
		m_object->blockSignals(false);
	}

	Object* operator->()
	{
		return m_object;
	}

private:
	Object* m_object;
};


std::vector<std::uint8_t> ReadFile(std::filesystem::path fn)
{
	std::vector<std::uint8_t> data(std::filesystem::file_size(fn));
	std::ifstream f(fn, std::ios::binary);
	f.read(reinterpret_cast<char*>(data.data()), data.size());
	return data;
}


MooerManager::MooerManager(QWidget* parent)
	: m_usb({Mooer::vendor_id, Mooer::product_id})
	, m_mooer(&m_usb, this)
	, m_need_patches(true)
#ifdef MOOER_HAS_MIDI
	, m_midi(MIDI::Interface::Create("MooerManager", this))
#endif
{
	m_ui.setupUi(this);
	m_ui.centralwidget->setEnabled(false);

	m_ui.cb_amp_type->clear();
	for(auto name : Mooer::amp_model_names)
		m_ui.cb_amp_type->addItem(QString::fromStdString(std::string(name)));

	m_ui.cb_cab_type->clear();
	for(auto name : Mooer::cab_model_names)
		m_ui.cb_cab_type->addItem(QString::fromStdString(std::string(name)));

	m_ui.cbPatch->clear();
	for(int n = 0; n < 200; n++)
		m_ui.cbPatch->addItem(QString("Empty %1").arg(n));

	connect(m_ui.action_Quit, &QAction::triggered, QApplication::instance(), &QApplication::quit);
	connect(m_ui.pbIdentify, &QPushButton::clicked, [&](bool) { m_mooer.SendIdentifyRequest(); });
	connect(m_ui.pbPatchList, &QPushButton::clicked, [&](bool) { m_mooer.SendPatchListRequest(); });
	connect(m_ui.cbPatch,
			&QComboBox::currentIndexChanged,
			[&](int index)
			{
				SwitchMenuIfDifferent(0);
				m_mooer.SendPresetChange(index);
			});
	connect(m_ui.pbLoadPreset,
			&QPushButton::clicked,
			[&](bool)
			{
				auto fileName =
					QFileDialog::getOpenFileName(this, tr("Open Preset"), ptFileOpen, tr("Preset Files (*.mo)"));
				std::filesystem::path fnPreset = fileName.toStdString();
				auto data = ReadFile(fnPreset);
				m_mooer.LoadMoPreset(data);
			});

	ConnectFX();
	ConnectDistortion();
	ConnectAmplifier();
	ConnectCabinet();
	ConnectNoiseGate();
	ConnectEqualizer();
	ConnectModulator();

	connect(
		this,
		&MooerManager::MooerIdentity,
		this,
		[&](QString version, QString name)
		{
			qDebug() << "Connected to " << name;
			m_ui.statusbar->showMessage(QString("Connected to %1, version %2").arg(name, version), 5 * 1000);
			if(m_need_patches)
			{
				m_mooer.SendFlush();
				m_mooer.SendPatchListRequest();
				m_need_patches = false;
			}
		},
		Qt::QueuedConnection);

	connect(
		this,
		&MooerManager::MooerPatchSetting,
		this,
		[&](int patchIndex)
		{
			const int maxPatchIdx = 199;
			if(patchIndex < maxPatchIdx)
			{
				int pct = (patchIndex * 100) / 199;
				auto msg = QString("Downloading settings: %1%%").arg(pct);
				m_ui.statusbar->showMessage(msg);
			}
			else
			{
				m_ui.centralwidget->setEnabled(true);
				m_ui.statusbar->showMessage("Settings Downloaded", 1000);
				UpdatePatchDropdown();
				UpdateSettingsView(Mooer::RxFrame::Group::ActivePatch);
			}
		},
		Qt::QueuedConnection);

	connect(this,
			&MooerManager::MooerPatchChange,
			[&](int index)
			{
				// qDebug() << QString("Active Patch: %1").arg(index + 1);
				WBS(m_ui.cbPatch)->setCurrentIndex(index);
			});

	connect(this, &MooerManager::MooerSettingsChanged, this, &MooerManager::UpdateSettingsView);

	m_usb.StartEventLoop();

	// This is the sequence MooerStudio sends out
	m_mooer.SendIdentifyRequest();
	m_mooer.SendIdentifyRequest();
	qDebug() << "MooerManager: constructor finished";
}


// clang-format off

void MooerManager::ConnectFX()
{
	auto* pFx = &m_mstate.activePreset.fx;
	auto send = [this]() {
		SwitchMenuIfDifferent(1);
		m_mooer.SetFX(m_mstate.activePreset.fx);
	};

	connect(m_ui.cb_fx_enabled,	&QCheckBox::clicked, 
			[=](bool e){ pFx->enabled = e; send(); });
	connect(m_ui.cb_fx_type, &QComboBox::currentIndexChanged,
			[=](int idx){ pFx->type = idx + 1; send(); });
	connect(m_ui.s_fx_p1, &QSlider::valueChanged,
			[=](int v){	pFx->q = v;	send(); });
	connect(m_ui.s_fx_p2, &QSlider::valueChanged, 
			[=](int v){	pFx->position = v; send(); });
	connect(m_ui.s_fx_p3, &QSlider::valueChanged,
			[=](int v){	pFx->peak = v; send(); });
	connect(m_ui.s_fx_p4, &QSlider::valueChanged,
			[=](int v){	pFx->level = v; send(); });
}


void MooerManager::ConnectDistortion()
{
	auto* pS = &m_mstate.activePreset.distortion;
	auto send = [this]() {
		SwitchMenuIfDifferent(2);
		m_mooer.SetDS(m_mstate.activePreset.distortion);
	};

	connect(m_ui.cb_ds_enabled, &QCheckBox::clicked,
		    [=](bool e){ pS->enabled = e; send(); });
	connect(m_ui.cb_ds_type, &QComboBox::currentIndexChanged,
			[=](int i){ pS->type = i + 1; send(); });
	connect(m_ui.s_ds_p1, &QSlider::valueChanged,
			[=](int v){ pS->volume = v; send(); });
	connect(m_ui.s_ds_p2, &QSlider::valueChanged,
			[=](int v){ pS->tone = v; send(); });
	connect(m_ui.s_ds_p3, &QSlider::valueChanged,
			[=](int v){ pS->gain = v; send(); });
}


void MooerManager::ConnectAmplifier()
{
	auto* pS = &m_mstate.activePreset.amp;
	auto send = [this]() {
		SwitchMenuIfDifferent(3);
		m_mooer.SetAmplifier(m_mstate.activePreset.amp);
	};

	connect(m_ui.cb_amp_enabled, &QCheckBox::clicked,
		    [=](bool e){ pS->enabled = e; send(); });
	connect(m_ui.cb_amp_type, &QComboBox::currentIndexChanged,
			[=, this](int i){
				m_ui.pb_amp_load->setEnabled(i >= 55);
				pS->type = i+1; send();
			});
	connect(m_ui.pb_amp_load, &QPushButton::clicked,
			[this]() { OnAmpLoad(); });
	connect(m_ui.s_amp_gain, &QSlider::valueChanged,
			[=](int v){ pS->gain = v; send(); });
	connect(m_ui.s_amp_bass, &QSlider::valueChanged,
			[=](int v){ pS->bass = v; send(); });
	connect(m_ui.s_amp_mid, &QSlider::valueChanged,
			[=](int v){ pS->mid = v; send(); });
	connect(m_ui.s_amp_treble, &QSlider::valueChanged,
			[=](int v){ pS->treble = v; send(); });
	connect(m_ui.s_amp_pres, &QSlider::valueChanged,
			[=](int v){ pS->pres = v; send(); });
	connect(m_ui.s_amp_mst, &QSlider::valueChanged,
			[=](int v){ pS->mst = v; send(); });
}


void MooerManager::ConnectCabinet()
{
	auto* pS = &m_mstate.activePreset.cab;
	auto send = [this]() {
		SwitchMenuIfDifferent(4);
		m_mooer.SetCabinet(m_mstate.activePreset.cab);
	};

	connect(m_ui.cb_cab_enabled, &QCheckBox::clicked,
			[=](bool e){ pS->enabled = e; send(); });
	connect(m_ui.cb_cab_type, &QComboBox::currentIndexChanged,
			[this](int i){
				m_ui.pb_cab_load->setEnabled(i >= 26);
			});
	connect(m_ui.pb_cab_load, &QPushButton::clicked, [&]() { OnCabinetLoad(); });
	connect(m_ui.cb_cab_tube, &QComboBox::currentIndexChanged,
			[=](int v){ pS->tube = v; send(); });
	connect(m_ui.cb_cab_mic, &QComboBox::currentIndexChanged,
			[=](int v){ pS->mic = v; send(); });
	connect(m_ui.d_cab_center, &QDial::valueChanged,
			[=](int v){ pS->center = v; send(); });
	connect(m_ui.d_cab_distance, &QDial::valueChanged,
			[=](int v){ pS->distance = v; send(); });
}


void MooerManager::ConnectNoiseGate()
{
	auto* pS = &m_mstate.activePreset.noiseGate;
	auto send = [this]() {
		SwitchMenuIfDifferent(5);
		m_mooer.SetNoiseGate(m_mstate.activePreset.noiseGate);
	};

	connect(m_ui.cb_ns_enabled, &QCheckBox::clicked,
			[=](bool e){ pS->enabled = e; send(); });
	connect(m_ui.cb_ns_type, &QComboBox::currentIndexChanged,
			[=](int i){ pS->type = i+1; send(); });
	connect(m_ui.sl_ns_p1, &QSlider::valueChanged,
			[=](int v){ pS->attack = v; send(); });
	connect(m_ui.sl_ns_p2, &QSlider::valueChanged,
			[=](int v){ pS->release = v; send(); });
	connect(m_ui.sl_ns_p3, &QSlider::valueChanged,
			[=](int v){ pS->thresh = v; send(); });
}


void MooerManager::ConnectEqualizer()
{
	auto* pS = &m_mstate.activePreset.equalizer;
	auto send = [this]() {
		SwitchMenuIfDifferent(6);
		m_mooer.SetEQ(m_mstate.activePreset.equalizer);
	};

	connect(m_ui.cb_eq_enabled, &QCheckBox::clicked,
			[=](bool e){ pS->enabled = e; send(); });
	connect(m_ui.cb_eq_type, &QComboBox::currentIndexChanged,
			[=](int i){ pS->type = i+1; send(); });
	connect(m_ui.s_eq_band1, &QSlider::valueChanged,
			[=](int v){ pS->band[0] = v; send(); });
	connect(m_ui.s_eq_band2, &QSlider::valueChanged,
			[=](int v){ pS->band[1] = v; send(); });
	connect(m_ui.s_eq_band3, &QSlider::valueChanged,
			[=](int v){ pS->band[2] = v; send(); });
	connect(m_ui.s_eq_band4, &QSlider::valueChanged,
			[=](int v){ pS->band[3] = v; send(); });
}


void MooerManager::ConnectModulator()
{
	auto* pS = &m_mstate.activePreset.modulation;
	auto send = [this]() {
		SwitchMenuIfDifferent(7);
		m_mooer.SetModulator(m_mstate.activePreset.modulation);
	};

	connect(m_ui.cb_mod_enabled, &QCheckBox::clicked,
			[=](bool e){ pS->enabled = e; send(); });
	connect(m_ui.cb_mod_type, &QComboBox::currentIndexChanged,
			[=](int i){ pS->type = i + 1; send(); });
	connect(m_ui.s_mod_p1, &QSlider::valueChanged,
			[=](int v){ pS->rate = v; send(); });
	connect(m_ui.s_mod_p2, &QSlider::valueChanged,
			[=](int v){ pS->level = v; send(); });
	connect(m_ui.s_mod_p3, &QSlider::valueChanged,
			[=](int v){ pS->depth = v; send(); });
}

// clang-format on


void MooerManager::OnAmpLoad()
{
	int amp_idx = m_ui.cb_amp_type->currentIndex();
	int slot_idx = amp_idx - 55;
	if(slot_idx < 0)
		return;

#if DEBUG_LVL > 2
	// std::string fnAmp = "/home/thijs/gitrepo/mooer/traces/CALI DUAL 1.amp";
	std::filesystem::path fnAmp = "/home/thijs/gitrepo/mooer/traces/E-MARSHALL PLEXI CRUNCH AV.GNR";
#else
	auto fileName = QFileDialog::getOpenFileName(
		this, tr("Open Amplifier"), ptFileOpen, tr("Amplifier model (*.amp);;MNRS profile (*.gnr)"));
	std::filesystem::path fnAmp = fileName.toStdString();
#endif

	std::string name = fnAmp.stem().string();
	auto ampData = ReadFile(fnAmp);
	if(iequals(fnAmp.extension().string(), ".amp"))
		m_mooer.LoadAmplifier(ampData, name, slot_idx);
	else if(iequals(fnAmp.extension().string(), ".gnr"))
		m_mooer.LoadGNR(ampData, name, slot_idx);
	else
		throw std::runtime_error("Amplifier File format not supported");

	m_mstate.ampModelNames.set(slot_idx, name);
	emit MooerSettingsChanged(Mooer::RxFrame::AMP);
}


void MooerManager::OnCabinetLoad()
{
	int amp_idx = m_ui.cb_amp_type->currentIndex();
	int slot_idx = amp_idx - 26;
	if(slot_idx < 0)
		return;

	// std::string fnCab = "/home/thijs/gitrepo/mooer/traces/impulse V30 3 LPR.wav";
	auto fileName =
		QFileDialog::getOpenFileName(this, tr("Open Cabinet (Impulse Response)"), ptFileOpen, tr("WAVE Files (*.wav)"));
	std::filesystem::path fnCab = fileName.toStdString();

	std::string name = std::filesystem::path(fnCab).stem().string();
	auto ampData = ReadFile(fnCab);
	m_mooer.LoadWav(ampData, name, slot_idx);
	m_mstate.ampModelNames.set(slot_idx, name);
	emit MooerSettingsChanged(Mooer::RxFrame::CAB);
}


void MooerManager::OnUsbConnection()
{
	qInfo() << "MooerManager::OnUsbConnection()"; // << std::flush;
}


void MooerManager::SwitchMenuIfDifferent(int menu)
{
	if(m_mstate.activeMenu == menu)
		return;
	m_mooer.SwitchMenu(menu);
	m_mstate.activeMenu = menu;
}


std::string_view as_string_view(std::span<const std::uint8_t> s)
{
	return std::string_view((const char*)s.data(), s.size());
}


void MooerManager::OnMooerFrame(const Mooer::RxFrame::Frame& frame)
{
	// No UI access, since this is called from a background thread.
	std::lock_guard lock{m_dev_mutex};

	std::span<const std::uint8_t> data = frame.noidx_data();
	auto data_nochk = frame.nochecksum_data();

	switch(frame.group())
	{
	case Mooer::RxFrame::Identify:
		// Handled by OnMooerIdentify()
		break;
	case Mooer::RxFrame::PedalAssignment_Maybe:
	{
		std::string fw_version(as_string_view(data.subspan(1, 5)));		 // "2.0.4"
		std::string model_name(as_string_view(data.subspan(1 + 5, 11))); // "MOOER_GE200"
		qDebug() << QString("FWVersion %1, Model %2").arg(fw_version.c_str()).arg(model_name.c_str());
	}
	break;
	case Mooer::RxFrame::CabinetUpload:
		qDebug() << QString("MooerManager: Cabinet Acknowledge received for %1").arg(frame.index());
	case Mooer::RxFrame::AmpUpload:
		qDebug() << QString("MooerManager: Amp Acknowledge received for %1").arg(frame.index());
		break;
	case Mooer::RxFrame::ActivePatch:
		emit MooerPatchChange(frame.index());
		m_mstate.activePresetIndex = frame.index();
#ifdef MOOER_HAS_MIDI
		if(m_midi)
			m_midi->ProgramChange(0, m_mstate.activePresetIndex);
#endif
		break;
	case Mooer::RxFrame::AmpModels:
		m_mstate.ampModelNames = Mooer::DeviceFormat::AmpModelNames(frame.data);
		emit MooerSettingsChanged(Mooer::RxFrame::AMP);
		break;
	case Mooer::RxFrame::CabModels:
#if DEBUG_LVL > 0
		qDebug() << "Received CAB models";
		m_mstate.cabModelNames = Mooer::DeviceFormat::AmpModelNames(frame.data);
		// assert(!"Not Implemented");
#endif
		emit MooerSettingsChanged(Mooer::RxFrame::CAB);
		break;
	case Mooer::RxFrame::PatchSetting:
	{
#if DEBUG_LVL > 3
		Mooer::File::PresetPadded preset = data_nochk.subspan(1);
		qDebug() << std::format("MooerManager: received patch {:3d} {}", frame.index(), preset.getName());
#endif
		if(data_nochk.size() == 0x201)
		{
			m_mstate.savedPresets.at(frame.index()) = data_nochk.subspan(1);
			emit MooerPatchSetting(frame.index());
		}
		else
		{
			qDebug() << QString("Received invalid preset of size %1").arg(data_nochk.size());
		}
	}
	break;
	case Mooer::RxFrame::ActivePatchSetting:
	{
		auto idx = data_nochk[0];
		qDebug() << QString("Received Preset data for %1").arg(idx);
		// m_mstate.savedPresets.at(idx) = data_nochk.subspan(1);
	}
	break;
	case Mooer::RxFrame::FX:
		m_mstate.activePreset.fx = data_nochk;
		emit MooerSettingsChanged(frame.group());
		break;
	case Mooer::RxFrame::DS_OD:
		m_mstate.activePreset.distortion = data_nochk;
		emit MooerSettingsChanged(frame.group());
		break;
	case Mooer::RxFrame::AMP:
		m_mstate.activePreset.amp = data_nochk;
		emit MooerSettingsChanged(frame.group());
		break;
	case Mooer::RxFrame::CAB:
		m_mstate.activePreset.cab = data_nochk;
		emit MooerSettingsChanged(frame.group());
		break;
	case Mooer::RxFrame::NS_GATE:
		m_mstate.activePreset.noiseGate = data_nochk;
		emit MooerSettingsChanged(frame.group());
		break;
	case Mooer::RxFrame::EQ:
		m_mstate.activePreset.equalizer = data_nochk;
		emit MooerSettingsChanged(frame.group());
		break;
	case Mooer::RxFrame::MOD:
		m_mstate.activePreset.modulation = data_nochk;
		emit MooerSettingsChanged(frame.group());
		break;
	case Mooer::RxFrame::DELAY:
		m_mstate.activePreset.delay = data_nochk;
		emit MooerSettingsChanged(frame.group());
		break;
	case Mooer::RxFrame::REVERB:
		m_mstate.activePreset.reverb = data_nochk;
		emit MooerSettingsChanged(frame.group());
		break;
	case Mooer::RxFrame::RHYTHM:
		m_mstate.activePreset.rhythm = data_nochk;
		emit MooerSettingsChanged(frame.group());
		break;
	case Mooer::RxFrame::Menu:
		m_mstate.activeMenu = data_nochk[0];
		break;
	default:
		qDebug() << QString("MooerManager: received frame 0x%1, size %2")
						.arg((int)frame.group(), 0, 16)
						.arg(frame.data.size());
		break;
	}
#if DEBUG_LVL > 2
	if(frame.group() != Mooer::RxFrame::PatchSetting && (data.size() > 20))
	{
		int sz = frame.data.size();
		qDebug() << std::format("MooerManager: received frame {:04x}, size {}", frame.key, frame.data.size());
		sz++;
	}
#endif
}


void MooerManager::OnMooerIdentify(const Mooer::Listener::Identity& id)
{
	emit MooerIdentity(QString::fromStdString(id.version), QString::fromStdString(id.name));
}


void MooerManager::OnControlChange(std::uint8_t channel, MIDI::ControlChange controller, std::uint8_t value)
{
	// std::lock_guard lock{m_dev_mutex};
}


void MooerManager::OnProgramChange(std::uint8_t channel, std::uint8_t value)
{
	// std::lock_guard lock{m_dev_mutex};
}


void MooerManager::OnSysex(std::uint8_t channel, MIDI::Manufacturer manufacturer, std::span<std::uint8_t> data)
{
	// std::lock_guard lock{m_dev_mutex};
}


void MooerManager::UpdateSettingsView(Mooer::RxFrame::Group group)
{
	auto& preset = m_mstate.activePreset;

	if(group == Mooer::RxFrame::Group::FX)
	{
		WBS(m_ui.cb_fx_enabled)->setChecked(preset.fx.enabled != 0);
		WBS(m_ui.cb_fx_type)->setCurrentIndex(preset.fx.type - 1);
		WBS(m_ui.s_fx_p1)->setValue(preset.fx.q);
		WBS(m_ui.s_fx_p2)->setValue(preset.fx.position);
		WBS(m_ui.s_fx_p3)->setValue(preset.fx.peak);
		WBS(m_ui.s_fx_p4)->setValue(preset.fx.level);
	}

	if(group == Mooer::RxFrame::Group::DS_OD)
	{
		WBS(m_ui.cb_ds_enabled)->setChecked(preset.distortion.enabled);
		WBS(m_ui.cb_ds_type)->setCurrentIndex(preset.distortion.type - 1);
		WBS(m_ui.s_ds_p1)->setValue(preset.distortion.volume);
		WBS(m_ui.s_ds_p2)->setValue(preset.distortion.tone);
		WBS(m_ui.s_ds_p3)->setValue(preset.distortion.gain);
	}

	if(group == Mooer::RxFrame::Group::AMP)
	{
		QSignalBlocker bAmp(m_ui.cb_amp_type);
		// qDebug() << std::format("{} ampNames, {} dropdowns", m_mstate.ampModelNames.size(),
		// m_ui.cb_amp_type->count());
		for(int n = 0; n < m_mstate.ampModelNames.size(); n++)
		{
			auto name = QString::fromLatin1(m_mstate.ampModelNames[n].data(), m_mstate.ampModelNames[n].size());
			m_ui.cb_amp_type->setItemText(n + 55, name);
			// qDebug() << "UpdateSettingsView " << (n + 55) << " " << name;
		}
		WBS(m_ui.cb_amp_enabled)->setChecked(preset.amp.enabled);
		WBS(m_ui.cb_amp_type)->setCurrentIndex(preset.amp.type - 1);
		WBS(m_ui.s_amp_gain)->setValue(preset.amp.gain);
		WBS(m_ui.s_amp_bass)->setValue(preset.amp.bass);
		WBS(m_ui.s_amp_mid)->setValue(preset.amp.mid);
		WBS(m_ui.s_amp_treble)->setValue(preset.amp.treble);
		WBS(m_ui.s_amp_pres)->setValue(preset.amp.pres);
		WBS(m_ui.s_amp_mst)->setValue(preset.amp.mst);
	}

	if(group == Mooer::RxFrame::Group::CAB)
	{
		WBS(m_ui.cb_cab_enabled)->setChecked(preset.cab.enabled);
		WBS(m_ui.cb_cab_type)->setCurrentIndex(preset.cab.type - 1);
		WBS(m_ui.cb_cab_tube)->setCurrentIndex(preset.cab.tube);
		WBS(m_ui.cb_cab_mic)->setCurrentIndex(preset.cab.mic);
		WBS(m_ui.d_cab_center)->setValue(preset.cab.center);
		WBS(m_ui.d_cab_distance)->setValue(preset.cab.distance);
	}

	if(group == Mooer::RxFrame::Group::NS_GATE)
	{
		auto& ps = preset.noiseGate;
		WBS(m_ui.cb_ns_enabled)->setChecked(ps.enabled);
		WBS(m_ui.cb_ns_type)->setCurrentIndex(ps.type - 1);
		WBS(m_ui.sl_ns_p1)->setValue(ps.attack);
		WBS(m_ui.sl_ns_p2)->setValue(ps.release);
		WBS(m_ui.sl_ns_p3)->setValue(ps.thresh);
	}

	if(group == Mooer::RxFrame::Group::EQ)
	{
		auto& ps = preset.equalizer;
		WBS(m_ui.cb_eq_enabled)->setChecked(ps.enabled);
		WBS(m_ui.cb_eq_type)->setCurrentIndex(ps.type - 1);
		WBS(m_ui.s_eq_band1)->setValue(ps.band[0]);
		WBS(m_ui.s_eq_band2)->setValue(ps.band[1]);
		WBS(m_ui.s_eq_band3)->setValue(ps.band[2]);
		WBS(m_ui.s_eq_band4)->setValue(ps.band[3]);
	}

	if(group == Mooer::RxFrame::Group::MOD)
	{
		auto& ps = preset.modulation;
		WBS(m_ui.cb_mod_enabled)->setChecked(ps.enabled);
		WBS(m_ui.cb_mod_type)->setCurrentIndex(ps.type - 1);
		WBS(m_ui.s_mod_p1)->setValue(ps.rate);
		WBS(m_ui.s_mod_p2)->setValue(ps.level);
		WBS(m_ui.s_mod_p3)->setValue(ps.depth);
	}
}


void MooerManager::UpdatePatchDropdown()
{
	auto& presets = m_mstate.savedPresets;
	QStringList items(presets.size());
	for(int n = 0; n < presets.size(); n++)
	{
		// std::string pn = std::format("{:-3d}: {}", n + 1, m_patches[n].name());
		//  qDebug() << pn;
		// items[n].sprintf("%-3i: %s", n + 1, m_patches[n].name());
		QString name = QString::fromStdString(std::string(presets[n].getName()));
		items[n] = QString("%1: %2").arg(n + 1).arg(name);
	}

	QSignalBlocker sb(m_ui.cbPatch);
	auto idx = m_ui.cbPatch->currentIndex();
	m_ui.cbPatch->clear();
	m_ui.cbPatch->addItems(items);
	m_ui.cbPatch->setCurrentIndex(idx);
}
