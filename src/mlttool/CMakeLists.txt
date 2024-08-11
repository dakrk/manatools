add_executable(mlttool
	main.cpp
)

target_link_libraries(mlttool PRIVATE
	manatools::manatools
)

manatools_target(mlttool)
