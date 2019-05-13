#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pulse/pulseaudio.h>

typedef struct UserData {
	pa_mainloop* mainloop;
	pa_context* pactx;
	int mute;
	pa_cvolume cvol;
	char default_sink_name[256];
} UserData;

void cb_sink_info(pa_context* pactx, const pa_sink_info* i, int eol, void* userdata)
{
	UserData* ud = (UserData*)userdata;

	if (i) {
		ud->mute = i->mute;
		ud->cvol = i->volume;
	}
}

void cb_server_info(pa_context* pactx, const pa_server_info* inf, void* userdata)
{
	UserData* ud = (UserData*)userdata;

	strcpy(ud->default_sink_name, inf->default_sink_name);
	pa_context_get_sink_info_by_name(pactx, ud->default_sink_name, cb_sink_info, ud);
}

void cb_state(pa_context* pactx, void* userdata)
{
	if (pa_context_get_state(pactx) == PA_CONTEXT_READY) {
		pa_context_get_server_info(pactx, cb_server_info, userdata);
		pa_context_subscribe(pactx, PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SERVER, NULL, NULL);
	}
}

void cb_subscription(pa_context* pactx, pa_subscription_event_type_t t, unsigned int idx, void* userdata)
{
	UserData* ud = (UserData*)userdata;

	if (t & PA_SUBSCRIPTION_EVENT_SINK_INPUT) {
		pa_context_get_server_info(pactx, cb_server_info, userdata);
	} else {
		pa_context_get_sink_info_by_name(pactx, ud->default_sink_name, cb_sink_info, userdata);
	}
}

void handle_stdin(pa_mainloop_api* ea, pa_io_event* e, int fd, pa_io_event_flags_t events, void* userdata)
{
	UserData* ud = (UserData*)userdata;

	char buf[2];
	int ret = read(0, buf, sizeof buf);

	if (ret != sizeof buf) {
		pa_mainloop_quit(ud->mainloop, 1);
		return;
	}

	switch (buf[0]) {
		case '!':
			pa_context_set_sink_mute_by_name(ud->pactx, ud->default_sink_name, ud->mute = !ud->mute, NULL, NULL);
			break;
		case '[':
		case '<':
			pa_cvolume_dec(&ud->cvol, 0x10000U / 20);
			pa_context_set_sink_volume_by_name(ud->pactx, ud->default_sink_name, &ud->cvol, NULL, NULL);
			break;
		case ']':
		case '>':
			pa_cvolume_inc_clamp(&ud->cvol, 0x10000U / 20, (buf[0] == '>') ? PA_VOLUME_MAX : PA_VOLUME_NORM);
			pa_context_set_sink_volume_by_name(ud->pactx, ud->default_sink_name, &ud->cvol, NULL, NULL);
			break;
	}
}

int main()
{
	UserData ud = {0};

	ud.mainloop = pa_mainloop_new();
	pa_mainloop_api* api = pa_mainloop_get_api(ud.mainloop);
	ud.pactx = pa_context_new(api, "pavolcli");
	pa_context_set_state_callback(ud.pactx, cb_state, &ud);
	pa_context_set_subscribe_callback(ud.pactx, cb_subscription, &ud);
	pa_context_connect(ud.pactx, NULL, PA_CONTEXT_NOAUTOSPAWN | PA_CONTEXT_NOFAIL, NULL);

	api->io_new(api, 0, PA_IO_EVENT_INPUT, handle_stdin, &ud);

	int retval;
	pa_mainloop_run(ud.mainloop, &retval);
	return retval;
}
