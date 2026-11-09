# Tasks

## 1. Контракты окна, событий и ввода

- [x] 1.1 Создать типы событий ввода `input_events.h` (KeyCode, MouseButton, InputEvent variant) и структуру `InputState` в `adapters/window/include/huynity/adapters/window/`; проверить модульными тестами корректность обновления состояния при нажатии/отпускании клавиш и кнопок мыши.
- [x] 1.2 Создать базовый интерфейс `WindowAdapter` в `include/huynity/adapters/window/window_adapter.h`, наследующий `runtime::PresentationPort`; проверить compile/API-тестами отсутствие зависимостей от Core и Renderer (зависимость только от Image и Runtime).

## 2. Реализация оконных бэкендов

- [x] 2.1 Реализовать детерминированный `HeadlessWindow` в `adapters/window/` с хранением состояния в памяти, программной генерацией событий ввода и синхронным `present()`; проверить тестами возврат буфера, статус `isOpen` и обработку несовпадения габаритов буфера.
- [x] 2.2 Настроить `adapters/window/CMakeLists.txt` с созданием целей `huynity_window` / `huynity::window` и опциональным поиском SDL3/SDL2 (`find_package` QUIET) для `NativeWindow` с прозрачным fallback на `HeadlessWindow`; проверить сборку при включённых и выключённых адаптерах.

## 3. Интеграция с главным циклом и верификация

- [x] 3.1 Реализовать интеграционный тест `adapters/window/tests/window_integration_tests.cpp`: проверить связку `WindowAdapter` с `runtime::FramePipeline` и `runtime::MainLoop`, смену поколений поверхности при resize и сброс stale-снимков.
- [x] 3.2 Создать пример `examples/window_demo/` (или автономный интерактивный таргет), открывающий окно с отображением анимированной сцены `SoftwareRenderer`; задокументировать оконный адаптер в `docs/window_adapter.md`.
- [x] 3.3 Выполнить форматирование изменённых файлов через clang-format 20, полную сборку в `$HUYNITY_BUILD_DIR`, CTest всего дерева и строгую валидацию `openspec validate window-and-input-adapter --strict`.
