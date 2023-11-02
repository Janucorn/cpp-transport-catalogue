#include <iostream>
#include <iomanip>
#include <string_view>

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

using namespace transport_ctg;

int main() {
	
	input::Reader input_reader;
	statreader::StatReader stat_reader(input_reader);

	input_reader.SetCatalogue();
	stat_reader.PrintInfo();
}