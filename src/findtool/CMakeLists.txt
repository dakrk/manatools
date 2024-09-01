add_executable(findtool
	main.cpp
)

target_link_libraries(findtool PRIVATE
	manatools::manatools
	mio::mio
)

manatools_target(findtool)
