PROJECT(pixmaps)

FILE(GLOB _pixmaps ${CMAKE_CURRENT_SOURCE_DIR}/*.png  ${CMAKE_CURRENT_SOURCE_DIR}/*.gif)

IF (_pixmaps)
	INSTALL(FILES ${_pixmaps} DESTINATION ${DATA_INSTALL_DIR}/pixmaps)
	INSTALL(FILES gpsicon.xpm DESTINATION share/gpsdrive/pixmaps/)
IF (NOT MAEMO)
	INSTALL(FILES gpsdrive.png DESTINATION share/icons)
ENDIF (NOT MAEMO)
ENDIF (_pixmaps)

