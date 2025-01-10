// https://gitlab.freedesktop.org/pipewire/pipewire/-/blob/master/src/examples/midi-src.c?ref_type=heads
// https://gitlab.freedesktop.org/pipewire/pipewire/-/blob/master/src/tools/pw-mididump.c?ref_type=heads

#include <functional>
#include <stdexcept>

#include <pipewire/filter.h>
#include <pipewire/pipewire.h>

#include <spa/control/control.h>
#include <spa/pod/builder.h>


namespace PipeWire
{


class ScopeGuard
{
public:
	ScopeGuard(std::function<void(void)>&& fn)
		: m_cb(std::move(fn))
	{
	}

	~ScopeGuard()
	{
		m_cb();
	}

private:
	std::function<void(void)> m_cb;
};


class MooerMidiControl
{
public:
	MooerMidiControl();

	~MooerMidiControl();

	void OnProcess(struct spa_io_position* position);

	void OnSignal(int signal_number);

	static void on_process(void* userdata, struct spa_io_position* position);

private:
	static void do_quit(void* userdata, int signal_number);


	struct pw_main_loop* m_loop;
	struct pw_filter* m_filter;
	struct port* m_InputPort;
};


const char* g_opt_remote = "mooer_remote_name";


static const struct pw_filter_events g_filter_events = {
	.version = PW_VERSION_FILTER_EVENTS,
	.process = MooerMidiControl::on_process,
};


MooerMidiControl::MooerMidiControl()
{
	int argc = 0;
	char** argv = nullptr;
	pw_init(&argc, &argv);

	m_loop = pw_main_loop_new(NULL);
	if(m_loop == nullptr)
		throw std::runtime_error("Could not create Pulsewire loop");

	// pw_loop_add_signal(pw_main_loop_get_loop(m_loop), SIGINT, do_quit, data);
	// pw_loop_add_signal(pw_main_loop_get_loop(m_loop), SIGTERM, do_quit, data);

	m_filter = pw_filter_new_simple(pw_main_loop_get_loop(m_loop),
									"mooer-midi",
									pw_properties_new(PW_KEY_REMOTE_NAME,
													  g_opt_remote,
													  PW_KEY_MEDIA_TYPE,
													  "Midi",
													  PW_KEY_MEDIA_CATEGORY,
													  "Filter",
													  PW_KEY_MEDIA_ROLE,
													  "DSP",
													  NULL),
									&g_filter_events,
									this);
	/*
		m_InputPort =
			pw_filter_add_port(m_filter,
							   PW_DIRECTION_INPUT,
							   PW_FILTER_PORT_FLAG_MAP_BUFFERS,
							   sizeof(struct port),
							   pw_properties_new(PW_KEY_FORMAT_DSP, "32 bit raw UMP", PW_KEY_PORT_NAME, "input", NULL),
							   NULL,
							   0);

		if(pw_filter_connect(data->filter, PW_FILTER_FLAG_RT_PROCESS, NULL, 0) < 0)
			throw std::runtime_error("Can connect Pipewire filter");
	*/
}


MooerMidiControl::~MooerMidiControl()
{
	pw_filter_destroy(m_filter);
	pw_main_loop_destroy(m_loop);
	pw_deinit();
}


void MooerMidiControl::on_process(void* userdata, struct spa_io_position* position)
{
	return static_cast<MooerMidiControl*>(userdata)->OnProcess(position);
}


void MooerMidiControl::OnProcess(struct spa_io_position* position)
{
	struct pw_buffer* b = pw_filter_dequeue_buffer(m_InputPort);
	struct spa_buffer* buf = b->buffer;
	struct spa_data* d = &buf->datas[0];

	ScopeGuard sg([this, b](void) { pw_filter_queue_buffer(m_InputPort, b); });

	if(d->data == NULL)
		return;

	struct spa_pod* pod;
	if((pod = (spa_pod*)spa_pod_from_data(d->data, d->maxsize, d->chunk->offset, d->chunk->size)) == NULL)
		return;
	if(!spa_pod_is_sequence(pod))
		return;

	spa_pod_control* c;
	SPA_POD_SEQUENCE_FOREACH((struct spa_pod_sequence*)pod, c)
	{
#if 0
		struct midi_event ev;

		if(c->type != SPA_CONTROL_UMP)
			continue;

		ev.track = 0;
		ev.sec = (frame + c->offset) / (float)position->clock.rate.denom;
		ev.data = SPA_POD_BODY(&c->value), ev.size = SPA_POD_BODY_SIZE(&c->value);
		ev.type = MIDI_EVENT_TYPE_UMP;

		// ToDo: Convert MIDI CC commands to MOOER patch changes

		// fprintf(stdout, "%4d: ", c->offset);
		// midi_file_dump_event(stdout, &ev);
#endif
	}
}


void MooerMidiControl::do_quit(void* userdata, int signal_number)
{
	return static_cast<MooerMidiControl*>(userdata)->OnSignal(signal_number);
}


void MooerMidiControl::OnSignal(int signal_number)
{
	pw_main_loop_quit(m_loop);
}


} // namespace PipeWire
