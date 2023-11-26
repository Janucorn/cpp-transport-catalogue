#pragma once
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
#include "json.h"
#include "json_reader.h"
#include "transport_catalogue.h"

#include <iostream>

namespace json {
namespace request_handler {

class RequestHandler {
public:
	explicit RequestHandler(JsonReader& queries, const transport_ctg::Catalogue& catalogue, const renderer::MapRenderer& renderer, std::ostream& out);

private:
	const JsonReader& queries_;
	const transport_ctg::Catalogue& catalogue_;
	const renderer::MapRenderer& renderer_;

	// хранит ссылку на выходной поток и выводит ответы по запросам
	void PrintInfo(std::ostream& out) const;
	// хранит ссылку на словарь и возвращает информацию для вывода остановки
	const Node PrintStop(const Dict& queryAsMap) const;
	// хранит ссылку на словарь и возвращает информацию для вывода маршрута
	const Node PrintRoute(const Dict& queryAsMap) const;

	const Node PrintMap(const Dict& queryAsMap) const;

	// выводит SVG-изображение карты
	void PrintRenderedMap(std::ostream& out) const;

};

} // namespace request_handler
} // namespace json