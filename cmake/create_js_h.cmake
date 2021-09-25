function (create_js_h)
	file (READ ts/build/index.min.js data)
	file (WRITE cpp/include/js.h "R\"\"\"(\n${data}//)\"\"\";")
endfunction ()

create_js_h()
