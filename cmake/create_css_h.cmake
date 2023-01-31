function (create_css_h)
	file (READ build/assets/uhs.min.css data)
	file (WRITE build/include/css.h "R\"\"\"(*/\n${data}/*)\"\"\";")
endfunction ()

create_css_h()
