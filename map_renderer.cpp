#include "map_renderer.h"

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

using namespace std::literals;

namespace renderer {

inline const double EPSILON = 1e-6;
bool IsZero(double value) {
	return std::abs(value) < EPSILON;
}

svg::Point SphereProjector::operator()(geo::Coordinates cords) const {
	return {
		(cords.lng - min_lon_) * zoom_coeff_ + padding_,
		(max_lat_ - cords.lat) * zoom_coeff_ + padding_
	};
}

// ------ MapRender ------
MapRenderer::MapRenderer(const RenderSettings& settings)
	: render_settings_(settings) {
}

std::vector<svg::Polyline> MapRenderer::GetRoute(const std::map<std::string_view, transport_ctg::Bus*>& buses, const SphereProjector& sp) const{
	std::vector<svg::Polyline> result;
	size_t color_num = 0;
	for (const auto& [busname, bus_ptr] : buses) {
		if (bus_ptr->stops_ptr.empty()) {
			continue;
		}
		svg::Polyline route;
		for (const auto& stop : bus_ptr->stops_ptr) {
			route.AddPoint(sp(stop->coordinates));
		}
		route.SetStrokeColor(render_settings_.color_palette[color_num]);
		route.SetStrokeWidth(render_settings_.line_width);
		route.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
		route.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
		route.SetFillColor("none"s);

		if (color_num + 1 < render_settings_.color_palette.size()) {
			++color_num;
		} else {
			color_num = 0;
		}
		result.push_back(route);
	}
	return result;
}

// названия маршрутов должны быть нарисованы в алфавитном порядке.
// сначала выводится название для его первой конечной остановки, а затем, 
// если маршрут некольцевой и конечные не совпадают - для второй конечной
// Название маршрутов должно выводиться в двух текстовых объектах: подложке и самой надписи
std::vector<svg::Text> MapRenderer::GetBusLabel(const std::map<std::string_view, transport_ctg::Bus*>& buses, const SphereProjector& projector) const {
	std::vector<svg::Text> result;
	svg::Text text;
	svg::Text underlayer;
	size_t color_num = 0;

	for (const auto& [busname, bus] : buses) {
		if (bus->stops_ptr.empty()) {
			continue;
		}
		// задаем общие настройки для самой надписи и подложки
		text.SetPosition(projector(bus->stops_ptr[0]->coordinates));
		text.SetOffset(render_settings_.bus_label_offset);
		text.SetFontSize(render_settings_.bus_label_font_size);
		text.SetFontFamily("Verdana"s);
		text.SetFontWeight("bold"s);
		text.SetData(static_cast<std::string>(bus->name));
		underlayer = text;
		
		// доп свойства надписи - цвет заливки должен соответствовать цвету маршрута
		text.SetFillColor(render_settings_.color_palette[color_num]);
		if (color_num + 1 < render_settings_.color_palette.size()) {
			++color_num;
		} else {
			color_num = 0;
		}

		// доп свойства подложки
		underlayer.SetFillColor(render_settings_.underlayer_color);
		underlayer.SetStrokeColor(render_settings_.underlayer_color);
		underlayer.SetStrokeWidth(render_settings_.underlayer_width);
		underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
		underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

		result.push_back(underlayer);
		result.push_back(text);

		// добавляем надпись для второй конечной остановки, если маршрут некольцевой и конечные различны
		if (!(bus->is_roundtrip)) {
			// вторая конечная остановка находится в середине всего маршрута
			const auto& second_last_stop = bus->stops_ptr[bus->stops_ptr.size() / 2];

			if (bus->stops_ptr[0] != second_last_stop) {
				svg::Text text2 = text;
				svg::Text underlayer2 = underlayer;
				text2.SetPosition(projector(second_last_stop->coordinates));
				underlayer2.SetPosition(projector(second_last_stop->coordinates));

				result.push_back(underlayer2);
				result.push_back(text2);
			}
		}
	}
	return result;
}

// выводит изображение в виде кружочков для каждой остановки в порядке возрастания
std::vector<svg::Circle> MapRenderer::GetStopsSymbols(const std::map<std::string_view, transport_ctg::Stop*>& sorted_stops, const SphereProjector& projector) const{
	std::vector<svg::Circle> result;
	svg::Circle symbol;
	for (const auto& [stopname, stop] : sorted_stops) {
		symbol.SetCenter(projector(stop->coordinates));
		symbol.SetRadius(render_settings_.stop_radius);
		symbol.SetFillColor("white"s);
		result.push_back(symbol);
	}
	return result;
}

// sorted_stops список остановок в порядке возрастания, через которые проезжает хотя бы один маршрут
std::vector<svg::Text> MapRenderer::GetStopLabel(const std::map<std::string_view, transport_ctg::Stop*>& sorted_stops, const SphereProjector& projector) const {
	std::vector<svg::Text> result;
	svg::Text text;
	svg::Text underlayer;

	for (const auto& [stopname, stop] : sorted_stops) {
		text.SetPosition(projector(stop->coordinates));
		text.SetOffset(render_settings_.stop_label_offset);
		text.SetFontSize(render_settings_.stop_label_font_size);
		text.SetFontFamily("Verdana"s);
		text.SetData(static_cast<std::string>(stopname));
		underlayer = text;
		
		// дополнительные свойства для подложки
		underlayer.SetFillColor(render_settings_.underlayer_color);
		underlayer.SetStrokeColor(render_settings_.underlayer_color);
		underlayer.SetStrokeWidth(render_settings_.underlayer_width);
		underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
		underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

		// дополнительные свойства для надписи
		text.SetFillColor("black"s);
		
		result.push_back(underlayer);
		result.push_back(text);
	}
	return result;
}

svg::Document MapRenderer::GetRenderedMap(const std::map<std::string_view, transport_ctg::Bus*>& buses) const {
	svg::Document document;
	std::vector<geo::Coordinates> stops_coords;
	std::map<std::string_view, transport_ctg::Stop*> sorted_stops;

	for (const auto& [busname, bus] : buses) {
		if (bus->stops_ptr.empty()) {
			continue;
		}
		for (const auto& stop : bus->stops_ptr) {
			stops_coords.push_back(stop->coordinates);
			sorted_stops.emplace( stop->name, stop );
		}
	}

	SphereProjector projector(
		stops_coords.begin(), stops_coords.end(),
		render_settings_.width, render_settings_.height,
		render_settings_.padding
	);

	for (const auto& line : GetRoute(buses, projector)) {
		document.Add(line);
	}
	for (const auto& bus_label : GetBusLabel(buses, projector)) {
		document.Add(bus_label);
	}
	for (const auto& stop_symbols : GetStopsSymbols(sorted_stops, projector)) {
		document.Add(stop_symbols);
	}
	for (const auto& stop_label : GetStopLabel(sorted_stops, projector)) {
		document.Add(stop_label);
	}

	return document;
}


} // namespace renderer