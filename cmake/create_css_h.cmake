function(create_css_h)
	file(READ cpp/assets/build/uhs.min.css data)
	file(WRITE cpp/include/css.h "R\"\"\"(*/\n${data}/*)\"\"\";")
endfunction()

create_css_h()
