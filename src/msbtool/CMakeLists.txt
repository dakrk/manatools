if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
	# Silence GCC incorrectly warning about the std::variant
	add_compile_options(-Wno-maybe-uninitialized)
endif()

add_executable(msbtool
	main.cpp
)

target_link_libraries(msbtool PRIVATE
	manatools::manatools
)

manatools_target(msbtool)
