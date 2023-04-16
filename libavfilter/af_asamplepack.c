/*
 * Copyright (c) 2012 Andrey Utkin
 * Copyright (c) 2012 Stefano Sabatini
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * Filter that changes number of samples on single output operation
 */

#include "audio.h"
#include "avfilter.h"
#include "filters.h"
#include "float.h"
#include "formats.h"
#include "internal.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"

typedef struct ApackCtx {
  const AVClass *class;
  int nbOutSamples; ///< how many samples to output
  uint64_t sampleSum;
  AVAudioFifo *audioFifo;
  int frameFormat;
  int nbChannels;
  int sampleRate;
  uint64_t channelLayout;
  AVRational timeBase;
  uint64_t pts;
  uint64_t ptsOffset;
} ApackCtx;

#define OFFSET(x) offsetof(ApackCtx, x)
#define FLAGS AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM

static const AVOption asamplepack_options[] = {
    {"n", "set the number of per-frame output samples", OFFSET(nbOutSamples), AV_OPT_TYPE_INT, {.i64 = 1764}, 1, INT_MAX, FLAGS},
    {"time_base", NULL, OFFSET(timeBase), AV_OPT_TYPE_RATIONAL, {.dbl = 0}, 0, DBL_MAX, FLAGS},
    {NULL}};

AVFILTER_DEFINE_CLASS(asamplepack);

static int filter_frame(AVFilterLink *inlink, AVFrame *inFrame) {
  int ret;
  AVFilterContext *ctx = inlink->dst;
  ApackCtx *s = ctx->priv;
  AVFilterLink *const outlink = ctx->outputs[0];

  if (!s->audioFifo) {
    s->audioFifo = av_audio_fifo_alloc(inFrame->format,
                                       inFrame->ch_layout.nb_channels,
                                       4 * s->nbOutSamples);

    s->nbChannels = inFrame->ch_layout.nb_channels;
    s->frameFormat = inFrame->format;
    s->sampleRate = inFrame->sample_rate;
    s->channelLayout = inFrame->channel_layout;
    s->ptsOffset = inFrame->pts;
  }

  ret = av_audio_fifo_write(s->audioFifo, inFrame->data, inFrame->nb_samples);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "asamplepack::could not write to fifo\n");
    return ret;
  }

  if (av_audio_fifo_size(s->audioFifo) >= s->nbOutSamples) {
    // we have enough samples in the fifo to create our new output frame
    AVFrame *outFrame = av_frame_alloc();
    outFrame->format = s->frameFormat;
    outFrame->channels = s->nbChannels;
    outFrame->sample_rate = s->sampleRate;
    outFrame->nb_samples = s->nbOutSamples;
    outFrame->time_base = s->timeBase;
    outFrame->channel_layout = s->channelLayout;

    av_frame_get_buffer(outFrame, 0);
    ret = av_audio_fifo_read(s->audioFifo, (void **)outFrame->data, s->nbOutSamples);
    if (ret < 0) {
      av_frame_unref(outFrame);
      return ret;
    }

    s->pts = s->sampleSum * s->timeBase.den / (s->timeBase.num * s->sampleRate);
    outFrame->pts = s->pts + s->ptsOffset;
    outFrame->pkt_dts = outFrame->pts;
    outFrame->best_effort_timestamp = outFrame->pts;
    outFrame->duration = s->nbOutSamples * s->timeBase.den / (s->timeBase.num * s->sampleRate);

    s->sampleSum += s->nbOutSamples;
    ret = ff_filter_frame(outlink, outFrame);
  }

  av_frame_unref(inFrame);
  return ret;
}

/**
 * @brief request_frame
 *
 * the downstream filter wants some data from us.
 * We need to get the next frame from our upstream
 * filter which will trigger filter_frame to be called.
 * We do not need to do any processing here, just fetching the next frame
 *
 * @param outlink
 * @return int
 */
static int request_frame(AVFilterLink *outlink) {
  // Request the next frame from the upstream filter
  int ret = ff_request_frame(outlink);
  return ret;
}

static av_cold int preinit(AVFilterContext *ctx) {
  ApackCtx *s = ctx->priv;
  s->sampleSum = 0;
  s->audioFifo = NULL;

  return 0;
}

static av_cold void uninit(AVFilterContext *ctx) {
}

static const AVFilterPad asamplepack_inputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_AUDIO,
        .filter_frame = filter_frame,
    },
};

static const AVFilterPad asamplepack_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_AUDIO,
        .request_frame = request_frame,
    },
};

const AVFilter ff_af_asamplepack = {
    .name = "asamplepack",
    .description = NULL_IF_CONFIG_SMALL("Pack given number of samples into a new frame and auto adjust PTS"),
    .preinit = preinit,
    .uninit = uninit,
    .priv_size = sizeof(ApackCtx),
    .priv_class = &asamplepack_class,
    FILTER_INPUTS(asamplepack_inputs),
    FILTER_OUTPUTS(asamplepack_outputs),
};
