file(GLOB_RECURSE COMPONENT_SRCS "src/*.c")
file(GLOB_RECURSE WEB_FILES "webpage/*")

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS
        "."
        "include"
    EMBED_FILES ${WEB_FILES}
)

target_add_binary_data(${COMPONENT_TARGET} "certs/aws_root_ca_pem" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/certificate_pem_crt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "certs/private_pem_key" TEXT)
