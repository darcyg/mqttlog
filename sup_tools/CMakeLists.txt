project(sup_tools)

SET(AGENT_SUBDIR_PATH app_tcf_agent/agent) 

add_subdirectory(${AGENT_SUBDIR_PATH})

file(WRITE ${CMAKE_BINARY_DIR}/tmp/root_agent.sh "#!/bin/bash\n")
file(APPEND ${CMAKE_BINARY_DIR}/tmp/root_agent.sh "kdesudo -c '${sup_tools_BINARY_DIR}/${AGENT_SUBDIR_PATH}/tcf-agent'\n")   
file(COPY ${CMAKE_BINARY_DIR}/tmp/root_agent.sh DESTINATION ${sup_tools_BINARY_DIR} FILE_PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ)
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/tmp/)
