echo arg1 = %1
echo arg2 = %2
echo arg3 = %3
dir %2
set DESTDIR=libmbmaster
mkdir %DESTDIR%
xcopy /Y %1\..\..\mbmaster\include\mbm.h %DESTDIR%
xcopy /Y %1\..\..\mbmaster\include\internal\mbmiconfig.h %DESTDIR%
xcopy /Y %1\..\..\mbmaster\include\common\mbtypes.h %DESTDIR%
xcopy /Y %1\port\mbmconfig.h %DESTDIR%
xcopy /Y %1\port\mbport.h %DESTDIR%
xcopy /Y %2mbmaster.lib %DESTDIR%
xcopy /Y %2mbmaster.dll %DESTDIR%
xcopy /Y %2mbmasterd.lib %DESTDIR%
xcopy /Y %2mbmasterd.dll %DESTDIR%
pause
