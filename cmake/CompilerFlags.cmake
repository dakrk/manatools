# These flags should make binary sizes smaller

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(-ffunction-sections)
	add_link_options(-Wl,--gc-sections)

	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		add_compile_options(-Xclang -fmerge-functions -fnew-infallible)
		add_link_options(-fuse-ld=lld -Wl,--icf=safe)
	endif()
endif()
