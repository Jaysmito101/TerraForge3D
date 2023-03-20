include_directories(
	./include
	)


set(SOURCES
	./src/glad.c
	)

add_library(Glad ${SOURCES})