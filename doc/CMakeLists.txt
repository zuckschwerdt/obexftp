
find_program ( ASCIIDOC_EXECUTABLE asciidoc )
find_program ( XMLTO_EXECUTABLE xmlto )

if ( ASCIIDOC_EXECUTABLE AND XMLTO_EXECUTABLE )
  set ( PROGRAMS
    obexftp
    obexftpd
  )

  foreach ( program ${PROGRAMS} )
    set ( ASCIIDOC_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${program}.1.txt )
    set ( ASCIIDOC_XML_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${program}.1.xml )
    set ( ASCIIDOC_HTML_OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${program}.1.html )

    add_custom_command ( OUTPUT ${ASCIIDOC_XML_OUTPUT}
      COMMAND ${ASCIIDOC_EXECUTABLE}
      ARGS --doctype=manpage --backend=docbook
      -o ${ASCIIDOC_XML_OUTPUT} ${ASCIIDOC_INPUT}
      DEPENDS ${ASCIIDOC_INPUT}
      VERBATIM
    )

    add_custom_command ( OUTPUT ${ASCIIDOC_HTML_OUTPUT}
      COMMAND ${ASCIIDOC_EXECUTABLE}
      ARGS --doctype=manpage --backend=xhtml11
      -o ${ASCIIDOC_HTML_OUTPUT} ${ASCIIDOC_INPUT}
      DEPENDS ${ASCIIDOC_INPUT}
      VERBATIM
    )
    list ( APPEND html_help_files ${ASCIIDOC_HTML_OUTPUT} )

    if ( UNIX )
      add_custom_command ( OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${program}.1
        COMMAND ${XMLTO_EXECUTABLE}
        ARGS -o ${CMAKE_CURRENT_BINARY_DIR} man ${ASCIIDOC_XML_OUTPUT}
        DEPENDS ${ASCIIDOC_XML_OUTPUT}
        VERBATIM
      )
      list ( APPEND manpage_files ${CMAKE_CURRENT_BINARY_DIR}/${program}.1 )
    endif ( UNIX )
  endforeach ( program )

  if ( UNIX )
    add_custom_target ( doc ALL
      DEPENDS ${manpage_files}
    )

    install ( FILES ${manpage_files}
      DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
      COMPONENT documentation
      OPTIONAL
    )
  endif ( UNIX )

  add_custom_target ( htmldoc
    DEPENDS ${html_help_files}
  )

  install ( FILES ${html_help_files}
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/html
    COMPONENT documentation
    OPTIONAL
  )
endif ( ASCIIDOC_EXECUTABLE AND XMLTO_EXECUTABLE )
