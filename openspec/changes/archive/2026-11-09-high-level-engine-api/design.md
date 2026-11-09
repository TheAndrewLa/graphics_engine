# Design

## Context

Движок Huynity содержит полный набор низкоуровневых подсистем: `engine/core` (сцена, иерархия, команды, Render Preparation), `engine/runtime` (FramePipeline 1+1, FixedStepUpdate, MainLoop), `engine/renderer` (SoftwareRenderer), `adapters/window` (WindowAdapter, InputEvents, InputState) и `engine/objectlib` (геометрия). Однако создание даже простейшего интерактивного приложения требует от разработчика ручной сборки и синхронизации десятков классов. Нужен высокоуровневый эргономичный фасад `Engine`.

## Goals / Non-Goals

**Goals:**
- Предоставить разработчику игры единую точку входа `huynity::Engine` в `<huynity/engine.h>`.
- Поддержать игровой цикл с фиксированным шагом через коллбек `onUpdate(double dt)`.
- Обеспечить доступ к актуальному состоянию ввода через `engine.input().isKeyPressed(...)`.
- Реализовать быстрое создание 2D и 3D сущностей в активной сцене в 1 строку: `engine.create<ObjectType::...>(...)`.
- Реализовать режим `Engine::preview(geometry)` для мгновенного отображения любого геометрического объекта без предварительной ручной настройки сцены.
- Обеспечить создание базовой игры «Hello, Cube» менее чем за 20 строк C++23 кода.

**Non-Goals:**
- Полнофункциональный визуальный редактор (это задача `apps/editor`).
- Симуляция твёрдых тел и физических столкновений (rigid body physics).
- Звуковая подсистема.

## Decisions

### 1. Архитектура класса `huynity::Engine`

Класс `Engine` инкапсулирует подсистемы и управляет координацией кадра:

```cpp
namespace huynity {

struct EngineConfig {
    adapters::window::WindowSize windowSize{800, 600};
    std::string windowTitle{"Huynity Game"};
    double fixedTimeStep{1.0 / 60.0};
    bool forceHeadless{false};
};

class Engine {
public:
    explicit Engine(const EngineConfig& config = {});

    // Настройка сцены и камеры
    [[nodiscard]] core::Scene& scene() noexcept;
    void setCamera(render_contracts::Camera3 camera);
    void setCamera(render_contracts::Camera2 camera);
    void setLighting(render_contracts::Lighting3 lighting);

    // Удобное создание объектов
    template <objectlib::ObjectType kind>
    core::ObjectId create(const objectlib::TypedParams<kind>& params,
                          core::Mobility mobility = core::Mobility::Static);
    core::ObjectId create(const objectlib::ObjectSpec& spec,
                          core::Mobility mobility = core::Mobility::Static);

    // Обратные вызовы игры
    void onUpdate(std::function<void(double fixedDt)> callback);
    void onFrame(std::function<void(double alpha)> callback);
    void onEvent(std::function<void(const adapters::window::InputEvent&)> callback);

    // Состояние ввода
    [[nodiscard]] const adapters::window::InputState& input() const noexcept;

    // Жизненный цикл
    void run();
    void step(double dt); // для детерминированного тестирования
    void stop() noexcept;

    // Режим быстрого превью (статический и метод)
    static void preview(const objectlib::Geometry& geometry, const EngineConfig& config = {});
    void show(core::ObjectId id);

private:
    EngineConfig m_config;
    core::Scene m_scene;
    std::unique_ptr<adapters::window::WindowAdapter> m_window;
    runtime::FramePipeline m_pipeline;
    runtime::FixedStepUpdate m_fixedUpdate;
    renderer::SoftwareRenderer m_renderer;
    adapters::window::InputState m_inputState;
    std::optional<render_contracts::Camera3> m_camera3;
    std::optional<render_contracts::Camera2> m_camera2;
    render_contracts::Lighting3 m_lighting;
    bool m_isRunning{false};
    // ... пользовательские коллбеки
};

}
```

*Rationale:* Разработчику больше не нужно знать про `RendererPort`, `CompletionChannel` или `FramePipeline` — фасад выполняет всю оркестрацию внутри себя.

### 2. Реализация режима `preview(geometry)`

При вызове `Engine::preview(geometry)`:
1. Вычисляются габариты через `objectlib::bounds(geometry)`.
2. Если геометрия трёхмерная (`Aabb3`):
   - Добавляется рассеянный свет (0.3) и направленный свет под 45 градусов (0.7).
   - Вычисляется максимальный размер габарита $D = \max(\text{size}_x, \text{size}_y, \text{size}_z)$.
   - Ортографическая или перспективная камера автоматически отдаляется на дистанцию $Z = \text{center}_z + D \cdot 1.8$, обеспечивая 100% видимость объекта в кадре.
3. Если геометрия двумерная (`Aabb2`):
   - Настраивается камера `Camera2` с шириной $W = \text{size}_x \cdot 1.2$ и высотой $H = \text{size}_y \cdot 1.2$.
4. Запускается демонстрационный цикл окна.

## Risks / Trade-offs

- **[Фасад скрывает низкоуровневые настройки]** → `engine.scene()` и методы `setCamera`/`setLighting` дают полный доступ к исходным низкоуровневым объектам для продвинутых задач.
- **[std::function в коллбеках создаёт микро-оверхед]** → Вызов происходит 1 раз за тик (60 Гц), накладные расходы составляют единицы наносекунд и пренебрежимо малы по сравнению с рендерингом кадра.
