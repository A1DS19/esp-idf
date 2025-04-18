file(GLOB_RECURSE COMPONENT_SRCS "src/*.c")
file(GLOB_RECURSE WEB_FILES "webpage/*")

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS
        "."
        "include"
    EMBED_FILES ${WEB_FILES}
)

