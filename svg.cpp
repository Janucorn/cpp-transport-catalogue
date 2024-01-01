#include "svg.h"

namespace svg {

	using namespace std::literals;

	void Object::Render(const RenderContext& context) const {
		context.RenderIndent();

		// делегируем вывод тега своим подклассам
		RenderObject(context);

		context.out << std::endl;
	}

	// ------- Circle -----------------

	Circle& Circle::SetCenter(Point center) {
		center_ = center;
		return *this;
	}

	Circle& Circle::SetRadius(double radius) {
		radius_ = radius;
		return *this;
	}

	void Circle::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
		out << "r=\""sv << radius_ << "\""sv;

		// выводим атрибуты
		RenderAttrs(out);

		out << "/>"sv;
	}

	// ------------- Polyline ---------------

	Polyline& Polyline::AddPoint(Point point) {
		points_.push_back(std::move(point));
		return *this;
	}

	void Polyline::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<polyline points=\""sv;
		bool is_first = true;
		for (const auto& point : points_) {
			if (is_first) {
				out << point.x << ',' << point.y;
				is_first = false;
			} else {
				out << ' ' << point.x << ',' << point.y;
			}
		}
		out << "\""sv;
		// выводим атрибуты
		RenderAttrs(out);

		out << "/>"sv;
	}

	// ------------- Text -------------------

	Text& Text::SetPosition(Point position) {
		pos_ = position;
		return *this;
	}

	Text& Text::SetOffset(Point offset) {
		offset_ = offset;
		return *this;
	}

	Text& Text::SetFontSize(uint32_t size) {
		font_size_ = size;
		return *this;
	}

	Text& Text::SetFontFamily(std::string font_family) {
		font_family_ = std::move(font_family);
		return *this;
	}

	Text& Text::SetFontWeight(std::string font_weight) {
		font_weight_ = std::move(font_weight);
		return *this;
	}

	Text& Text::SetData(std::string data) {
		data_ = std::move(data);
		return *this;
	}

	void Text::RenderObject(const RenderContext& context) const {
		auto& out = context.out;
		out << "<text"sv;
		// выводим атрибуты
		RenderAttrs(out);
		out << " x=\""sv << pos_.x << "\" y=\""sv << pos_.y << "\""sv;
		out << " dx=\""sv << offset_.x << "\" dy=\""sv << offset_.y << "\""sv;
		out << " font-size=\""sv << font_size_ << "\""sv;
		if (!font_family_.empty()) {
			out << " font-family=\""sv << font_family_ << "\""sv;
		}
		if (!font_weight_.empty()) {
			out << " font-weight=\""sv << font_weight_ << "\""sv;
		}
		out << ">"sv << data_ << "</text>"sv;
	}

	// ------------- Document ---------------

	void Document::AddPtr(std::unique_ptr<Object>&& obj) {
		objects_.emplace_back(std::move(obj));
	}

	void Document::Render(std::ostream& out) const {
		RenderContext context(out, 2, 2);
		out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
		out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;
		for (const auto& obj : objects_) {
			obj->Render(context);
		}
		out << "</svg>"sv;
	}

	std::ostream& operator<<(std::ostream& out, const StrokeLineCap line_cap) {
		switch (line_cap) {
		case StrokeLineCap::BUTT: {
			out << "butt"sv;
			break;
		}
		case StrokeLineCap::ROUND: {
			out << "round"sv;
			break;
		}
		case StrokeLineCap::SQUARE: {
			out << "square"sv;
			break;
		}
		default: {
			break;
		}
		}
		return out;
	}

	std::ostream& operator<<(std::ostream& out, const StrokeLineJoin line_join) {
		switch (line_join) {
		case StrokeLineJoin::ARCS: {
			out << "arcs"sv;
			break;
		}
		case StrokeLineJoin::BEVEL: {
			out << "bevel"sv;
			break;
		}
		case StrokeLineJoin::MITER: {
			out << "miter"sv;
			break;
		}
		case StrokeLineJoin::MITER_CLIP: {
			out << "miter-clip"sv;
			break;
		}
		case StrokeLineJoin::ROUND: {
			out << "round"sv;
			break;
		}
		default: {
			break;
		}
		}
		return out;
	}

	std::ostream& operator<<(std::ostream& out, const Color& color) {
		std::visit(OstreamColorPrinter{ out }, color);
		return out;
	}

} // namespace svg