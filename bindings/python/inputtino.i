%module inputtino

// Add necessary symbols to generated header
%{
#include <inputtino/result.hpp>
#include <inputtino/input.hpp>
%}


// Parse the original header file
%include "inputtino/result.hpp"
%include "inputtino/input.hpp"
