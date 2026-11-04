# Object Library: реализованный API

Реализована базовая часть [UML-модели](objectlib_class_diagram.md): геометрические
значения, параметры создания, обе фабрики и операции. Код находится в
`engine/objectlib`, зависит только от `engine/math` и стандартной библиотеки.
Цели CMake: `huynity_objectlib` и ALIAS `huynity::objectlib`.

## Создание

```cpp
#include "huynity/objectlib/create_object.h"
#include "huynity/objectlib/operations/tessellate.h"

namespace objects = huynity::objectlib;

auto sphere = objects::createObject<objects::ObjectType::Sphere3>(
    objects::Sphere3Params{.radius = 2.0}
);
if (!sphere) {
    // sphere.error().code — ошибка входной геометрии.
} else {
    auto mesh = objects::tessellate(*sphere, {
        .maxDeviation = 0.05,
        .maxPrimitiveCount = 4096
    });
    // mesh: expected<TriangleMesh3, GeometryError>.
}

auto geometry = objects::createObject(
    objects::ObjectSpec{objects::Sphere3Params{.radius = 2.0}}
);
// expected<Geometry, CreateError>; при успехе variant содержит Sphere3.
```

Params принимаются по значению. Для большого массива можно передать
`std::move(params)`; созданный объект владеет данными независимо от Params.
Поля геометрических классов закрыты; массивы читаются через `span<const T>`.
View действует только пока жив и стабилен владелец. После перемещения исходное
значение можно уничтожить или присвоить заново; его прежнюю геометрию и views
использовать нельзя. Копирование объектов даёт независимые значения.

## Каталог

| Теги ObjectType | Поля Params | Результат |
|---|---|---|
| Point2, Point3 | coordinates | Проверенные значения Math Point2 / Point3 |
| Segment2, Segment3 | a, b | Отрезок; совпадающие концы допустимы |
| Line2, Line3 | origin, direction | Бесконечная прямая |
| Ray2, Ray3 | origin, direction | Луч |
| Triangle2, Triangle3 | vertices: array из трёх точек | Невырожденный треугольник |
| Polyline2, Polyline3 | vertices, closed | Полилиния |
| Aabb2, Aabb3 | min, max | Ограничивающий объём |
| TriangleMesh2, TriangleMesh3 | positions, triangles | Треугольная сетка |
| Plane3 | normal, offset | Плоскость n·x + offset = 0 |
| Polygon2 | contour | Простой контур без отверстий |
| Rectangle2 | size: Vector2 | Прямоугольник |
| Circle2, Disk2, Sphere3 | radius | Окружность, диск, сфера |
| Box3 | size: Vector3 | Параллелепипед |

Всего 23 тега. Имена параметров получаются добавлением Params, например
TriangleMesh3Params. Фигуры с радиусом и размерами центрированы в локальном начале
координат. Их размещение в сцене относится к Core. Направления и нормали не
нормализуются автоматически. Point и Vector — изменяемые агрегаты Math;
последующее прямое изменение их координат не сохраняет гарантию конечности.

## Операции

| Заголовок | Операция | Результат |
|---|---|---|
| operations/bounds.h | bounds для конечной геометрии | expected<Aabb<N>, GeometryError> |
| operations/bounds.h | bounds(Geometry) | expected<variant<Aabb2, Aabb3>, GeometryError> |
| operations/segments.h | segments(Polyline<N>) | vector<Segment<N>> |
| operations/triangulate.h | triangulate(Polygon2) | expected<TriangleMesh2, GeometryError> |
| operations/tessellate.h | sampleCircle(Circle2, options) | expected<Polyline2, GeometryError> |
| operations/tessellate.h | tessellate(Disk2 / Rectangle2, options) | expected<TriangleMesh2, GeometryError> |
| operations/tessellate.h | tessellate(Sphere3 / Box3, options) | expected<TriangleMesh3, GeometryError> |
| operations/tessellate.h | tessellate(Geometry, options) | expected<TessellatedGeometry, GeometryError> |

bounds поддерживает Point, Segment, Triangle, Polyline, Polygon2, TriangleMesh,
Aabb, Rectangle2, Circle2, Disk2, Sphere3 и Box3. Для Line, Ray и Plane конечных
bounds без области ограничения нет. Пустая сетка допустима, но bounds возвращает
EmptyGeometry.

Для runtime-значения `bounds(const Geometry&)` сам выбирает конкретную операцию.
Результат `RuntimeBoundsResult` содержит `BoundsGeometry` — `variant<Aabb2, Aabb3>`
с размерностью исходной геометрии. Неподдерживаемые Line, Ray и Plane дают
UnsupportedOperation. Изменённый Point с нечестными координатами и Geometry в
состоянии valueless_by_exception дают InvalidGeometry. Ошибки конкретных операций
сохраняются, а результат владеет своими данными независимо от исходного Geometry.

`tessellate(const Geometry&, options)` возвращает `RuntimeTessellationResult` с
`TessellatedGeometry` — `variant<Polyline2, TriangleMesh2, TriangleMesh3>`. Circle2
делегируется `sampleCircle`, Disk2 / Rectangle2 / Sphere3 / Box3 — соответствующим
типизированным перегрузкам. Для остальных альтернатив возвращается
UnsupportedOperation. В частности, Polygon2 триангулируется отдельным вызовом
`triangulate`, а не runtime-перегрузкой tessellate. Ошибки опций, бюджета и численных
вычислений передаются без изменения; результат не зависит от времени жизни входа.

Geometry — закрытый каталог геометрических значений, а не гарантия поддержки всех
операций или отображаемости. Aabb и TriangleMesh могут быть как входными объектами,
так и результатами вычислений. Фигуры с радиусом и размерами используют локальные
координаты с центром в начале; геометрия с явными точками сохраняет их координаты.
Мировое размещение, преобразования и идентичность экземпляров сцены относятся к Core.
Point остаётся изменяемым unchecked-значением Math: операция, использующая его,
должна проверять фактические координаты, как это делает bounds.

Замкнутая полилиния даёт дополнительный последний отрезок. Окружность даёт контур,
диск — заполненную сетку. Триангуляция ear clipping поддерживает вогнутые простые
контуры и оба направления обхода; выходные треугольники ориентированы против
часовой стрелки. Коллинеарные точки границы могут остаться неиспользованными
позициями сетки. Отверстия и самопересечения не поддерживаются.

Сфера строится подразделением октаэдра с общими вершинами рёбер; грани сферы и
Box ориентированы наружу. Mesh хранит позиции и индексы uint32_t без копий Triangle.
Нормали, UV и материалы в первую реализацию не входят.

## Численные и ресурсные правила

- Все геометрические параметры конечны. Радиусы и размеры положительны;
  направления и нормаль плоскости ненулевые. Aabb допускает нулевые размеры.
- Triangle отвергает коллинеарные / совпадающие вершины. Mesh проверяет конечность
  позиций и диапазоны индексов, но допускает вырожденные грани как контейнер данных.
- Polygon2 нормализует повтор первой точки в конце, отвергает нулевые рёбра,
  перекрытия и пересечения, допускает прямолинейные промежуточные точки.
- Предикаты используют long double без общего глобального epsilon и без точной
  арифметики. Почти вырожденные данные ограничены точностью floating point;
  NumericalFailure сообщает о выявленной невозможности вычисления.
- maxDeviation — конечная положительная абсолютная погрешность в единицах
  геометрии. Для окружности / диска ограничивается отклонение дуги от хорды,
  для сферы — радиальная погрешность граней, с учётом округления floating point.
  Box / Rectangle используют те же валидируемые options без криволинейной аппроксимации.
- maxPrimitiveCount ограничивает отрезки для sampleCircle и треугольники для
  tessellate. При нехватке бюджета для заданной точности возвращается BudgetExceeded.
  Значения options по умолчанию — 0.01 и 1 000 000.
- Если половина размера Rectangle / Box округляется в ноль, bounds / tessellate
  возвращают NumericalFailure. Предел примитивов не гарантирует время вычисления.
- Проверка простоты Polygon2 квадратичная; текущий ear clipping имеет кубическую
  верхнюю оценку. Интерактивная работа с произвольно большими контурами не обещана.
- std::bad_alloc может распространяться как исключение. Потоков, очередей,
  глобального хранилища и Scene ObjectId в библиотеке нет.

## Сборка и проверки

```sh
cmake --preset ninja
cmake --build --preset ninja
ctest --test-dir build --output-on-failure
./build/examples/objectlib/huynity_objectlib_example
```

BUILD_TESTING управляет тестами; HUYNITY_BUILD_EXAMPLES — примером. Отдельная цель:
`cmake --build --preset ninja --target huynity_objectlib`.

Восемь групп тестов проверяют создание, примитивы, коллекции и владение, оба входа
фабрики, bounds, триангуляцию, тесселяцию и численные границы. Независимые проверки
используют площади, объём, принадлежность точек, замкнутость сетки, ориентацию
граней и отклонение от исходной поверхности. Несовместимые Params и неподдерживаемые
теги проверяются на этапе компиляции.

Кривые, NURBS-поверхности, Cylinder / Cone / Torus и CompositeObject остаются
следующим этапом, как отмечено в третьем представлении UML.
