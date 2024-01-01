#include "json_reader.h"

#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>

using namespace std::literals;

namespace json {

JsonReader::JsonReader(std::istream& input)
	: queries_(Load(input)) {
}

	// Описание базы маршрутов и остановок
const Node& JsonReader::GetBaseRequest() const {
	if (!queries_.GetRoot().AsMap().count("base_requests"s)) {
		static Node empty_node = nullptr;
		return empty_node;
	}
	return queries_.GetRoot().AsMap().at("base_requests"s);
}

// ответы на запросы к транаспортному справочнику
const Node& JsonReader::GetStatRequest() const {
	if (!queries_.GetRoot().AsMap().count("stat_requests"s)) {
		static Node empty_node = nullptr;
		return empty_node;
	}
	return queries_.GetRoot().AsMap().at("stat_requests"s);
}

const Node& JsonReader::GetRenderSettings() const {
	if (!queries_.GetRoot().AsMap().count("render_settings"s)) {
		static Node empty_node = nullptr;
		return empty_node;
	}
	return queries_.GetRoot().AsMap().at("render_settings"s);
}

const Node& JsonReader::GetRoutingSettings() const {
	if (!queries_.GetRoot().AsMap().count("routing_settings"s)) {
		static Node empty_node = nullptr;
		return empty_node;
	}
	return queries_.GetRoot().AsMap().at("routing_settings"s);
}

void JsonReader::AddToCatalogue(transport_ctg::Catalogue& catalogue) {
	const Array& queries = GetBaseRequest().AsArray();
	//сначала добавляем все остановки в базу
	for (const auto& query : queries) {
		const auto& query_stop = query.AsMap();
		const auto& type = query_stop.at("type"s).AsString();
		if (type == "Stop"s) {
			CreateStopBase(query_stop, catalogue);
		}
	}
	// формируем таблицу расстояний между остановками
	AddDistance(catalogue);
	
	// добавляем все маршруты в базу
	for (const auto& query : queries) {
		const auto& query_bus = query.AsMap();
		const auto& type = query_bus.at("type"s).AsString();
		if (type == "Bus"s) {
			CreateBusBase(query_bus, catalogue);
		}
	}
}

transport_ctg::Stop JsonReader::CreateStopBase(const Dict& stop_map, transport_ctg::Catalogue& catalogue) const {
	transport_ctg::Stop stop;
	stop.name = stop_map.at("name"s).AsString();
	stop.coordinates.lat = stop_map.at("latitude"s).AsDouble();
	stop.coordinates.lng = stop_map.at("longitude"s).AsDouble();
	catalogue.AddStop(stop);

	return stop;
}

void JsonReader::AddDistance(transport_ctg::Catalogue& catalogue) const {
	const Array queries = GetBaseRequest().AsArray();
	for (const auto& query : queries) {
		const auto& stop_map = query.AsMap();
		const auto& type = stop_map.at("type"s).AsString();
		if (type == "Stop"s) {
			// основная остановка
			const auto from = stop_map.at("name"s).AsString();
			// список остановок и расстояний от основной остановки
			const auto& distance = stop_map.at("road_distances"s).AsMap();
			for (const auto& [to, dist] : distance) {
				auto stopFromPtr = catalogue.FindStop(from);
				auto stopToPtr = catalogue.FindStop(to);

				catalogue.AddDistanceBetweenStops({ stopFromPtr, stopToPtr }, dist.AsInt());
			}
		}
	}
}

transport_ctg::Bus JsonReader::CreateBusBase(const Dict& bus_map, transport_ctg::Catalogue& catalogue) const {
	
	transport_ctg::Bus bus;
	const auto& stops_arr = bus_map.at("stops"s).AsArray();
	bus.name = bus_map.at("name"s).AsString();
	
	if (bus_map.at("is_roundtrip"s).AsBool()) {
		bus.stops_ptr.reserve(stops_arr.size());
		bus.is_roundtrip = true;
	} else {
		// для некольцевого маршрута добавляются остановки в обратном направлении
		bus.stops_ptr.reserve(stops_arr.size() * 2);
	}
	// добавляем указатели на остановки из базы остановок
	for (const auto& stopname : stops_arr) {
		transport_ctg::Stop* stop_ptr = catalogue.FindStop(stopname.AsString());
		bus.stops_ptr.push_back(std::move(stop_ptr));
	}

	// добавляем остановки к маршруту, если он некольцевой
	if (!bus_map.at("is_roundtrip"s).AsBool()) {
		std::vector<transport_ctg::Stop*> temp = bus.stops_ptr;
		for (auto it = std::rbegin(temp) + 1; it != std::rend(temp); ++it) {
			bus.stops_ptr.push_back(*it);
		}
	}
	catalogue.AddBus(bus);
	return bus;
}

renderer::RenderSettings JsonReader::SetRenderSettings(const Dict& request_settings) const {
	if (&GetRenderSettings() == nullptr) {
		throw std::invalid_argument("Render_settings is doesn't exist"s);
	}
	renderer::RenderSettings settings;
	settings.width = request_settings.at("width"s).AsDouble();
	settings.height = request_settings.at("height"s).AsDouble();
	settings.padding = request_settings.at("padding"s).AsDouble();
	settings.line_width = request_settings.at("line_width"s).AsDouble();
	settings.stop_radius = request_settings.at("stop_radius"s).AsDouble();
	
	settings.bus_label_font_size = request_settings.at("bus_label_font_size"s).AsInt();
	const Array& bus_offset = request_settings.at("bus_label_offset"s).AsArray();
	assert(bus_offset.size() == 2);
	settings.bus_label_offset = { bus_offset[0].AsDouble(), bus_offset[1].AsDouble() };

	settings.stop_label_font_size = request_settings.at("stop_label_font_size"s).AsInt();
	const Array& stop_offset = request_settings.at("stop_label_offset"s).AsArray();
	assert(stop_offset.size() == 2);
	settings.stop_label_offset = { stop_offset[0].AsDouble(), stop_offset[1].AsDouble() };

	const auto& color = request_settings.at("underlayer_color"s);
	if (color.IsString()) {
		settings.underlayer_color = color.AsString();
	} else if (color.IsArray()) {
		if (color.AsArray().size() == 3) {
			const auto& r = color.AsArray()[0].AsInt();
			const auto& g = color.AsArray()[1].AsInt();
			const auto& b = color.AsArray()[2].AsInt();
			settings.underlayer_color = svg::Rgb( r, g, b );
		} else if (color.AsArray().size() == 4) {
			const auto& r = color.AsArray()[0].AsInt();
			const auto& g = color.AsArray()[1].AsInt();
			const auto& b = color.AsArray()[2].AsInt();
			const auto& opacity = color.AsArray()[3].AsDouble();
			settings.underlayer_color = svg::Rgba(r, g, b, opacity);
		} else {
			throw std::logic_error("wrong underlayer color type of rgb or rgba"s);
		}
	} else {
		throw std::logic_error("wrong type of underlayer color"s);
	}
	settings.underlayer_width = request_settings.at("underlayer_width"s).AsDouble();

	const auto& color_palette = request_settings.at("color_palette"s).AsArray();
	for (const auto& clr : color_palette) {
		if (clr.IsString()) {
			settings.color_palette.push_back(clr.AsString());
		} else if (clr.IsArray()) {
			if (clr.AsArray().size() == 3) {
				const auto& r = clr.AsArray()[0].AsInt();
				const auto& g = clr.AsArray()[1].AsInt();
				const auto& b = clr.AsArray()[2].AsInt();
				settings.color_palette.push_back(svg::Rgb(r, g, b));
			} else if (clr.AsArray().size() == 4) {
				const auto& r = clr.AsArray()[0].AsInt();
				const auto& g = clr.AsArray()[1].AsInt();
				const auto& b = clr.AsArray()[2].AsInt();
				const auto& opacity = clr.AsArray()[3].AsDouble();
				settings.color_palette.push_back(svg::Rgba(r, g, b, opacity));
			} else {
				throw std::logic_error("wrong color palete type of rgb or rgba"s);
			}
		} else {
			throw std::logic_error("wrong type of color palette"s);
		}
	}
	return settings;
}

renderer::MapRenderer JsonReader::SetMapRenderer(const Dict& settings) const {
	renderer::RenderSettings render_settings = SetRenderSettings(settings);
	return { render_settings };
}

transport_ctg::BusRouter JsonReader::SetRouter(const Dict& settings, const transport_ctg::Catalogue& catalogue) const {
	if (&GetRoutingSettings() == nullptr) {
		throw std::invalid_argument("Routing_settings is doesn't exist"s);
	}
	return transport_ctg::BusRouter({
	settings.at("bus_wait_time"s).AsInt(),
	settings.at("bus_velocity"s).AsDouble()}, catalogue);
}

} // namespace json