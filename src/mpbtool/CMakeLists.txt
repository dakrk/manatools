add_executable(mpbtool
	operations.cpp
	main.cpp
)

target_link_libraries(mpbtool PRIVATE
	manatools::manatools
	sf2cute::sf2cute
)

manatools_target(mpbtool)
