#include <iostream>
#include <string>

#include <custom_variant.hpp>

int main() {
	using namespace custom_variant;

	// Create a variant_t that can hold int, double, and std::string
	variant_t<int, double, char, std::string> var;

	// Set the variant to hold an int
	var = 42;

	// Access the value of the variant as an int
	int value_int = var.get<int>();
	std::cout << "Variant holds an int: " << value_int << std::endl;

	// Automatic type conversion - retrieving a float value as a double
	var = 3.14f;
	double value_double = var.get<double>();
	std::cout << "Variant holds a float, retrieved as double: " << value_double << std::endl;

	// Set the variant to hold a std::string
	var = "Hello, variant!";

	// Access the value of the variant as a std::string
	std::string value_string = var.get<std::string>();
	std::cout << "Variant holds a string: " << value_string << std::endl;

	// Accessing the variant's value using index
	var = 10;
	int value_at_index = var.get<0>(); // Index 0 corresponds to int
	std::cout << "Variant at index 0 (int): " << value_at_index << std::endl;

	// Using the visit function to apply a visitor to the variant
	var = "Hello, visit!";
	var.visit([](const auto& value) {
		std::cout << "Variant holds: " << value << std::endl;
	});

	// Check the index of the current value in the variant
	std::cout << "Current index: " << var.index() << std::endl;

	return 0;
}