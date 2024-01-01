#pragma once

/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */
#include "json.h"
#include "json_builder.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

#include <iostream>

namespace json {
	class JsonReader {
	public:
		JsonReader(std::istream& input);

		const Node& GetBaseRequest() const;
		const Node& GetStatRequest() const;
		const Node& GetRenderSettings() const;
		const Node& GetRoutingSettings() const;

		void AddToCatalogue(transport_ctg::Catalogue& catalogue);
		renderer::MapRenderer SetMapRenderer(const Dict& settings) const;
		// задает маршрутизатор
		transport_ctg::BusRouter SetRouter(const Dict& settings, const transport_ctg::Catalogue& catalogue) const;

	private:
		json::Document queries_;

		// добавляет остановки в базу
		transport_ctg::Stop CreateStopBase(const Dict& map, transport_ctg::Catalogue& catalogue) const;

		// добавляет маршруты в базу
		transport_ctg::Bus CreateBusBase(const Dict& map, transport_ctg::Catalogue& catalogue) const;

		// таблица расстояний между остановками
		void AddDistance(transport_ctg::Catalogue& catalogue) const;

		// задает настройки для визуализации карты
		renderer::RenderSettings SetRenderSettings(const Dict& settings) const;

	};

} // end of namespace "json"