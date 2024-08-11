function(manatools_target target)
	target_compile_definitions(${target} PRIVATE "$<$<CONFIG:DEBUG>:MANATOOLS_DEBUG>")

	target_compile_features(${target} PUBLIC cxx_std_23)

	target_include_directories(${target} PRIVATE
		${PROJECT_SOURCE_DIR}/src
		${CMAKE_CURRENT_SOURCE_DIR}
	)
endfunction()
