file(READ "${INPUT}" bytes HEX)
string(REGEX REPLACE "(..)" "0x\\1," bytes "${bytes}")
file(WRITE "${OUTPUT}" "// Generated from ${INPUT}; do not edit.\n#pragma once\n#include <cstddef>\nnamespace reflow::resources {\ninline constexpr unsigned char ${SYMBOL}[] = {${bytes}0};\ninline constexpr std::size_t ${SYMBOL}_size = sizeof(${SYMBOL}) - 1;\n}\n")
