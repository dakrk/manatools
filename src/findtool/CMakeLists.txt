add_executable(findtool
	main.cpp
)

target_link_libraries(findtool PRIVATE
	manatools::manatools
)

manatools_target(findtool)
