function (create_js_h)
	file (READ build/ts/index.min.js data)
	file (WRITE build/include/js.h "R\"\"\"(\n${data}//)\"\"\";")
endfunction ()

create_js_h()
