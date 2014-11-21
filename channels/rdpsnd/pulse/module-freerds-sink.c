/**
 * FreeRDS: FreeRDP Remote Desktop Services (RDS)
 * PulseAudio Sink
 *
 * Copyright 2014 Dell Software <Mike.McDonald@software.dell.com>
 * Copyright 2013 Jay Sorg
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <poll.h>

#include <pulse/rtclock.h>
#include <pulse/timeval.h>
#include <pulse/xmalloc.h>
#include <pulse/i18n.h>

#include <pulsecore/core-error.h>
#include <pulsecore/sink.h>
#include <pulsecore/module.h>
#include <pulsecore/core-util.h>
#include <pulsecore/modargs.h>
#include <pulsecore/log.h>
#include <pulsecore/thread.h>
#include <pulsecore/thread-mq.h>
#include <pulsecore/rtpoll.h>

#include "module-freerds-sink-symdef.h"

#include "rdpsnd_plugin_api.h"

PA_MODULE_AUTHOR("Dell Software");
PA_MODULE_DESCRIPTION("FreeRDS Sink");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(FALSE);
PA_MODULE_USAGE(
	"sink_name=<name for the sink> "
	"sink_properties=<properties for the sink> "
	"format=<sample format> "
	"rate=<sample rate>"
	"channels=<number of channels> "
	"channel_map=<channel map>");

#define DEFAULT_SINK_NAME "freerds-sink"
#define BLOCK_USEC 30000
//#define BLOCK_USEC (PA_USEC_PER_SEC * 2)

struct context {
	pa_core *core;
	pa_module *module;
	pa_sink *sink;

	pa_thread *thread;
	pa_thread_mq thread_mq;
	pa_rtpoll *rtpoll;

	pa_usec_t block_usec;
	pa_usec_t timestamp;
	pa_usec_t failed_connect_time;
	pa_usec_t last_send_time;

	void *api_context;

	int skip_bytes;
	int got_max_latency;
};

static const char* const valid_modargs[] = {
	"sink_name",
	"sink_properties",
	"format",
	"rate",
	"channels",
	"channel_map",
	NULL
};

static void close_send(struct context *context);

static int sink_process_msg(pa_msgobject *o, int code, void *data,
	int64_t offset, pa_memchunk *chunk)
{
	struct context *context = PA_SINK(o)->userdata;
	pa_usec_t now;
	long latency;

	pa_log_debug("sink_process_msg: code %d", code);

	switch (code)
	{

		case PA_SINK_MESSAGE_SET_VOLUME: /* 3 */
			break;

		case PA_SINK_MESSAGE_SET_MUTE: /* 6 */
			break;

		case PA_SINK_MESSAGE_GET_LATENCY: /* 7 */
			now = pa_rtclock_now();
			latency = context->timestamp > now ? context->timestamp - now : 0ULL;
			pa_log_debug("sink_process_msg: latency %ld", latency);
			*((pa_usec_t*) data) = latency;
			return 0;

		case PA_SINK_MESSAGE_GET_REQUESTED_LATENCY: /* 8 */
			break;

		case PA_SINK_MESSAGE_SET_STATE: /* 9 */
			if (PA_PTR_TO_UINT(data) == PA_SINK_RUNNING) /* 0 */
			{
				pa_log("sink_process_msg: running");

				context->timestamp = pa_rtclock_now();
			}
			else
			{
				pa_log("sink_process_msg: not running");
				close_send(context);
			}
			break;

	}

	return pa_sink_process_msg(o, code, data, offset, chunk);
}

static void sink_update_requested_latency_cb(pa_sink *s)
{
	struct context *context;
	size_t nbytes;

	pa_sink_assert_ref(s);
	pa_assert_se(context = s->userdata);

	context->block_usec = BLOCK_USEC;
	//context->block_usec = pa_sink_get_requested_latency_within_thread(s);
	pa_log("1 block_usec %d", context->block_usec);

	context->got_max_latency = 0;
	if (context->block_usec == (pa_usec_t) -1)
	{
		context->block_usec = s->thread_info.max_latency;
		pa_log_debug("2 block_usec %d", context->block_usec);
		context->got_max_latency = 1;
	}

	nbytes = pa_usec_to_bytes(context->block_usec, &s->sample_spec);
	pa_sink_set_max_rewind_within_thread(s, nbytes);
	pa_sink_set_max_request_within_thread(s, nbytes);
}

static void process_rewind(struct context *context, pa_usec_t now)
{
	size_t rewind_nbytes, in_buffer;
	pa_usec_t delay;

	pa_assert(context);

	/* Figure out how much we shall rewind and reset the counter */
	rewind_nbytes = context->sink->thread_info.rewind_nbytes;
	context->sink->thread_info.rewind_nbytes = 0;

	if (rewind_nbytes <= 0)
		goto DO_NOTHING;

	pa_log_debug("Requested to rewind %lu bytes.",
		(unsigned long) rewind_nbytes);

	if (context->timestamp <= now)
		goto DO_NOTHING;

	delay = context->timestamp - now;
	in_buffer = pa_usec_to_bytes(delay, &context->sink->sample_spec);

	if (in_buffer <= 0)
		goto DO_NOTHING;

	if (rewind_nbytes > in_buffer)
		rewind_nbytes = in_buffer;

	pa_sink_process_rewind(context->sink, rewind_nbytes);
	context->timestamp -= pa_bytes_to_usec(rewind_nbytes, &context->sink->sample_spec);
	context->skip_bytes += rewind_nbytes;

	pa_log_debug("Rewound %lu bytes.", (unsigned long) rewind_nbytes);

	return;

DO_NOTHING:
	pa_sink_process_rewind(context->sink, 0);
}

static int data_send(struct context *context, pa_memchunk *chunk)
{
	void *api_context;
	BOOL success;
	BYTE *data;
	int bytes;

	api_context = context->api_context;

	if (api_context == NULL)
	{
		if (context->failed_connect_time != 0)
		{
			if (pa_rtclock_now() - context->failed_connect_time < 1000000)
			{
				return 0;
			}
		}

		/* Establish a connection to the RDPSND plugin. */
		api_context = rdpsnd_plugin_api_new();
		if (api_context && rdpsnd_plugin_api_connect(api_context))
		{
			context->api_context = api_context;
			context->failed_connect_time = 0;
			pa_log("Connected ok");
		}
		else
		{
			rdpsnd_plugin_api_free(api_context);
			context->failed_connect_time = pa_rtclock_now();
			pa_log_debug("Connected failed");
			return 0;
		}
	}

	bytes = chunk->length;
	pa_log_debug("bytes %d", bytes);

	/* from rewind */
	if (context->skip_bytes > 0)
	{
		if (bytes > context->skip_bytes)
		{
			bytes -= context->skip_bytes;
			context->skip_bytes = 0;
		}
		else
		{
			context->skip_bytes -= bytes;
			return bytes;
		}
	}

	/* Play the audio. */
	data = (BYTE *)pa_memblock_acquire(chunk->memblock);
	data += chunk->index;
#if 1
	success = rdpsnd_plugin_api_play_audio(api_context, data, bytes);
#else
	success = TRUE;
#endif
	pa_memblock_release(chunk->memblock);

	pa_log("api_context=%p, data=%p, bytes=%d, success=%d", api_context, data, bytes, success ? 1 : 0);

	if (!success)
	{
		pa_log("data_send: send failed for %d bytes", bytes);
		rdpsnd_plugin_api_free(api_context);
		context->api_context = NULL;
		return 0;
	}

	return bytes;
}

static void close_send(struct context *context)
{
	pa_log("close_send:");

	if (context->api_context)
	{
		rdpsnd_plugin_api_free(context->api_context);

		context->api_context = NULL;
	}
}

static void process_render(struct context *context, pa_usec_t now)
{
	pa_memchunk chunk;
	int request_bytes;

	pa_assert(context);

	if (context->got_max_latency)
	{
		return;
	}

	pa_log_debug("process_render: u->block_usec %d", context->block_usec);
	while (context->timestamp < now + context->block_usec)
	{
		request_bytes = context->sink->thread_info.max_request;
		request_bytes = MIN(request_bytes, 16 * 1024);
		pa_sink_render(context->sink, request_bytes, &chunk);
		pa_log("process_render: %d bytes", chunk.length);
		data_send(context, &chunk);
		pa_memblock_unref(chunk.memblock);
		context->timestamp += pa_bytes_to_usec(chunk.length, &context->sink->sample_spec);
	}
}

static void thread_func(void *userdata)
{
	struct context *context = userdata;
	int ret;
	pa_usec_t now;

	pa_assert(context);

	pa_log_debug("Thread starting up");

	pa_thread_mq_install(&context->thread_mq);

	context->timestamp = pa_rtclock_now();

	for (;;)
	{

		if (context->sink->thread_info.state == PA_SINK_RUNNING)
		{

			now = pa_rtclock_now();

			if (context->sink->thread_info.rewind_requested)
			{
				process_rewind(context, now);
			}

			if (context->timestamp <= now)
			{
				pa_log_debug("thread_func: calling process_render");
				process_render(context, now);
			}

			pa_rtpoll_set_timer_absolute(context->rtpoll, context->timestamp);

		}
		else
		{
			pa_rtpoll_set_timer_disabled(context->rtpoll);
		}

		if ((ret = pa_rtpoll_run(context->rtpoll, TRUE)) < 0)
		{
			goto FAIL;
		}

		if (ret == 0)
		{
			goto DONE;
		}
	}

FAIL:
 	/* If this was no regular exit from the loop we have to continue
	 * processing messages until we received PA_MESSAGE_SHUTDOWN */
	pa_asyncmsgq_post(context->thread_mq.outq, PA_MSGOBJECT(context->core),
		PA_CORE_MESSAGE_UNLOAD_MODULE, context->module, 0, NULL, NULL);
	pa_asyncmsgq_wait_for(context->thread_mq.inq, PA_MESSAGE_SHUTDOWN);

DONE:
	pa_log_debug("Thread shutting down");
}

int pa__init(pa_module *module)
{
	struct context *context = NULL;

	pa_sample_spec ss;
	pa_channel_map cmap;
	pa_modargs *modargs = NULL;
	pa_sink_new_data data;
	size_t nbytes;

	pa_log("Loading module module-freerds-sink.");

	pa_assert(module);

	if (!(modargs = pa_modargs_new(module->argument, valid_modargs)))
	{
		pa_log("Failed to parse module arguments.");
		goto FAIL;
	}

	ss = module->core->default_sample_spec;
	cmap = module->core->default_channel_map;
	if (pa_modargs_get_sample_spec_and_channel_map(modargs,
			&ss, &cmap, PA_CHANNEL_MAP_DEFAULT) < 0)
	{
		pa_log("Invalid sample format specification or channel map");
		goto FAIL;
	}

	context = pa_xnew0(struct context, 1);

	module->userdata = (void *) context;

	context->core = module->core;
	context->module = module;
	context->rtpoll = pa_rtpoll_new();
	pa_thread_mq_init(&context->thread_mq, module->core->mainloop, context->rtpoll);

	pa_sink_new_data_init(&data);
	data.driver = __FILE__;
	data.module = module;
	pa_sink_new_data_set_name(&data,
		pa_modargs_get_value(modargs, "sink_name", DEFAULT_SINK_NAME));
	pa_sink_new_data_set_sample_spec(&data, &ss);
	pa_sink_new_data_set_channel_map(&data, &cmap);
	pa_proplist_sets(data.proplist, PA_PROP_DEVICE_DESCRIPTION, "FreeRDS sink");
	pa_proplist_sets(data.proplist, PA_PROP_DEVICE_CLASS, "abstract");

	if (pa_modargs_get_proplist(modargs, "sink_properties",
			data.proplist, PA_UPDATE_REPLACE) < 0)
	{
		pa_log("Invalid properties");
		pa_sink_new_data_done(&data);
		goto FAIL;
	}

	context->sink = pa_sink_new(module->core, &data,
		PA_SINK_LATENCY | PA_SINK_DYNAMIC_LATENCY);
	pa_sink_new_data_done(&data);

	if (!context->sink)
	{
		pa_log("Failed to create sink object.");
		goto FAIL;
	}

	context->sink->parent.process_msg = sink_process_msg;
	context->sink->update_requested_latency = sink_update_requested_latency_cb;
	context->sink->userdata = context;

	pa_sink_set_asyncmsgq(context->sink, context->thread_mq.inq);
	pa_sink_set_rtpoll(context->sink, context->rtpoll);

	context->block_usec = BLOCK_USEC;
	pa_log_debug("3 block_usec %d", context->block_usec);
	nbytes = pa_usec_to_bytes(context->block_usec, &context->sink->sample_spec);
	pa_sink_set_max_rewind(context->sink, nbytes);
	pa_sink_set_max_request(context->sink, nbytes);

#if defined(PA_CHECK_VERSION)
#if PA_CHECK_VERSION(0, 9, 22)
	if (!(context->thread = pa_thread_new("freerds-sink", thread_func, context)))
#else
	if (!(context->thread = pa_thread_new(thread_func, context)))
#endif
#else
	if (!(context->thread = pa_thread_new(thread_func, context)))
#endif
	{
		pa_log("Failed to create thread.");
		goto FAIL;
	}

	pa_sink_put(context->sink);

	pa_modargs_free(modargs);

	return 0;

FAIL:
	if (modargs)
	{
		pa_modargs_free(modargs);
	}

	pa__done(module);

	return -1;
}

int pa__get_n_used(pa_module *module)
{
	struct context *context;

	pa_assert(module);
	pa_assert_se(context = module->userdata);

	return pa_sink_linked_by(context->sink);
}

void pa__done(pa_module *module)
{
	struct context *context;

	pa_assert(module);

	context = module->userdata;
	if (!context) return;

	if (context->sink)
	{
		pa_sink_unlink(context->sink);
	}

	if (context->thread)
	{
		pa_asyncmsgq_send(context->thread_mq.inq, NULL,
			PA_MESSAGE_SHUTDOWN, NULL, 0, NULL);
		pa_thread_free(context->thread);
	}

	pa_thread_mq_done(&context->thread_mq);

	if (context->sink)
	{
		pa_sink_unref(context->sink);
	}

	if (context->rtpoll)
	{
		pa_rtpoll_free(context->rtpoll);
	}

	pa_xfree(context);
}
