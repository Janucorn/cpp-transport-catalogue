/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * В качестве источника для идей предлагаем взглянуть на нашу версию обработчика запросов.
 * Вы можете реализовать обработку запросов способом, который удобнее вам.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

 // Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
 // с другими подсистемами приложения.
 // См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)
 /*
 class RequestHandler {
 public:
	 // MapRenderer понадобится в следующей части итогового проекта
	 RequestHandler(const TransportCatalogue& db, const renderer::MapRenderer& renderer);

	 // Возвращает информацию о маршруте (запрос Bus)
	 std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

	 // Возвращает маршруты, проходящие через
	 const std::unordered_set<BusPtr>* GetBusesByStop(const std::string_view& stop_name) const;

	 // Этот метод будет нужен в следующей части итогового проекта
	 svg::Document RenderMap() const;

 private:
	 // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
	 const TransportCatalogue& db_;
	 const renderer::MapRenderer& renderer_;
 };
 */
#include "request_handler.h"

#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace json {
namespace request_handler{

using namespace std::literals;

RequestHandler::RequestHandler(JsonReader& queries, const transport_ctg::Catalogue& catalogue, const renderer::MapRenderer& renderer, std::ostream& out)
	: queries_(queries)
	, catalogue_(catalogue)
	, renderer_(renderer) {
	PrintInfo(out);
	out << std::endl;
}

void RequestHandler::PrintInfo(std::ostream& out) const {
	// список результатов для вывода
	Array result;
	// список запросов
	const Array& queries = queries_.GetStatRequest().AsArray();

	result.reserve(queries.size());
	for (const auto& query : queries) {
		// query содержит обязательные ключи type, id
		// тип запроса
		const auto& type = query.AsMap().at("type").AsString();
		// предполагаем, что в запросах обязательно еще содержится ключ name, поэтому не проводим проверку на наличие этого ключа
		if (type == "Stop") {
			result.push_back(PrintStop(query.AsMap()));
		}
		if (type == "Bus") {
			result.push_back(PrintRoute(query.AsMap()));
		}
		if (type == "Map") {
			result.push_back(PrintMap(query.AsMap()));
		}
	}
	// выводим результат в выходной поток
	Print(Document{ result }, out);
}

const Node RequestHandler::PrintStop(const Dict& query) const {
	Node result;
	const auto& stopname = query.at("name"s).AsString();
	const int id = query.at("id"s).AsInt();
	const auto& stop_ptr = catalogue_.FindStop(stopname);

	// проверяем есть ли наличие маршрутов проходящих через эту остановку
	if (!stop_ptr) {
		result = Builder{}
			.StartDict()
                .Key("request_id"s).Value(id)
			.Key("error_message"s).Value("not found"s)
			.EndDict()
			.Build();
	} else {
		Array buses;
		for (const auto& bus : catalogue_.GetBusesForStop(stopname)) {
			buses.push_back(static_cast<std::string>(bus));
		}
		result = Builder{}
			.StartDict()
			.Key("buses"s).Value(buses)
			.Key("request_id"s).Value(id)
			.EndDict()
			.Build();
	}
	return  result;
}

const Node RequestHandler::PrintRoute(const Dict& query) const {
	Node result;
	const int id = query.at("id"s).AsInt();
	const auto& busname = query.at("name"s).AsString();
	const auto& bus_ptr = catalogue_.FindBus(busname);

	if (!bus_ptr) {
		result = Builder{}
			.StartDict()
			.Key("request_id"s).Value(id)
			.Key("error_message"s).Value("not found"s)
			.EndDict()
			.Build();
	} else {
		const auto& bus_info = catalogue_.GetBusInfo(busname);
		result = Builder{}
			.StartDict()
                .Key("request_id"s).Value(id)
			.Key("curvature"s).Value(bus_info.route_length / bus_info.coordinate_length)
			.Key("route_length"s).Value(bus_info.route_length)
			.Key("stop_count"s).Value(bus_info.stops_on_route)
			.Key("unique_stop_count"s).Value(bus_info.unique_stops)
			.EndDict()
			.Build();
	}
	return result;
}

void RequestHandler::PrintRenderedMap(std::ostream& out) const {
	const auto& sorted_buses = catalogue_.GetSortedBuses();
	const svg::Document& document = renderer_.GetRenderedMap(sorted_buses);
	document.Render(out);
}

const Node RequestHandler::PrintMap(const Dict& query) const {
	Node result;
	std::ostringstream strm(""s);
	PrintRenderedMap(strm);
	const int id = query.at("id"s).AsInt();

	result = Builder{}
		.StartDict()
		.Key("map"s).Value(strm.str())
		.Key("request_id"s).Value(id)
		.EndDict()
		.Build();
	return result;
}
} // namesapce request_handler
} // namespace json