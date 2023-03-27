#include "context-manager.hh"
#include <iostream>

contextlib::context_manager<int> open(std::string const& filename)
{
	std::cerr << "aquired resource" << std::endl;
	try {
		*(co_yield 10);
	} catch (std::exception const& exception) {
		std::cerr << exception.what() << '\n';
	}
	std::cerr << "released resource" << std::endl;
}

int main()
{
	contextlib::with(open("foo"), [](int value) {
		std::cerr << value << std::endl;
		throw std::logic_error("division by 0");
	});
}
