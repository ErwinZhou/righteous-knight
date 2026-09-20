#pragma once

// defers global resource loading until the OpenGL context is ready
// load tags control initialization order


#include <functional>
#include <stdexcept>
#include <cstdint>

enum LoadTag : uint32_t {
	LoadTagEarly,
	LoadTagDefault,
	LoadTagLate,
	MaxLoadTag //<-- just used to track # of load tags
};

//Add a function to an internal list of loading functions:
// (only call *before* "call_load_functions()")
void add_load_function(LoadTag tag, std::function< void() > const &fn);

//Call all loading functions:
// (loading functions may throw exceptions if they fail.)
// (only call *once*)
void call_load_functions();


//work-around for MSVC not accepting this as a lambda:
template< typename T >
T const *new_T() { return new T; }

template< typename T >
struct Load {
	//Constructing a Load< T > adds the passed function to the list of functions to call:
	Load(LoadTag tag, const std::function< T const *() > &load_fn = new_T< T >) : value(nullptr) {
		add_load_function(tag, [this,load_fn](){
			this->value = load_fn();
			if (!(this->value)) {
				throw std::runtime_error("Loading failed.");
			}
		});
	}

	//Make a "Load< T >" behave like a "T const *":
	explicit operator bool() { return value != nullptr; }
	operator T const *() { return value; }
	T const &operator*() { return *value; }
	T const *operator->() { return value; }

	T const *value;
};


//Specialization:
//Load< void > just calls a function:
template< >
struct Load< void > {
	//Constructing a Load< T > adds the passed function to the list of functions to call:
	Load( LoadTag tag, const std::function< void() > &load_fn) {
		add_load_function(tag, load_fn);
	}
};


