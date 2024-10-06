add_executable(fobtool
	main.cpp
)

target_link_libraries(fobtool PRIVATE
	manatools::manatools
)

manatools_target(fobtool)
