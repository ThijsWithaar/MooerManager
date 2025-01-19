#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <span>
#include <thread>

#define NOMINMAX
#include <libusb.h>


namespace USB
{

class ConnectionListener
{
public:
	virtual ~ConnectionListener() = default;

	virtual void OnUsbConnection() = 0;
};

class TransferListener
{
public:
	TransferListener(int nWritePackets = 10);

	virtual ~TransferListener();

	TransferListener(const TransferListener&) = delete;
	TransferListener& operator=(const TransferListener&) = delete;

	/// Return true to continue listening
	virtual bool OnUsbInterruptData(std::span<std::uint8_t> data) = 0;

	void Connect(libusb_device_handle* device, unsigned char endpoint);

private:
	static void data_transfer_cb(libusb_transfer* tf);

	std::array<std::uint8_t, 64> m_buffer;
	libusb_transfer *m_read_transfer, *m_bulk_write_transfer;
	bool m_continue;
};

class Connection
{
public:
	struct DeviceId
	{
		int vendor, product;

		auto operator<=>(const DeviceId&) const = default;
	};

	Connection(DeviceId did);

	~Connection();

	void StartEventLoop();

	std::span<std::uint8_t> interrupt_transfer(unsigned char endpoint, std::span<std::uint8_t> data);

	void control_transfer(
		uint8_t request_type, uint8_t request, int16_t wValue, uint16_t wIndex, std::span<const std::uint8_t> data);

	void bulk_transfer(unsigned char endpoint, std::span<const std::uint8_t> data);

	void set_interface(int interface_number, int alternate_setting);

	void Connect(TransferListener* listener, unsigned char endpoint);

private:
	static int hotplug_cb(libusb_context* ctx, libusb_device* device, libusb_hotplug_event event, void* user_data);

	static void data_transfer_cb(libusb_transfer* tf);

	void RunEventLoop();

	std::jthread m_event_worker;
	libusb_context* m_ctx;
	libusb_hotplug_callback_handle m_hotplug_handle;
	libusb_device_handle* m_device;
};

} // namespace USB
