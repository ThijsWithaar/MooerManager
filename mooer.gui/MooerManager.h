#pragma once

#include <mutex>

#include <QMainWindow>

#include <MooerParser.h>
#include <UsbConnection.h>
#include <midi/Midi.h>

#if defined(__linux__) || defined(_WIN32)
#define MOOER_HAS_MIDI
#endif


#include "ui_MooerManager.h"


class MooerManager : public QMainWindow, USB::ConnectionListener, Mooer::Listener, MIDI::Callback
{
	Q_OBJECT
public:
	MooerManager(QWidget* parent);

	~MooerManager();

signals:
	void MooerConnected();
	void MooerIdentity(QString version, QString name);
	void MooerPatchSetting(int idx); ///< Received the settings for this patch
	void MooerPatchChange(int idx);	 ///< Index of the active patch
	void MooerSettingsChanged(Mooer::RxFrame::Group group);

private:
	void SwitchMenuIfDifferent(int);

	// USB::ConnectionListener
	void OnUsbConnected(bool) override;

	// Mooer::Listener
	void OnMooerFrame(const Mooer::RxFrame::Frame& frame) override;
	void OnMooerIdentify(const Mooer::Listener::Identity& id) override;

	// MIDI::Callback
	void OnControlChange(std::uint8_t channel, MIDI::ControlChange controller, std::uint8_t value) override;
	void OnProgramChange(std::uint8_t channel, std::uint8_t value) override;
	void OnSysex(std::uint8_t channel, MIDI::Manufacturer manufacturer, std::span<std::uint8_t> data) override;

	// GUI setup
	void ConnectFX();
	void ConnectDistortion();
	void ConnectAmplifier();
	void ConnectCabinet();
	void ConnectNoiseGate();
	void ConnectEqualizer();
	void ConnectModulator();

	// GUI slots
	void OnAmpLoad();
	void OnCabinetLoad();
	void UpdatePatchDropdown();
	void UpdateSettingsView(Mooer::RxFrame::Group group);

	// GUI status
	Ui::MainWindow m_ui;
	USB::Connection m_usb;
	Mooer::Parser m_mooer;

	// Device
	std::mutex m_dev_mutex;
	bool m_need_patches;
	Mooer::Listener::Identity m_device_id;
	Mooer::DeviceFormat::State m_mstate; // Device state
#if defined(MOOER_HAS_MIDI)
	std::unique_ptr<MIDI::Interface> m_midi;
#endif
};
