REM -- grayscale
ci2pnm.exe -noraw ..\cisuite\basn0g01.ci basn0g01.pgm
ci2pnm.exe -noraw ..\cisuite\basn0g02.ci basn0g02.pgm
ci2pnm.exe -noraw ..\cisuite\basn0g04.ci basn0g04.pgm
ci2pnm.exe -noraw ..\cisuite\basn0g08.ci basn0g08.pgm
ci2pnm.exe -noraw ..\cisuite\basn0g16.ci basn0g16.pgm
REM -- full-color
ci2pnm.exe -noraw ..\cisuite\basn2c08.ci basn2c08.ppm
ci2pnm.exe -noraw ..\cisuite\basn2c16.ci basn2c16.ppm
REM -- paletted
ci2pnm.exe -noraw ..\cisuite\basn3p01.ci basn3p01.ppm
ci2pnm.exe -noraw ..\cisuite\basn3p02.ci basn3p02.ppm
ci2pnm.exe -noraw ..\cisuite\basn3p04.ci basn3p04.ppm
ci2pnm.exe -noraw ..\cisuite\basn3p08.ci basn3p08.ppm
REM -- gray with alpha-channel
ci2pnm.exe -noraw ..\cisuite\basn4a08.ci basn4a08.pgm
ci2pnm.exe -noraw ..\cisuite\basn4a16.ci basn4a16.pgm
REM -- color with alpha-channel
ci2pnm.exe -noraw -alpha basn6a08.pgm ..\cisuite\basn6a08.ci basn6a08.ppm
ci2pnm.exe -noraw -alpha basn6a16.pgm ..\cisuite\basn6a16.ci basn6a16.ppm
REM -- grayscale
ci2pnm.exe -raw ..\cisuite\basn0g01.ci rawn0g01.pgm
ci2pnm.exe -raw ..\cisuite\basn0g02.ci rawn0g02.pgm
ci2pnm.exe -raw ..\cisuite\basn0g04.ci rawn0g04.pgm
ci2pnm.exe -raw ..\cisuite\basn0g08.ci rawn0g08.pgm
ci2pnm.exe -raw ..\cisuite\basn0g16.ci rawn0g16.pgm
REM -- full-color
ci2pnm.exe -raw ..\cisuite\basn2c08.ci rawn2c08.ppm
ci2pnm.exe -raw ..\cisuite\basn2c16.ci rawn2c16.ppm
REM -- paletted
ci2pnm.exe -raw ..\cisuite\basn3p01.ci rawn3p01.ppm
ci2pnm.exe -raw ..\cisuite\basn3p02.ci rawn3p02.ppm
ci2pnm.exe -raw ..\cisuite\basn3p04.ci rawn3p04.ppm
ci2pnm.exe -raw ..\cisuite\basn3p08.ci rawn3p08.ppm
REM -- gray with alpha-channel
ci2pnm.exe -raw ..\cisuite\basn4a08.ci rawn4a08.pgm
ci2pnm.exe -raw ..\cisuite\basn4a16.ci rawn4a16.pgm
REM -- color with alpha-channel
ci2pnm.exe -noraw -alpha rawn6a08.pgm ..\cisuite\basn6a08.ci rawn6a08.ppm
ci2pnm.exe -noraw -alpha rawn6a16.pgm ..\cisuite\basn6a16.ci rawn6a16.ppm

