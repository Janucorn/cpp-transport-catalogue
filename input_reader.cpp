#include "input_reader.h"
#include "transport_catalogue.h"

#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace transport_ctg {

using namespace std::literals;
namespace input {
namespace detail {

std::string ReadLine(std::istream& in) {
	std::string str;
	std::getline(in, str);
	return str;
}

int ReadLineWithNumber(std::istream& in) {
	int result;
	in >> result;
	ReadLine(in);
	return result;
}

// разбивает запрос на тип и текст запроса
Query QueryType(std::string& text) {
	Query query;
	std::string_view type(text);
	auto space = type.find(' ');
	// Тип запроса (Stop/Bus)
	query.type = type.substr(0u, space);
	// удаляем тип из запроса
	type.remove_prefix(std::min(type.size(), space));
	// Удаляем пробелы из начала текста запроса
	type.remove_prefix(std::min(type.size(), type.find_first_not_of(' ')));
	query.text = type;

	return query;
}

// создаем имя маршрута/остановки
std::string_view CreateName(std::string_view& text) {
	auto pos = text.find(':');
	// Определяем имя 
	std::string_view name = text.substr(0u, pos);
	// Удаляем имя остновки с символом ":" из текста
	text.remove_prefix(pos + 1u);
	// Ищем пробелы перед координатами
	pos = text.find_first_not_of(' ');
	if (pos != text.npos) {
		text.remove_prefix(pos);
	}
	return name;
}

// Считываем запросы
std::vector<detail::Query> SetQueryBase(std::istream& in) {
	int query_count = detail::ReadLineWithNumber(in);
	std::vector<Query> queries;
	queries.reserve(query_count);

	// Создаем список запросов и их типов
	for (int i = 0; i < query_count; ++i) {
		auto text = ReadLine(in);
		queries.push_back(std::move(QueryType(text)));
	}
	return queries;
}
}// end of namespace transport_ctg::input::detail

// ****реализовать конструктор иначе
Reader::Reader()
	: query_count_(0){
}

Catalogue& Reader::GetCatalogue() {
	return catalogue_;
}

 // Создаем каталог
void Reader::SetCatalogue(std::istream& in) {

	queries_ = std::move(detail::SetQueryBase(in));

	// добавляем остановки в базу
	for (const auto& query : queries_) {
		if (query.type == "Stop"sv) {
			CreateStopBase(query.text);
		}
	}
	// добавляем расстояния между остановками
	for (const auto& query : queries_) {
		if (query.type == "Stop"sv) {
			AddDistance(query.text);
		}
	}
	// Обработка запроса маршрута
	for (const auto& query : queries_) {
		if (query.type == "Bus") {
			CreateBusBase(query.text);
		}
	}
} // end InputReader::SetCatalogue()

// Добавляем расстояния между остановками
void Reader::AddDistance(const std::string_view& query_text) {
	std::string_view text(query_text);
	std::string_view from_stop = detail::CreateName(text);
	std::string_view to_stop;
	std::string distance_str;
	double distance = 0.;

	auto pos = text.find(',');
	// удалили первую координату
	text.remove_prefix(std::min(text.size(), pos + 1u));

	// удалили вторую координату с запятой ',' и с пробелами 
	pos = text.find(',');
	// помимо координат ничего нет в тексте, поэтому прекрашаем обработку
	if (pos == text.npos) {
		return;
	}
	text.remove_prefix(std::min(text.size(), text.find_first_not_of(' ', pos + 1u)));

	// подразумевается, что после наименования всегда следует пара координат
	while (!text.empty()) {

		// определяем расстояние
		pos = text.find_first_of('m');
		if (pos != text.npos) {
			distance_str = text.substr(0u, pos);
			distance = std::stod(std::move(distance_str));
		}
		// определяем наименование другой остановки
		pos = text.find("to"sv);
		if (pos != text.npos) {
			text.remove_prefix(text.find_first_not_of(' ', pos + 2u));
		}
		// оперделяем конец наименования
		pos = text.find(',');
		// это конец строки
		if (pos == text.npos) {
			to_stop = text;
			text.remove_prefix(text.size());
		}
		// записываем наименование и удаляем пробелы после
		else {
			to_stop = text.substr(0u, pos);
			text.remove_prefix(std::min(text.size(), text.find_first_not_of(' ', pos + 1u)));
		}
		// Остановки уже должны иметься в базе
		auto stopFromPtr = catalogue_.FindStop(from_stop);
		auto stopToPtr = catalogue_.FindStop(to_stop);
		catalogue_.AddDistanceBetweenStops({ stopFromPtr, stopToPtr }, distance);
	}
}

// Обрабатываем стоп-запрос и вносим его в базу
Stop Reader::CreateStopBase(const std::string_view query_text) {
	std::string_view text(query_text);
	std::string lat_str, lng_str;
	Stop stop;

	stop.name = detail::CreateName(text);

	// Ищем разделитель после первой координаты	
	auto pos = text.find(',');
	// Записываем первую координату
	lat_str = text.substr(0u, pos);
	stop.latitude = std::stod(std::move(lat_str));

	// Удаляем первую координату и знак ","
	text.remove_prefix(pos + 1u);

	// Ищем пробелы перед координатами
	pos = text.find_first_not_of(' ');
	if (pos != text.npos) {
		text.remove_prefix(pos);
	}
	// Записываем вторую координату
	lng_str = text.substr(0u, text.find_first_of(' '));
	stop.longitude = std::stod(std::move(lng_str));

	// Подумать как улучшить и не захватывать лишнее в конце
	// или обрбатывать остаток дальше (расстояние до др остановки)
	catalogue_.AddStop(stop);

	return stop;
}

// Обрабатываем Bus-запрос и вносим маршрут в базу
Bus Reader::CreateBusBase(const std::string_view query_text) {
	Bus bus;
	std::string_view text(query_text);
	
	bus.name = detail::CreateName(text);
	bool is_ring_route = false;

	// Определяем кольцевой ли маршрут
	auto separator = text.find('>');
	if (separator != text.npos) {
		is_ring_route = true;
	}
	// добавляем остановки
	while (!text.empty()) {
		std::string_view stop_to_bus;

		if (is_ring_route) {
			separator = text.find('>');
		}
		else {
			separator = text.find('-');
		}

		if (separator == text.npos) {
			Stop* stop_ptr = catalogue_.FindStop(text);
			bus.stops_ptr.push_back(std::move(stop_ptr));
			stop_ptr = nullptr;
			break;
		}
		else {
			// Определяем название остановки до разделителя
			stop_to_bus = text.substr(0, separator);
			// Удаляем пробелы в конце названия
			auto last_char = stop_to_bus.find_last_not_of(' ');
			if (last_char != stop_to_bus.npos) {
				stop_to_bus = stop_to_bus.substr(0, last_char + 1u);
			}
		}
		// Ищем указатель на остановку
		Stop* stop_ptr = catalogue_.FindStop(stop_to_bus);
		bus.stops_ptr.push_back(stop_ptr);

		// Определяем позицию первого непробельного символа после разделителя
		auto new_stop = text.find_first_not_of(' ', separator + 1u);
		text.remove_prefix(std::min(text.size(), new_stop));
	}
	// Добавляем остановки, если маршрут некольцевой
	if (!is_ring_route) {
		std::vector<Stop*> temp = bus.stops_ptr;

		for (auto it = rbegin(temp) + 1; it != rend(temp); ++it) {
			bus.stops_ptr.push_back(*it);
		}
	}
	catalogue_.AddBus(bus.name, bus.stops_ptr);
	return bus;
}// конец обрабатки маршрута

} // end of namespace "transport_ctg::input
} // end of namespace "transport_ctg"