function(create_js_h)
	file(READ ts/build/uhs.js data)
	file(WRITE cpp/include/js.h "R\"\"\"(" ${data} ")\"\"\";")
endfunction()

create_js_h()
