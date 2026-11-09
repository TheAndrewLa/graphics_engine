#pragma once

#include "huynity/runtime/fixed_step_update.h"
#include "huynity/runtime/frame_pipeline.h"

#include <optional>
#include <utility>

namespace huynity::runtime {

struct LoopEvents {
    std::optional<SurfaceSize> resize; // последний известный размер поверхности за проход
};

struct LoopResult {
    PipelineCompletion completion;
    UpdateResult update;
    bool prepared{};
    DispatchResult dispatch{DispatchResult::Empty};
};

/// @brief Выполняет один проход главного цикла в зафиксированном порядке фаз:
/// events/resize → completions → commands → bounded update → prepare → dispatch.
///
/// Порядок гарантирует, что resize применяется до обработки FrameReady (кадр прежнего
/// поколения поверхности отбрасывается без показа), а снимок строится только после
/// применения команд и ограниченного числа игровых шагов.
///
/// Перекрытие prepare/render ограничено глубиной 1+1: один исполняемый Renderer кадр
/// и не более одного подготовленного снимка. Подготовка пропускается при нулевой
/// площади поверхности или занятом prepared-слоте; пока слот занят, изменения
/// накапливаются в Core. Освободившемуся Renderer prepared-снимок отправляется
/// в том же проходе заключительной фазой dispatch.
///
/// @param commands фаза применения команд сцены
/// @param updatePhase игровой шаг фиксированной длительности, вызывается ограниченно
/// @param prepare фаза подготовки снимка; помещает его через FramePipeline::storePrepared
///        и возвращает true при успешном сохранении
template <typename CommandsPhase, typename UpdatePhase, typename PreparePhase>
LoopResult runIteration(RendererPort& port, FramePipeline& pipeline, CompletionChannel& channel,
                        FixedStepUpdate& update, const LoopEvents& events, double elapsed, CommandsPhase&& commands,
                        UpdatePhase&& updatePhase, PreparePhase&& prepare) {
    LoopResult result;
    if (events.resize.has_value()) {
        pipeline.resize(*events.resize);
    }
    result.completion = pipeline.complete(channel);
    std::forward<CommandsPhase>(commands)();
    result.update = update.advance(elapsed, std::forward<UpdatePhase>(updatePhase));
    if (pipeline.canPrepare()) {
        result.prepared = static_cast<bool>(std::forward<PreparePhase>(prepare)());
    }
    result.dispatch = pipeline.dispatchPrepared(port);
    return result;
}

} // namespace huynity::runtime
