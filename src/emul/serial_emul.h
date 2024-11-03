#pragma once
#include <string_view>

std::string_view uart_output();
void uart_input(std::string_view text);
