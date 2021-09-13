function(create_css_h)
	file(READ cpp/assets/uhs.css data)
	file(WRITE cpp/include/css.h "R\"\"\"(*/\n${data}\n/*)\"\"\";")
endfunction()

create_css_h()
