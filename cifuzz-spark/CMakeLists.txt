#cifuzz:build-template:begin
#add_fuzz_test({{ .FuzzTestName }} {{ .FileName }})
#target_link_libraries({{ .FuzzTestName }} PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,ci_static>")
#target_include_directories({{ .FuzzTestName }} PRIVATE ${CMAKE_SOURCE_DIR}/contrib/examples)
#cifuzz:build-template:end
