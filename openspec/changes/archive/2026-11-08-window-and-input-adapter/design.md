# Design

## Context

Движок Huynity располагает реализованным `Runtime`, в котором определены интерфейс `runtime::PresentationPort` (`present(unique_ptr<ImageBuffer>)`) и конвейер `FramePipeline` с монотонными поколениями поверхности `SurfaceState`. Компонент `engine/image` предоставляет формат кадра `ImageBuffer` (RGBA8, 4 байта на пиксель, stride). Не хватает физического адаптера `adapters/window`, который бы открывал окно ОС, опрашивал события ввода и выводил пиксели.

## Goals / Non-Goals

**Goals:**
- Реализовать чистый оконный адаптер `adapters/window/` без зависимостей от Core, Renderer или Math.
- Реализовать интерфейс `runtime::PresentationPort` для вывода `ImageBuffer` на экран окна.
- Предоставить детерминированный `HeadlessWindow` для тестирования в CI без графического дисплея.
- Поддержать опциональный нативный backend (SDL3/SDL2 при наличии на машине) для реального вывода окна.
- Реализовать сбор и трансляцию платформенных событий ввода в типизированные `InputEvent` и синхронный `InputState`.

**Non-Goals:**
- Создание сложных GUI-виджетов (кнопки, текстовые поля) — это задача будущего слоя редактора/UI.
- Аппаратное ускорение через OpenGL/Vulkan/DirectX — вывод строго через blit готового софтверного буфера пикселей.
- Поддержка геймпадов и джойстиков на этом этапе (только клавиатура, мышь и окно).

## Decisions

### 1. Архитектура оконного адаптера и PresentationPort

Адаптер окна `WindowAdapter` наследует `runtime::PresentationPort` и объявляет интерфейс управления окном и событиями:

```cpp
namespace huynity::adapters::window {

class WindowAdapter : public runtime::PresentationPort {
public:
    virtual ~WindowAdapter() = default;

    [[nodiscard]] virtual bool isOpen() const noexcept = 0;
    virtual void close() noexcept = 0;
    [[nodiscard]] virtual WindowSize size() const noexcept = 0;

    virtual void pollEvents(std::vector<InputEvent>& outEvents) = 0;

    // runtime::PresentationPort
    [[nodiscard]] std::unique_ptr<image::ImageBuffer>
    present(std::unique_ptr<image::ImageBuffer> framebuffer) override = 0;
};

}
```

*Rationale:* Синхронный возврат `unique_ptr<ImageBuffer>` позволяет Runtime немедленно возвратить буфер в `FramebufferPool` без аллокаций.

*Альтернативы:* Асинхронный present с очередью буферов. Отклонено: для софтверного блита в окно достаточно прямого копирования пикселей в экранный контекст.

### 2. Разделение Headless и Native бэкендов

Для обеспечения стабильности сборки на любых машинах и в CI адаптер делится на:
- **`HeadlessWindow`**: всегда доступен, хранит состояние в памяти, симулирует показ буфера и генерацию событий. Используется для тестов и серверов.
- **`NativeWindow`**: использует SDL3/SDL2 (или платформенное окно), если библиотека найдена CMake (`find_package(SDL3 QUIET)` / `find_package(SDL2 QUIET)`).

*Rationale:* Исключает поломку сборки при отсутствии графических библиотек на машине разработчика или в контейнере CI.

### 3. Модель событий и снимка ввода

События ввода разделяются на поток дискретных событий (`InputEvent`) и агрегированное состояние (`InputState`):

```cpp
struct InputState {
    std::array<bool, 512> keys{};
    std::array<bool, 8> mouseButtons{};
    double mouseX{};
    double mouseY{};
    double mouseDeltaX{};
    double mouseDeltaY{};
    double scrollDelta{};

    [[nodiscard]] bool isKeyPressed(KeyCode key) const noexcept;
    [[nodiscard]] bool isMouseButtonPressed(MouseButton button) const noexcept;
};
```

*Rationale:* Игровым механикам перемещения (WASD, вращение камеры мышью) требуется мгновенный опрос зажатых клавиш в `onUpdate(dt)`, тогда как действиям вроде прыжка или открытия меню удобнее подписываться на дискретные события `onEvent`.

## Risks / Trade-offs

- **[Blit ImageBuffer в окно создаёт задержку копирования на CPU]** → Буфер уже находится в памяти в формате RGBA8, блит сводится к одному вызову `SDL_UpdateTexture` или `XPutImage` за кадр (менее 0.5 мс для 1080p).
- **[Resize окна приводит к показу растянутого кадра прежнего размера]** → Интеграция с монотонным `surfaceGeneration` в `FramePipeline` отбрасывает снимок старого разрешения без отображения.
- **[Отсутствие дисплея в GitHub Actions CI]** → Тестовый набор по умолчанию использует `HeadlessWindow`, реальное окно запускается только в интерактивных примерах при наличии дисплея.
