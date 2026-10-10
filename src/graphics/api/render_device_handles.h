#pragma once

#include "core/utils/handles.h"

/* tags */
struct SamplerTag;
struct PipelineTag;
struct BufferTag;
struct MatrixTag;
struct RenderTargetTag;

/* handle aliases */
using SamplerHandle = Handle<SamplerTag>;
using PipelineHandle = Handle<PipelineTag>;
using BufferHandle = Handle<BufferTag>;
using MatrixHandle = Handle<MatrixTag>;
using RenderTargetHandle = Handle<RenderTargetTag>;
