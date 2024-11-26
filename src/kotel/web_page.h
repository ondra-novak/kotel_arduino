#pragma once

#include "base64.h"

constexpr auto embedded_index_html = binary_data("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");
constexpr auto embedded_code_js = binary_data("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");
constexpr auto embedded_style_css = binary_data("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");

