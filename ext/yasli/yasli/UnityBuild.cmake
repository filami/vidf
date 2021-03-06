# based on
# https://cheind.wordpress.com/2009/12/10/reducing-compilation-time-unity-builds/
function(enable_unity_build UB_SUFFIX SOURCE_VARIABLE_NAME)
	set(files ${${SOURCE_VARIABLE_NAME}})
	list(SORT files)
	set(unit_build_file ${CMAKE_CURRENT_BINARY_DIR}/ub_${UB_SUFFIX}.cpp)
	set_source_files_properties(${files} PROPERTIES HEADER_FILE_ONLY true)
	set(file_content "// Unity Build generated by CMake")
	foreach(source_file ${files} )
		set(file_content "${file_content}\n#include <${CMAKE_CURRENT_SOURCE_DIR}/${source_file}>")
	endforeach(source_file)
	if(EXISTS ${unit_build_file})
		file(READ ${unit_build_file} old_content)
	else()
		set(old_content "")
	endif()
	if (NOT old_content STREQUAL file_content)
		file(WRITE ${unit_build_file} "${file_content}")
	endif()
	set(${SOURCE_VARIABLE_NAME} ${${SOURCE_VARIABLE_NAME}} ${unit_build_file} PARENT_SCOPE)  
endfunction(enable_unity_build)
