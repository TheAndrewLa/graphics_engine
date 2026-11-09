# Object Library: UML-диаграммы классов

Статус: базовая модель реализована; расширения третьего представления остаются
следующим этапом. [Использование реализации](objectlib_usage.md).
Последовательность проектирования: `senior-architect` —
границы, зависимости и состав типов; `software-design-review` — создание,
владение, инварианты и возможности расширения. Основа —
[принятый API создания](objectlib_creation_api.md) и [каталог объектов](objects.md).

Модель разделена на три представления одной библиотеки: создание, базовая
геометрия и дальнейшее расширение. Первые два задают направление реализации,
третье описывает последующие возможности и не входит в обязательный первый этап.

## 1. Обозначения и решения

- `+` — публичный член, `-` — закрытое поле; подчёркнутые операции статические.
- Пунктирная стрелка — зависимость; чёрный ромб — владение вложенными значениями.
  В этих представлениях отношения наследования отсутствуют.
- `N` ограничен значениями 2 и 3. Например, `PolylineN` обозначает C++-шаблон
  `Polyline<N>`, его публичные имена — `Polyline2` и `Polyline3`.
- `CreateResult<T>` обозначает `std::expected<T, CreateError>`,
  `TessellationResult<N>` — `std::expected<TriangleMesh<N>, GeometryError>`.
  `BoundsResult<N>` — `std::expected<Aabb<N>, GeometryError>`.
  `PolylineResult<N>` — `std::expected<Polyline<N>, GeometryError>`.
  Классами библиотеки эти сокращения не являются.
- `CreationAPI` и `GeometryOperations` — UML-группы свободных функций namespace
  `objectlib`, а не C++-классы с экземплярами. Отдельной фабрики-менеджера нет.
- `ObjectTraits` и `SupportedObject` существуют на этапе компиляции. Стрелка
  `specializes` показывает специализацию шаблона, а не наследование.
- `ObjectSpec` и `Geometry` — алиасы `std::variant`, а не базовые классы. На схеме
  показаны две альтернативы для читаемости; при поддержке новых типов их добавляют
  в каталог, traits и runtime-вход.

## 2. Создание и типобезопасный API

![UML: создание объектов](diagrams/objectlib_creation.svg)

`Sphere3` и `Polyline2` показаны подробно как примеры конкретных классов. Проверка
параметров находится в их статических `create`; оба входа общей фабрики используют
эти операции. Закрытые конструкторы исключают создание обычным вызывающим кодом
объектов с непроверенными параметрами. Копирование и перемещение значений разрешены;
для изменения геометрии создаётся новое проверенное значение.

```mermaid
classDiagram
direction TB
class CreationAPI {
    <<namespace_functions>>
    +createObject_K(ParamsFor_K params) CreateResult_K
    +createObject(ObjectSpec spec) GeometryCreateResult
}
class ObjectType {
    <<enumeration>>
    Sphere3
    Polyline2
    OtherImplementedTypes
}
class ObjectTraits {
    <<template>>
    +Params : type_alias
    +Object : type_alias
}
class SupportedObject {
    <<concept>>
    +requires_Params_and_Object
}
class Sphere3Traits {
    <<specialization>>
    +Params : Sphere3Params
    +Object : Sphere3
}
class Polyline2Traits {
    <<specialization>>
    +Params : Polyline2Params
    +Object : Polyline2
}
class Sphere3Params {
    <<struct>>
    +radius : double
}
class Polyline2Params {
    <<struct>>
    +vertices : vector_Point2
    +closed : bool
}
class Sphere3 {
    <<value>>
    -m_radius : double
    -Sphere3(validatedRadius)
    +create(Sphere3Params params) SphereCreateResult$
    +radius() double
}
class Polyline2 {
    <<value_alias>>
    -m_vertices : vector_Point2
    -m_isClosed : bool
    -Polyline2(validatedParams)
    +create(Polyline2Params params) PolylineCreateResult$
    +vertices() const_point_view
    +closed() bool
}
class ObjectSpec {
    <<variant_alias>>
    Sphere3Params_or_Polyline2Params_or_other_params
}
class Geometry {
    <<variant_alias>>
    Sphere3_or_Polyline2_or_other_values
}
class CreateError {
    <<struct>>
    +code : CreateErrorCode
}
class CreateErrorCode {
    <<enumeration>>
    NonFiniteParameter
    InvalidRadius
    TooFewVertices
    OtherTypeSpecificErrors
}
CreationAPI ..> ObjectTraits : compile_time_mapping
CreationAPI ..> SupportedObject : constrained_by
ObjectTraits ..> ObjectType : template_key
SupportedObject ..> ObjectTraits : checks_aliases
Sphere3Traits ..> ObjectTraits : specializes_for_Sphere3
Polyline2Traits ..> ObjectTraits : specializes_for_Polyline2
Sphere3Traits ..> Sphere3Params : Params_alias
Sphere3Traits ..> Sphere3 : Object_alias
Polyline2Traits ..> Polyline2Params : Params_alias
Polyline2Traits ..> Polyline2 : Object_alias
CreationAPI ..> Sphere3 : delegates_to_create
CreationAPI ..> Polyline2 : delegates_to_create
Sphere3 ..> Sphere3Params : validates_and_consumes
Polyline2 ..> Polyline2Params : validates_and_consumes
CreationAPI ..> ObjectSpec : runtime_input
CreationAPI ..> Geometry : runtime_success_value
ObjectSpec ..> Sphere3Params : alternative
ObjectSpec ..> Polyline2Params : alternative
Geometry ..> Sphere3 : alternative
Geometry ..> Polyline2 : alternative
CreationAPI ..> CreateError : error_result
CreateError *-- CreateErrorCode : code_value
```

Имена `createObject_K`, `ParamsFor_K` и `CreateResult_K` — сокращения UML для
шаблонного вызова, а не предлагаемое переименование публичного API:

```cpp
template<ObjectType kind>
    requires SupportedObject<kind>
[[nodiscard]]
auto createObject(typename ObjectTraits<kind>::Params params)
    -> std::expected<typename ObjectTraits<kind>::Object, CreateError>;

auto sphere = createObject<ObjectType::Sphere3>(Sphere3Params{.radius = 2.0});
```

`OtherImplementedTypes` и `OtherTypeSpecificErrors` обозначают пропущенные элементы
каталога, а не реальные значения enum. `GeometryCreateResult` —
`std::expected<Geometry, CreateError>`. Все структуры параметров содержат геометрию;
ни одна не содержит Scene ObjectId, родителя сцены или оконных ресурсов.

## 3. Базовая геометрия 2D и 3D

![UML: базовая геометрия](diagrams/objectlib_geometry.svg)

Общие математические значения `Point<N>` и `Vector<N>` размещаются в Math и
используются через публичные алиасы. Их числовые координаты могут быть произвольными;
геометрические классы проверяют конечность при создании. Общая математика не
получает зависимости на Object Library.

Для каждого проверяемого геометрического типа предполагается `XParams` и
`X::create(XParams) -> CreateResult<X>`. Повторяющиеся структуры параметров,
статические фабрики и конструкторы здесь опущены. Стрелки владения отображают
именно сохранённые данные, а не копии ссылок на объекты Core.

```mermaid
classDiagram
direction LR
class PointN {
    <<math_value>>
    +coordinates : array_double_N
}
class VectorN {
    <<math_value>>
    +components : array_double_N
}
class SegmentN {
    <<value_template>>
    -m_firstPoint : PointN
    -m_secondPoint : PointN
    +a() PointN
    +b() PointN
}
class LineN {
    <<value_template>>
    -m_origin : PointN
    -m_direction : VectorN
    +origin() PointN
    +direction() VectorN
}
class RayN {
    <<value_template>>
    -m_origin : PointN
    -m_direction : VectorN
    +origin() PointN
    +direction() VectorN
}
class TriangleN {
    <<value_template>>
    -m_vertices : array_PointN_3
    +vertices() const_point_view
}
class PolylineN {
    <<value_template>>
    -m_vertices : vector_PointN
    -m_isClosed : bool
    +vertices() const_point_view
    +closed() bool
}
class Polygon2 {
    <<value>>
    -m_contour : vector_Point2
    +contour() const_point_view
}
class Plane3 {
    <<value>>
    -m_normal : Vector3
    -m_offset : double
    +normal() Vector3
    +offset() double
}
class Rectangle2 {
    <<value>>
    -m_size : Vector2
    +size() Vector2
}
class Circle2 {
    <<value>>
    -m_radius : double
    +radius() double
}
class Disk2 {
    <<value>>
    -m_radius : double
    +radius() double
}
class Sphere3 {
    <<value>>
    -m_radius : double
    +radius() double
}
class Box3 {
    <<value>>
    -m_size : Vector3
    +size() Vector3
}
class AabbN {
    <<value_template>>
    -m_min : PointN
    -m_max : PointN
    +min() PointN
    +max() PointN
}
class TriangleMeshN {
    <<value_template>>
    -m_positions : vector_PointN
    -m_triangles : vector_TriangleIndices
    +positions() const_point_view
    +triangles() const_index_view
}
class TriangleIndices {
    <<struct>>
    +indices : array_uint32_3
}
class GeometryOperations {
    <<namespace_functions>>
    +bounds(geometry) BoundsResult_N
    +tessellate(surface, options) TessellationResult_N
    +triangulate(Polygon2 polygon) TessellationResult_2
    +sampleCircle(Circle2 circle, options) PolylineResult_2
    +segments(PolylineN polyline) vector_SegmentN
}
class TessellationOptions {
    <<struct>>
    +maxDeviation : double
    +maxPrimitiveCount : size_t
}
class GeometryError {
    <<struct>>
    +code : GeometryErrorCode
}
class GeometryErrorCode {
    <<enumeration>>
    InvalidOptions
    BudgetExceeded
    NumericalFailure
    UnsupportedOperation
}
SegmentN "1" *-- "2" PointN : endpoints
LineN "1" *-- "1" PointN : origin
LineN "1" *-- "1" VectorN : direction
RayN "1" *-- "1" PointN : origin
RayN "1" *-- "1" VectorN : direction
TriangleN "1" *-- "3" PointN : vertices
PolylineN "1" *-- "2..*" PointN : vertices
Polygon2 "1" *-- "3..*" PointN : contour_N2
Plane3 "1" *-- "1" VectorN : normal_N3
Rectangle2 "1" *-- "1" VectorN : size_N2
Box3 "1" *-- "1" VectorN : size_N3
AabbN "1" *-- "2" PointN : corners
TriangleMeshN "1" *-- "0..*" PointN : positions
TriangleMeshN "1" *-- "0..*" TriangleIndices : triangles
GeometryOperations ..> TriangleMeshN : produces_mesh
GeometryOperations ..> AabbN : computes_bounds
GeometryOperations ..> PolylineN : reads
GeometryOperations ..> SegmentN : produces_segments
GeometryOperations ..> Polygon2 : triangulates
GeometryOperations ..> Sphere3 : tessellates
GeometryOperations ..> Box3 : tessellates
GeometryOperations ..> Circle2 : samples_outline
GeometryOperations ..> Disk2 : triangulates_disk
GeometryOperations ..> Rectangle2 : triangulates_rectangle
GeometryOperations ..> TessellationOptions : reads
GeometryOperations ..> GeometryError : operation_errors
GeometryError *-- GeometryErrorCode : code_value
```

`TriangleMesh<N>` выбран как конкретный начальный вид Mesh: позиции плюс индексы
треугольников, без требования хранить объекты `Triangle<N>`. Материалы не принадлежат
этой модели. Нормали, UV и другие атрибуты можно добавить после определения их
контрактов; обязательными полями первой версии они не объявлены.
TriangleIndices содержит только числовые индексы, ссылающиеся на позиции своей
сетки; зависимости C++-типа TriangleIndices от TriangleMesh нет.

Операции — набор перегрузок для поддерживаемых типов, а не универсальный метод
любого объекта. `bounds` для Line, Ray и Plane не возвращает конечный AABB без
заданной области ограничения; такие перегрузки в начальный набор не входят.
Из полилинии получаются отрезки, из окружности — контур, из диска — заполненная
треугольная сетка. Все эти результаты не являются одним взаимозаменяемым типом.

Ограничение `maxPrimitiveCount` сдерживает объём результата; оно не гарантирует
длительность вычисления. Параметры точности и численной устойчивости должны быть
определены для каждого алгоритма, а не сведены к одному глобальному epsilon.
В первой версии GeometryError содержит код; дополнительный контекст ошибки можно
добавить позже. Исключения выделения памяти сохраняют политику из API создания.

## 4. Кривые, поверхности и расширение каталога

![UML: последующие расширения](diagrams/objectlib_extensions.svg)

Это последующий этап. Curve и Surface обозначают требования к конкретным типам,
а не обязательную виртуальную иерархию. Связь `models` — удовлетворение контракту.
Превращать эти требования в C++ concepts следует при появлении общих алгоритмов,
которым они действительно нужны.

```mermaid
classDiagram
direction LR
class CurveContract {
    <<concept_proposal>>
    +domain() ParameterInterval
    +evaluate(t) PointEvaluationResult_N
}
class SurfaceContract {
    <<concept_proposal>>
    +domain() ParameterDomain2
    +evaluate(u, v) PointEvaluationResult_3
}
class BSplineCurveN {
    <<planned_value_template>>
    -m_degree : unsigned
    -m_knots : vector_double
    -m_controlPoints : vector_PointN
    +domain() ParameterInterval
    +evaluate(t) PointEvaluationResult_N
}
class NurbsCurveN {
    <<planned_value_template>>
    -m_degree : unsigned
    -m_knots : vector_double
    -m_controlPoints : vector_PointN
    -m_weights : vector_double
    +domain() ParameterInterval
    +evaluate(t) PointEvaluationResult_N
}
class NurbsSurface3 {
    <<planned_value>>
    -m_degreeU : unsigned
    -m_degreeV : unsigned
    -m_knotsU : vector_double
    -m_knotsV : vector_double
    -m_controlPoints : vector_Point3
    -m_weights : vector_double
    -m_uCount : size_t
    -m_vCount : size_t
    +domain() ParameterDomain2
    +evaluate(u, v) PointEvaluationResult_3
}
class Cylinder3 {
    <<planned_value>>
    -m_radius : double
    -m_height : double
    +radius() double
    +height() double
}
class Cone3 {
    <<planned_value>>
    -m_baseRadius : double
    -m_height : double
    +baseRadius() double
    +height() double
}
class Torus3 {
    <<planned_value>>
    -m_majorRadius : double
    -m_minorRadius : double
    +majorRadius() double
    +minorRadius() double
}
class PointN {
    <<math_value>>
    +coordinates : array_double_N
}
class GeometryOperations {
    <<namespace_functions>>
    +sampleCurve(curve, options) PolylineResult_N
    +tessellate(surface, options) TessellationResult_3
}
BSplineCurveN ..> CurveContract : models
NurbsCurveN ..> CurveContract : models
NurbsSurface3 ..> SurfaceContract : models
BSplineCurveN "1" *-- "2..*" PointN : control_points
NurbsCurveN "1" *-- "2..*" PointN : control_points
NurbsSurface3 "1" *-- "4..*" PointN : control_grid_N3
GeometryOperations ..> BSplineCurveN : samples
GeometryOperations ..> NurbsCurveN : samples
GeometryOperations ..> NurbsSurface3 : tessellates
GeometryOperations ..> Cylinder3 : tessellates
GeometryOperations ..> Cone3 : tessellates
GeometryOperations ..> Torus3 : tessellates
```

`PointEvaluationResult<N>` и `PolylineResult<N>` используют GeometryError.
Для кривых нужно согласовать степени, число контрольных точек, длину и порядок
узлов, область параметра; для NURBS — также веса и знаменатель вычисления.
Модель предполагает степень не ниже 1, поэтому показана соответствующая нижняя
граница количества контрольных точек. Математически допустимые постоянные кривые
потребовали бы расширения этого контракта.

У поверхности плоские массивы описывают сетку `uCount × vCount`; размеры массива
точек и весов должны совпадать с произведением. BSpline и NURBS могут использовать
общие внутренние алгоритмы, без отношения наследования между их публичными типами.

## 5. Сопоставление с исходным каталогом

| Исходное имя | Типы в модели / решение |
|---|---|
| Point | Math Point2, Point3; публичные алиасы, при необходимости теги фабрики |
| Line | Line2, Line3 — бесконечные прямые |
| Polyline | Polyline2, Polyline3 |
| Ray | Ray2, Ray3 |
| Plane | Plane3 |
| Polygon | Polygon2; Polygon3 отложен до контракта плоскостности и локального базиса |
| Rectangle | Rectangle2; размещение фигуры в 3D задаёт Core |
| Triangle | Triangle2, Triangle3 |
| Curve | Предлагаемый CurveContract для конкретных видов кривых |
| BSpline | BSplineCurve2, BSplineCurve3 — следующий этап |
| NURBS | NurbsCurve2, NurbsCurve3, NurbsSurface3 — следующий этап |
| Mesh | TriangleMesh2, TriangleMesh3; теги enum следует называть так же |
| Surface | Предлагаемый SurfaceContract для конкретных параметрических поверхностей |
| Sphere | Sphere3 |
| Box | Box3; Aabb2/3 отдельно для ограничивающих объёмов |
| Cylinder | Cylinder3 — следующий этап |
| Cone | Cone3 — следующий этап |
| Torus | Torus3 — следующий этап |
| CompositeObject | Отложен: нужно определить геометрическое объединение, группу либо другую семантику; иерархия сцены уже принадлежит Core |

Дополнены Segment2/3, Circle2, Disk2, Aabb2/3 и Vector2/3. Они устраняют смешение
отрезка с прямой, окружности с заполненным кругом и фигуры с ограничивающим объёмом.

## 6. Инварианты и время жизни

| Семейство | Предлагаемое правило |
|---|---|
| Геометрические значения | Закрытое состояние, проверяемое создание, чтение через getters; преобразование возвращает новое значение |
| Line / Ray | Конечные параметры и ненулевое направление; нормализация не обязательна |
| Segment | Конечные концы; совпадающие концы допустимы, алгоритмы учитывают нулевую длину |
| Triangle | Конечные вершины; коллинеарные / совпадающие вершины отвергаются по предикатам long double |
| Plane | Конечная ненулевая нормаль и конечное смещение в уравнении n·x + d = 0 |
| Polyline | Не менее двух точек, для closed — не менее трёх; самопересечение допустимо |
| Polygon2 | Начальный вариант — простой контур без отверстий; замыкание не требует хранения повторного конца |
| Sphere / Circle / Disk | Конечный положительный радиус; центр локального описания в начале координат |
| Rectangle / Box | Конечные положительные размеры; локальный центр в начале координат, оси совпадают с координатными |
| Aabb | Конечные углы, min ≤ max по каждой координате; нулевой размер допустим |
| TriangleMesh | Все индексы в диапазоне позиций; пустая сетка допустима; численная и топологическая политики задаются отдельно |

Методы, возвращающие const-view, не передают владение. Представление действует
пока жив соответствующий объект и не происходит его перемещение или присваивание;
для асинхронной задачи требуется владение самим объектом / стабильным снимком.
Параллельные чтения допускаются при отсутствии скрытого изменяемого кеша;
кеш тесселяции первоначально находится во внешнем слое подготовки кадра.

Point и Vector как математические агрегаты не обещают эти геометрические
инварианты. Если фабрика принимает координаты для отдельного отображаемого Point,
она проверяет конечность; низкоуровневые математические значения остаются пригодны
для промежуточных вычислений.

## 7. Результат ревью и границы реализации

1. Единого базового Object не требуется: вычислительный луч, сетка и параметрическая
   сфера имеют разные операции. Runtime-каталог выражается через variant.
2. Размерность ограничена и задаётся типом. Polyline2 в первом представлении —
   алиас PolylineN с N=2, а не отдельный наследник.
3. Фабрика не владеет объектами после возврата и не создаёт Scene ObjectId.
   Ни один класс не зависит от Core, Renderer, UI, окна или акторной библиотеки.
4. Сетка получается отдельной операцией. Математическая Sphere3 не хранит второй
   изменяемый источник истины в виде встроенной сетки.
5. Параметры открыты для составления запроса; созданные геометрические классы
   закрывают поля. Другие операции не могут обходить проверяемое создание.
6. Статическая фабрика, runtime-фабрика и будущий async-адаптер используют одни
   проверки. Добавление типа требует синхронного обновления каталога и traits.
7. Расширение сетки атрибутами, CompositeObject, численные допуски и точный набор
   операций требуют отдельных решений. Диаграмма явно оставляет их открытыми.

Реализованы 23 конкретных тега базового каталога, оба входа фабрики и операции
из второго представления. Тесты проверяют несовместимые Params, валидацию,
удержание памяти и эквивалентность статического и runtime-входа. Полные численные
правила и алгоритмы приведены в документе использования.

[PlantUML-исходник всех трёх представлений](objectlib_classes.puml).
SVG показывают ту же модель в виде UML-классов с отделениями для полей и операций.
Это проектные артефакты, а не диаграммы, извлечённые из реализованного C++.

## 8. Файлы и проверка

| Представление | SVG | Исходник Graphviz |
|---|---|---|
| Создание | [objectlib_creation.svg](diagrams/objectlib_creation.svg) | [objectlib_creation.dot](diagrams/objectlib_creation.dot) |
| Базовая геометрия | [objectlib_geometry.svg](diagrams/objectlib_geometry.svg) | [objectlib_geometry.dot](diagrams/objectlib_geometry.dot) |
| Расширения | [objectlib_extensions.svg](diagrams/objectlib_extensions.svg) | [objectlib_extensions.dot](diagrams/objectlib_extensions.dot) |

Mermaid-блоки проверяются грамматикой classDiagram; SVG генерируются локальным
Graphviz по тем же классам, членам и отношениям. Отрисованные изображения проверены
визуально. PlantUML предоставляет альтернативное представление той же модели,
но его исходник не запускался через PlantUML, которого нет в окружении.

Повторная сборка конкретного SVG из корня проекта:

```sh
dot -Tsvg docs/diagrams/objectlib_creation.dot -o docs/diagrams/objectlib_creation.svg
```
