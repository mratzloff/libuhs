function(create_js_h)
	file(READ ts/build/uhs.js data)
	file(WRITE cpp/include/js.h "R\"\"\"(\n${data}\n//)\"\"\";")
endfunction()

create_js_h()
