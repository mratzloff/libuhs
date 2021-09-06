function(create_js_h)
	file(READ CMakeFiles/CmakeTmp/uhs.js data)
	file(WRITE include/js.h "R\"\"\"(" ${data} ")\"\"\";")
endfunction()

create_js_h()
