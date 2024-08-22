add_executable(msbtool
	main.cpp
)

target_link_libraries(msbtool PRIVATE
	manatools::manatools
)

manatools_target(msbtool)
