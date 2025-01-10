#pragma once

#include <mutex>

#include <QMainWindow>

#include <MooerParser.h>
#include <UsbConnection.h>

#include "ui_MooerManager.h"


class MooerManager : public QMainWindow, public USB::ConnectionListener, public Mooer::Listener
{
	Q_OBJECT
public:
	MooerManager(QWidget* parent);

signals:
	void MooerIdentity(QString version, QString name);
	void MooerPatchSetting(int idx); ///< Received the settings for this patch
	void MooerPatchChange(int idx);	 ///< Index of the active patch
	void MooerSettingsChanged(Mooer::RxFrame::Group group);

private:
	void SwitchMenuIfDifferent(int);

	// USB::ConnectionListener
	void OnUsbConnection() override;

	// Mooer::Listener
	void OnMooerFrame(const Mooer::RxFrame::Frame& frame) override;
	void OnMooerIdentify(const Mooer::Listener::Identity& id) override;

	// GUI setup
	void ConnectFX();
	void ConnectDistortion();
	void ConnectAmplifier();
	void ConnectCabinet();
	void ConnectNoiseGate();

	// GUI slots
	void OnAmpLoad();
	void OnCabinetLoad();
	void UpdatePatchDropdown();
	void UpdateSettingsView(Mooer::RxFrame::Group group);

	// GUI status
	Ui::MainWindow m_ui;
	USB::Connection m_usb;
	Mooer::Parser m_mooer;

	// Device status
	std::mutex m_dev_mutex;
	Mooer::Listener::Identity m_device_id;
	int m_patch_id;
	std::vector<Mooer::Patch> m_patches;
	Mooer::DeviceFormat::AmpModelNames m_ampModelNames;
	// Device status, now complete
	Mooer::DeviceFormat::State m_mstate;
};
