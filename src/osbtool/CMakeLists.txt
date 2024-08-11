add_executable(osbtool
	main.cpp
)

target_link_libraries(osbtool PRIVATE
	manatools::manatools
)

manatools_target(osbtool)
