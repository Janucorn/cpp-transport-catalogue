#pragma once
#include "transport_catalogue.h"

#include <iostream>
#include <utility>
#include <string>
#include <string_view>
#include <vector>

namespace transport_ctg {
namespace input {
namespace detail{

std::string ReadLine(std::istream& in);

int ReadLineWithNumber(std::istream& in);

struct Query {
	std::string type;
	std::string text;
};

// разбиывает запрос на тип и текст
Query QueryType(std::string& text);

// создает имя маршрута или остановки
std::string_view CreateName(std::string_view& text);

// считывание запросов
std::vector<detail::Query> SetQueryBase(std::istream& in);
} // end of namespace "transport_ctg::input::detail

class Reader {
public:
	explicit Reader();
	// создание каталога
	void SetCatalogue(std::istream& in);
	// возвращает ссылку на каталог
	Catalogue& GetCatalogue();

private:
	// количество запросов (при изменении конструктора удалить)
	int query_count_;
	// список типа и текста запросов 
	std::vector<detail::Query> queries_;
	// каталог
	Catalogue catalogue_;

	// создание базы остановок
	Stop CreateStopBase(const std::string_view text);
	// создание базы маршрутов
	Bus CreateBusBase(const std::string_view text);
	// добавление расстояния между остановками
	void AddDistance(const std::string_view& text);
};
} // end of namespace "transport_ctg::input"
} // end of namespace "transport_ctg"