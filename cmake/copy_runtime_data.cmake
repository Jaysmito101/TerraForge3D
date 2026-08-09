if(NOT DEFINED TF3D_DATA_SOURCE OR NOT DEFINED TF3D_DATA_DESTINATION)
    message(FATAL_ERROR "TF3D_DATA_SOURCE and TF3D_DATA_DESTINATION are required")
endif()

file(COPY "${TF3D_DATA_SOURCE}/."
     DESTINATION "${TF3D_DATA_DESTINATION}"
     PATTERN "windowconfigs.terr3d" EXCLUDE)

set(config_source "${TF3D_DATA_SOURCE}/configs/windowconfigs.terr3d")
set(config_destination "${TF3D_DATA_DESTINATION}/configs/windowconfigs.terr3d")

if(EXISTS "${config_source}" AND NOT EXISTS "${config_destination}")
    file(MAKE_DIRECTORY "${TF3D_DATA_DESTINATION}/configs")
    file(COPY "${config_source}" DESTINATION "${TF3D_DATA_DESTINATION}/configs")
endif()
