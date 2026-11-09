# Tasks

## 1. Runtime-ограничивающий объём

- [x] 1.1 Добавить BoundsGeometry, RuntimeBoundsResult и объявление bounds(const Geometry&) в operations/bounds.h; реализовать делегирование и обработку valueless в src/operations/runtime_operations.cpp, подключить файл в engine/objectlib/CMakeLists.txt и проверить сборку цели huynity_objectlib.
- [x] 1.2 Добавить в группу objectlib.runtime проверки bounds для поддерживаемых 2D/3D-альтернатив, UnsupportedOperation для Line/Ray/Plane, EmptyGeometry для пустой сетки, InvalidGeometry для NaN Point, сохранение явных координат и независимость результата от входа; проверить `ctest --test-dir "$HUYNITY_BUILD_DIR" -R '^objectlib.runtime$' --output-on-failure`.
- [x] 1.3 Описать runtime-bounds, variant-результат и ошибки в docs/objectlib_usage.md; сверить документ с поведением тестов и проверить относительные ссылки.

## 2. Runtime-тесселяция

- [x] 2.1 Добавить TessellatedGeometry, RuntimeTessellationResult и tessellate(const Geometry&, TessellationOptions) в operations/tessellate.h; реализовать Circle2 через sampleCircle, остальные пять фигур через typed-перегрузки, UnsupportedOperation для остальных типов и InvalidGeometry для valueless; проверить сборку huynity_objectlib.
- [x] 2.2 Добавить в objectlib.runtime сравнение typed/runtime-результатов для Circle2, Disk2, Rectangle2, Sphere3 и Box3, ошибки InvalidOptions/BudgetExceeded/NumericalFailure, UnsupportedOperation для Polygon2 и Point, локальную центрированность и владение результатом после уничтожения входа; проверить `ctest --test-dir "$HUYNITY_BUILD_DIR" -R '^objectlib.(runtime|tessellation|numerics)$' --output-on-failure`.
- [x] 2.3 Дополнить docs/objectlib_usage.md runtime-тесселяцией, различием tessellate и triangulate, unchecked-семантикой Math Point и границей ответственности Core за мировые преобразования; сверить примеры вызовов с публичными заголовками.

## 3. Интеграционная проверка

- [x] 3.1 Отформатировать только изменённые C++-файлы проектным clang-format 20 и проверить их через `clang-format -style=file --dry-run --Werror`, затем проверить `git diff --check`.
- [x] 3.2 Выполнить `cmake --build "$HUYNITY_BUILD_DIR" --parallel`, полный `ctest --test-dir "$HUYNITY_BUILD_DIR" --output-on-failure` и запустить huynity_objectlib_example; зафиксировать фактический каталог сборки и результаты.
- [x] 3.3 Просмотреть итоговый diff на соответствие proposal/spec/design, отсутствие изменений typed API и посторонних файлов; проверить `git status --short` и `openspec validate refine-objectlib-runtime-contracts --strict`.
