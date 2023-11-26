#pragma once
#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <optional>

namespace svg {

	struct Rgb {

		Rgb() = default;

		Rgb(uint8_t r, uint8_t g, uint8_t b)
			: red(r)
			, green(g)
			, blue(b) {
		}

		uint8_t red = 0;
		uint8_t green = 0;
		uint8_t blue = 0;
	};

	struct Rgba {
		Rgba() = default;

		Rgba(uint8_t r, uint8_t g, uint8_t b, double opt)
			: red(r)
			, green(g)
			, blue(b)
			, opacity(opt) {
		}

		uint8_t red = 0;
		uint8_t green = 0;
		uint8_t blue = 0;
		double opacity = 1.0; // альфа-канал от 0.0 (прозрачный) до 1.0 (непрозрачный)
	};

	// по умолчанию хранит первый в списке типов (т.е. monostate)
	using Color = std::variant<std::monostate, std::string, svg::Rgb, svg::Rgba>;
	// Объявив в заголовочном файле константу со спецификатором inline,
	// мы сделаем так, что она будет одной на все единицы трансляции,
	// которые подключают этот заголовок.
	// В противном случае каждая единица трансляции будет использовать свою копию этой константы
	inline const Color NoneColor{std::monostate()};

	struct OstreamColorPrinter {

		std::ostream& out;

		void operator()(std::monostate) const {
			out << "none";
		}

		void operator()(std::string color) const {
			out << color;
		}

		void operator()(Rgb rgb) const {
			out << "rgb(" << static_cast<int>(rgb.red) << ","
				<< static_cast<int>(rgb.green) << ","
				<< static_cast<int>(rgb.blue) << ")";
		}

		void operator()(Rgba rgba) const {
			out << "rgba(" << static_cast<int>(rgba.red) << ","
				<< static_cast<int>(rgba.green) << ","
				<< static_cast<int>(rgba.blue) << ","
				<< rgba.opacity << ")";
		}
	};

	// тип формы конца линии
	enum class StrokeLineCap {
		BUTT,
		ROUND,
		SQUARE,
	};

	// тип формы соединения линий
	enum class StrokeLineJoin {
		ARCS,
		BEVEL,
		MITER,
		MITER_CLIP,
		ROUND,
	};

	std::ostream& operator<<(std::ostream& out, const StrokeLineCap line_cap);

	std::ostream& operator<<(std::ostream& out, const StrokeLineJoin line_join);

	std::ostream& operator<<(std::ostream& out, const Color& color);

	// вспомогательный базовый класс, который будет содержать свойства, управляющие параметрами заливки и контура
	template <typename Owner>
	class PathProps {
	public:
		Owner& SetFillColor(Color color) {
			fill_color_ = std::move(color);
			return AsOwner();
		}

		Owner& SetStrokeColor(Color color) {
			stroke_color_ = std::move(color);
			return AsOwner();
		}

		Owner& SetStrokeWidth(double width) {
			width_ = std::move(width);
			return AsOwner();
		}

		Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
			line_cap_ = line_cap;
			return AsOwner();
		}

		Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
			line_join_ = line_join;
			return AsOwner();
		}
	protected:
		~PathProps() = default;

		void RenderAttrs(std::ostream& out) const {
			using namespace std::literals;

			if (fill_color_) {
				out << " fill=\""sv;
				std::visit(OstreamColorPrinter{ out }, *fill_color_);
				out << "\""sv;
			}
			if (stroke_color_) {
				out << " stroke=\""sv;
				std::visit(OstreamColorPrinter{ out }, *stroke_color_);
				out << "\""sv;
			}
			if (width_) {
				out << " stroke-width=\""sv << *width_ << "\""sv;
			}
			if (line_cap_) {
				out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
			}
			if (line_join_) {
				out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
			}
		}
	private:
		Owner& AsOwner() {
			// static_cast безопасно преобразует *this к Owner&,
			// если класс Owner - наследник PathProps
			return static_cast<Owner&>(*this);
		}
		// optional - a value that may or may not be present.
		std::optional<Color> fill_color_;
		std::optional<Color> stroke_color_;
		std::optional<double> width_;
		std::optional<StrokeLineCap> line_cap_;
		std::optional<StrokeLineJoin> line_join_;
	};

	struct Point {
		Point() = default;
		Point(double x, double y)
			: x(x)
			, y(y) {
		}
		double x = 0;
		double y = 0;
	};

	/*
	* вспомогательная структура, хранящае контекст для вывода SVG-документа с отступами.
	* Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элементов
	*/
	struct RenderContext {
		RenderContext(std::ostream& out)
			: out(out) {
		}

		RenderContext(std::ostream& out, int indent_step, int indent)
			: out(out)
			, indent_step(indent_step)
			, indent(indent) {
		}

		RenderContext Indented() const {
			return { out, indent_step, indent + indent_step };
		}

		void RenderIndent() const {
			for (int i = 0; i < indent; ++i) {
				out.put(' ');
			}
		}

		std::ostream& out;
		int indent_step = 0;
		int indent = 0;
	};

	/*
	 * Абстрактный базовый класс Object служит для унифицированоого хранения
	 * конкретных тегов SVG-документа
	 * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
	 */
	class Object {
	public:
		void Render(const RenderContext& context) const;

		virtual ~Object() = default;

	private:
		virtual void RenderObject(const RenderContext& context) const = 0;
	};

	class ObjectContainer {
	public:
		template<typename T>
		void Add(T obj);

		virtual void AddPtr(std::unique_ptr<Object>&&) = 0;

	protected:
		~ObjectContainer() = default;
	};

	// интерфейс Drawable задает объекты, которые можно нарисовать
	class Drawable {
	public:
		virtual ~Drawable() = default;
		virtual void Draw(ObjectContainer& container) const = 0;
	};

	/*
	* класс Circle моделирует элемент <circle> для отображения круга
	* https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
	*/
	// Наследованием от PathProps<Circle> мы "сообщаем" родителю, что вальцем является класс Circle
	class Circle final : public Object, public PathProps<Circle> {
	public:
		Circle& SetCenter(Point cente);
		Circle& SetRadius(double radius);
	private:
		void RenderObject(const RenderContext& context) const override;

		Point center_;
		double radius_ = 1.0;
	};

	/*
	 * класс Polyline моделирует элемент <polyline> для отображения ломанных линий
	 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
	*/
	class Polyline final : public Object, public PathProps<Polyline> {
	public:
		// добавляет очередную вершину к ломанной линии
		Polyline& AddPoint(Point point);
	private:
		void RenderObject(const RenderContext& context) const override;
		std::vector<Point> points_;
	};

	/*
	 * класс Text моделирует элемент <text> для отображения текста
	 * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
	*/
	class Text final : public Object, public PathProps<Text> {
	public:
		// Задает координаты опорной точки (x, y)
		Text& SetPosition(Point pos);

		// Задает смещение относительно опорной точки (dx, dy)
		Text& SetOffset(Point offset);

		// Задает размеры шрифта (font-size)
		Text& SetFontSize(uint32_t size);

		// Задает название шрифта (font-family)
		Text& SetFontFamily(std::string font_family);

		// Задает толщину шрифта (font-weight)
		Text& SetFontWeight(std::string font_weight);

		// Задает текстовое содержимое объекта (отображается внутри тега <text>)
		Text& SetData(std::string data);

	private:
		void RenderObject(const RenderContext& context) const override;

		Point pos_ = { 0.0, 0.0 };
		Point offset_ = { 0.0, 0.0 };
		uint32_t font_size_ = 1;
		std::string font_family_;
		std::string font_weight_;
		std::string data_;
	};

	class Document : public ObjectContainer {
	public:
		// добавляет в svg-документ объект наследник svg::Object
		void AddPtr(std::unique_ptr<Object>&& obj) override;

		// Выводит в ostream svg-представление документа
		void Render(std::ostream& out) const;
	private:
		std::vector<std::unique_ptr<Object>> objects_;
	};

	template <typename T>
	void ObjectContainer::Add(T obj) {
		AddPtr(std::make_unique<T>(std::move(obj)));
	}

} // namespace svg
