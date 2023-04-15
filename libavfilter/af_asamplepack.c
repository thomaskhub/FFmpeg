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
#include "formats.h"
#include "internal.h"
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"

typedef struct AmergeCtx {
  const AVClass *class;
  int nb_out_samples; ///< how many samples to output
  int bypass;
} AmergeCtx;

#define OFFSET(x) offsetof(AmergeCtx, x)
#define FLAGS AV_OPT_FLAG_AUDIO_PARAM | AV_OPT_FLAG_FILTERING_PARAM

static const AVOption asamplepack_options[] = {
    {"n", "set the number of per-frame output samples", OFFSET(nb_out_samples), AV_OPT_TYPE_INT, {.i64 = 1764}, 1, INT_MAX, FLAGS},
    {"bypass", "bypass the filter output = input", OFFSET(bypass), AV_OPT_TYPE_INT, {.i64 = 0}, 0, INT_MAX, FLAGS},
    {NULL}};

AVFILTER_DEFINE_CLASS(asamplepack);

static int activate(AVFilterContext *ctx) {
  AVFilterLink *inlink = ctx->inputs[0];
  AVFilterLink *outlink = ctx->outputs[0];
  AmergeCtx *s = ctx->priv;
  AVFrame *frame = NULL, *pad_frame;
  int ret;

  printf("Thomas:: called the asamplepack filter\n");

  // create a simple bypass for testing
  FF_FILTER_FORWARD_STATUS_BACK(outlink, inlink);

  ret = ff_inlink_consume_samples(inlink, s->nb_out_samples, s->nb_out_samples, &frame);
  // printf("Thomas:: called we consumed samples | Frame format = %i | In Link Format = %i | Out Link Format = %i |(%i)\n",
  //        frame->format, inlink->format, outlink->format, ret);

  if (ret < 0)
    return ret;

  printf("Thomas:: bypass is set to  (%i)\n", s->bypass);
  if (s->bypass >= 1) {
    printf("Thomas:: going to bypass the filter\n");
    return ff_filter_frame(outlink, frame);
  }

  // FF_FILTER_FORWARD_STATUS_BACK(outlink, inlink);

  // ret = ff_inlink_consume_samples(inlink, s->nb_out_samples, s->nb_out_samples, &frame);
  // if (ret < 0)
  //   return ret;

  // if (ret > 0) {
  //   if (!s->pad || frame->nb_samples == s->nb_out_samples)
  //     return ff_filter_frame(outlink, frame);

  //   pad_frame = ff_get_audio_buffer(outlink, s->nb_out_samples);
  //   if (!pad_frame) {
  //     av_frame_free(&frame);
  //     return AVERROR(ENOMEM);
  //   }

  //   ret = av_frame_copy_props(pad_frame, frame);
  //   if (ret < 0) {
  //     av_frame_free(&pad_frame);
  //     av_frame_free(&frame);
  //     return ret;
  //   }

  //   av_samples_copy(pad_frame->extended_data, frame->extended_data,
  //                   0, 0, frame->nb_samples, frame->ch_layout.nb_channels, frame->format);
  //   av_samples_set_silence(pad_frame->extended_data, frame->nb_samples,
  //                          s->nb_out_samples - frame->nb_samples, frame->ch_layout.nb_channels,
  //                          frame->format);
  //   av_frame_free(&frame);
  //   return ff_filter_frame(outlink, pad_frame);
  // }

  // FF_FILTER_FORWARD_STATUS(inlink, outlink);
  // if (ff_inlink_queued_samples(inlink) >= s->nb_out_samples) {
  //   ff_filter_set_ready(ctx, 100);
  //   return 0;
  // }
  // FF_FILTER_FORWARD_WANTED(outlink, inlink);

  return FFERROR_NOT_READY;
}

static const AVFilterPad asamplepack_inputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_AUDIO,
    },
};

static const AVFilterPad asamplepack_outputs[] = {
    {
        .name = "default",
        .type = AVMEDIA_TYPE_AUDIO,
    },
};

const AVFilter ff_af_asamplepack = {
    .name = "asamplepack",
    .description = NULL_IF_CONFIG_SMALL("Create output frames with a fixed size of 1764 samples"),
    .priv_size = sizeof(AmergeCtx),
    .priv_class = &asamplepack_class,
    FILTER_INPUTS(asamplepack_inputs),
    FILTER_OUTPUTS(asamplepack_outputs),
    .activate = activate,
};
