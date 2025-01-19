#include "UsbConnection.h"

#include <array>
#include <cassert>
#include <iostream>
#include <sstream>

// #define DEBUG_LVL_USB
#ifdef DEBUG_LVL_USB
#include <format>
#endif

namespace USB
{

struct CheckedLibUsb
{
	CheckedLibUsb(int rc)
	{
		validate(rc);
	}

	CheckedLibUsb& operator=(int rc)
	{
		validate(rc);
		return *this;
	}

	void validate(int rc)
	{
		if(rc == 0)
			return;
		std::stringstream ss;
		ss << "libusb error " << libusb_strerror(rc);
		throw std::runtime_error(ss.str());
	}
};

class DeviceList
{
public:
	DeviceList(libusb_context* ctx)
	{
		m_size = libusb_get_device_list(ctx, &m_list);
	}

	~DeviceList()
	{
		libusb_free_device_list(m_list, 1);
	}

	libusb_device** begin()
	{
		return m_list;
	}

	libusb_device** end()
	{
		return &m_list[m_size];
	}

	libusb_device** m_list;
	ssize_t m_size;
};


TransferListener::TransferListener(int nWritePackets)
	: m_read_transfer(libusb_alloc_transfer(0))
	, m_bulk_write_transfer(libusb_alloc_transfer(nWritePackets))
	, m_continue(true)
{
}


TransferListener::~TransferListener()
{
	m_continue = false;
	std::cout << "TransferListener::~TransferListener" << std::endl;
	CheckedLibUsb rcw = libusb_cancel_transfer(m_bulk_write_transfer);
	libusb_free_transfer(m_bulk_write_transfer);

	CheckedLibUsb rcr = libusb_cancel_transfer(m_read_transfer);
	libusb_free_transfer(m_read_transfer);
	m_read_transfer = nullptr;
}


void TransferListener::Connect(libusb_device_handle* device, unsigned char endpoint)
{
	const unsigned int timeout = 0;
	libusb_fill_interrupt_transfer(m_read_transfer,
								   device,
								   endpoint,
								   m_buffer.data(),
								   m_buffer.size(),
								   &TransferListener::data_transfer_cb,
								   this,
								   0);
	CheckedLibUsb rc = libusb_submit_transfer(m_read_transfer);
}


void TransferListener::data_transfer_cb(libusb_transfer* tf)
{
	auto self = reinterpret_cast<TransferListener*>(tf->user_data);
	if(tf->status == LIBUSB_TRANSFER_COMPLETED)
		self->OnUsbInterruptData(std::span<std::uint8_t>(self->m_buffer));
	if(!self->m_continue)
		return;
	CheckedLibUsb rc = libusb_submit_transfer(self->m_read_transfer); // Start the next read
}


//-- UsbConnection --


int Connection::hotplug_cb(libusb_context* ctx, libusb_device* device, libusb_hotplug_event event, void* user_data)
{
	auto self = reinterpret_cast<Connection*>(user_data);

	libusb_device* current_dev = (self->m_device != nullptr) ? libusb_get_device(self->m_device) : nullptr;

	if(self->m_device == nullptr && LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event)
	{
		std::cout << " Connection::hotplug_cb: connect\n";
		// int rc = libusb_open(device, &self->m_device);
		// self->m_listener->OnUsbConnection();
	}
	else if(LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event && (current_dev == device))
	{
		std::cout << " UsbConnection::hotplug_cb: DISconnect\n";
		// libusb_close(self->m_device);
		// self->m_device = nullptr;
	}

	return 1; // Finished with this event
}


Connection::Connection(DeviceId did)
	: m_device(nullptr)
{
	const int interface = 3;

#if LIBUSB_API_VERSION <= 0x01000109
	libusb_init(&m_ctx);
#else
	std::array<libusb_init_option, 0> init_options;
	libusb_init_context(&m_ctx, init_options.data(), init_options.size());
#endif

	DeviceList devlist(m_ctx);
	for(libusb_device* dev : devlist)
	{
		uint8_t bus = libusb_get_bus_number(dev);
		uint8_t port = libusb_get_port_number(dev);
		uint8_t addr = libusb_get_device_address(dev);
		libusb_device_descriptor desc;
		libusb_get_device_descriptor(dev, &desc);
		if(did != DeviceId{desc.idVendor, desc.idProduct})
			continue;
#if DEBUG_LVL_USB > 0
		std::cout << std::format(" bpa {}:{}:{}, dcls {:02x}.{:02x}.{:02x}\n",
								 bus,
								 port,
								 addr,
								 desc.bDeviceClass,
								 desc.bDeviceSubClass,
								 desc.bDeviceProtocol);
#endif

		libusb_config_descriptor* conf = nullptr;
		libusb_get_active_config_descriptor(dev, &conf);
#if DEBUG_LVL_USB > 0
		if(conf != nullptr)
			std::cout << std::format(" device, with {} itfs\n", conf->bNumInterfaces);
#endif

		libusb_device_handle* handle;
		if(int rc = libusb_open(dev, &handle))
		{
#if DEBUG_LVL_USB > 0
			std::cout << std::format(" open returns: {}\n", libusb_strerror(rc));
#endif
			continue;
		}

		for(int i = 0; i < conf->bNumInterfaces; i++)
		{
			int rc = libusb_detach_kernel_driver(handle, i);
#if DEBUG_LVL_USB > 0
			std::cout << std::format(" Detaching {} returns {} {}\n", i, rc, libusb_strerror(rc));
#endif
		}
		libusb_close(handle);

		/*libusb_config_descriptor* cfg = 0;
		libusb_get_active_config_descriptor(dev, &cfg);
		const libusb_interface* iface = cfg->interface;
		for(int n = 0; n < iface->num_altsetting; n++)
		{
		}*/
	}

	m_device = libusb_open_device_with_vid_pid(m_ctx, did.vendor, did.product);
	if(m_device == nullptr)
	{
		char err[80];
		printf("UsbConnection: could not open %04x:%04x", did.vendor, did.product);
		throw std::runtime_error(err);
		// throw std::runtime_error(std::format("UsbConnection: could not open {:04x}:{:04x}", did.vendor,
		// did.product));
	}

#if 0
	int events = LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED;
	int flags = 0;
	int dev_class = LIBUSB_HOTPLUG_MATCH_ANY;
	m_hotplug_handle = 0;
	m_hotplug_handle = libusb_hotplug_register_callback(
		m_ctx, events, flags, did.vendor, did.product, dev_class, &UsbConnection::hotplug_cb, this, &m_hotplug_handle);
#endif
}


Connection::~Connection()
{
	std::cout << "USB::Connection::~Connection" << std::endl;

	m_event_worker.request_stop();

	// libusb_hotplug_deregister_callback(m_ctx, m_hotplug_handle);

	// libusb_free_transfer(m_interrupt_read_transfer);

	m_event_worker.join();

	libusb_close(m_device);


	libusb_exit(m_ctx);
}

void Connection::StartEventLoop()
{
	m_event_worker = std::jthread{&Connection::RunEventLoop, this};
}


void Connection::RunEventLoop()
{
	auto st = m_event_worker.get_stop_token();
	int completed = 0;
	while((!st.stop_requested()) && (libusb_event_handling_ok(m_ctx)) && (!completed))
	{
		struct timeval tv{1, 0};
		/*if(libusb_try_lock_events(m_ctx) == 0)
		{
			libusb_handle_events_locked(m_ctx, &tv);
			libusb_unlock_events(m_ctx);
		}*/
		// if(!libusb_event_handling_ok(m_ctx))
		//	libusb_unlock_events(m_ctx);

		CheckedLibUsb rc = libusb_handle_events_timeout_completed(m_ctx, &tv, &completed);
	}
	std::cout << "USB::Connection::RunEventLoop(): done\n";
}


void Connection::data_transfer_cb(libusb_transfer* tf)
{
	auto self = reinterpret_cast<Connection*>(tf->user_data);
}


void Connection::Connect(TransferListener* listener, unsigned char endpoint)
{
	listener->Connect(m_device, endpoint);
}


std::span<std::uint8_t> Connection::interrupt_transfer(unsigned char endpoint, std::span<std::uint8_t> data)
{
	const int timeout_ms = 3 * 1000;
	int dsize = 0;
	assert(m_device != nullptr);
	int rx = libusb_interrupt_transfer(m_device, endpoint, data.data(), data.size(), &dsize, timeout_ms);
	if((timeout_ms == 0) || (rx != LIBUSB_ERROR_TIMEOUT))
	{
		CheckedLibUsb rtx(rx);
	}
	return data.subspan(0, dsize);
}


void Connection::control_transfer(
	uint8_t request_type, uint8_t request, int16_t wValue, uint16_t wIndex, std::span<const std::uint8_t> data)
{
	int timeout_ms = 0;
	auto pData = const_cast<std::uint8_t*>(data.data());
	int rx = libusb_control_transfer(m_device, request_type, request, wValue, wIndex, pData, data.size(), timeout_ms);
	if((timeout_ms == 0) || (rx != LIBUSB_ERROR_TIMEOUT))
	{
		CheckedLibUsb rtx(rx);
	}
}


void Connection::bulk_transfer(unsigned char endpoint, std::span<const std::uint8_t> data)
{
	int timeout_ms = 0;
	int actual_length = 0;
	auto pData = const_cast<std::uint8_t*>(data.data());
	int rx = libusb_bulk_transfer(m_device, endpoint, pData, data.size(), &actual_length, timeout_ms);
	if((timeout_ms == 0) || (rx != LIBUSB_ERROR_TIMEOUT))
	{
		CheckedLibUsb rtx(rx);
	}
}


void Connection::set_interface(int interface_number, int alternate_setting)
{
	CheckedLibUsb rx = libusb_set_interface_alt_setting(m_device, interface_number, alternate_setting);
}



} // namespace USB
